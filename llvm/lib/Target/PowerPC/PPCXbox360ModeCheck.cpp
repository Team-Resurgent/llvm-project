//===-- PPCXbox360ModeCheck.cpp - Xenon execution-mode checks -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Xbox 360 titles run in the PowerPC 32-bit computation mode (MSR[SF]=0) on a
// CPU (Xenon) whose VMX unit omits the AltiVec integer multiplies. Measured on
// a development kit:
//   - XER[CA] and XER[OV] come from the low 32 bits of an add or subtract;
//   - a record form (Rc=1) sets CR0 from the low 32 bits of its result;
//   - vmule*, vmulo*, vmhaddshs, vmhraddshs, vmladduhm, vmsum* and vsum* are
//     illegal instructions.
// (sradi/srad set CA from the full 64-bit operand, and cmpd/cmpld compare all
// 64 bits, as in the 64-bit mode.)
//
// Instruction selection avoids all of these for this target. This pass checks
// the final code and reports an error instead of emitting something the
// console would execute differently:
//   - one of the VMX integer multiplies (e.g. from an AltiVec intrinsic);
//   - a read of CA whose last definition in the block is a 64-bit
//     add/subtract-with-carry;
//   - a 64-bit record form whose CR0 result is used, unless the low word of
//     its result determines the comparison: always for a result that is
//     sign-extended from its low word or below 2^31 (andi., extsw., cntlzd.,
//     rldicl. clearing at least 33 high bits, ...), and for EQ/NE when its
//     high word is zero (rldicl. clearing exactly 32, andis., ...).
//
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "MCTargetDesc/PPCPredicates.h"
#include "PPCInstrInfo.h"
#include "PPCSubtarget.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"

using namespace llvm;

#define DEBUG_TYPE "ppc-xbox360-mode-check"

namespace {

class PPCXbox360ModeCheck : public MachineFunctionPass {
public:
  static char ID;
  PPCXbox360ModeCheck() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override {
    return "PowerPC Xbox 360 execution-mode check";
  }
};

} // end anonymous namespace

char PPCXbox360ModeCheck::ID = 0;

INITIALIZE_PASS(PPCXbox360ModeCheck, DEBUG_TYPE,
                "PowerPC Xbox 360 execution-mode check", false, false)

FunctionPass *llvm::createPPCXbox360ModeCheckPass() {
  return new PPCXbox360ModeCheck();
}

// The AltiVec integer multiplies Xenon does not implement.
static bool isXenonMissingVMX(unsigned Opc) {
  switch (Opc) {
  case PPC::VMULEUB: case PPC::VMULESB: case PPC::VMULEUH: case PPC::VMULESH:
  case PPC::VMULOUB: case PPC::VMULOSB: case PPC::VMULOUH: case PPC::VMULOSH:
  case PPC::VMHADDSHS: case PPC::VMHRADDSHS: case PPC::VMLADDUHM:
  case PPC::VMSUMUBM: case PPC::VMSUMMBM: case PPC::VMSUMUHM:
  case PPC::VMSUMUHS: case PPC::VMSUMSHM: case PPC::VMSUMSHS:
  case PPC::VSUM4UBS: case PPC::VSUM4SBS: case PPC::VSUM4SHS:
  case PPC::VSUM2SWS: case PPC::VSUMSWS:
    return true;
  default:
    return false;
  }
}

// 64-bit add/subtract instructions whose CA output is the 64-bit carry on a
// 64-bit-mode CPU but the low-word carry on the console.
static bool is64BitCarryDef(unsigned Opc) {
  switch (Opc) {
  case PPC::ADDC8: case PPC::ADDC8_rec: case PPC::ADDE8: case PPC::ADDE8_rec:
  case PPC::ADDIC8: case PPC::SUBFC8: case PPC::SUBFC8_rec: case PPC::SUBFE8:
  case PPC::SUBFE8_rec: case PPC::SUBFIC8: case PPC::ADDME8:
  case PPC::ADDME8_rec: case PPC::ADDZE8: case PPC::ADDZE8_rec:
  case PPC::SUBFME8: case PPC::SUBFME8_rec: case PPC::SUBFZE8:
  case PPC::SUBFZE8_rec:
    return true;
  default:
    return false;
  }
}

// How far a CR0 computed from the low 32 bits of a 64-bit record form's result
// (as on the console) agrees with one computed from all 64 bits.
enum class LowWordCR0 {
  Exact,  // the result is sign-extended from its low word: every bit agrees
  EqOnly, // the high word is zero: EQ agrees, LT/GT may not
  None    // nothing can be said
};

static LowWordCR0 lowWordCR0(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case PPC::ANDI8_rec: // <= 0xffff
  case PPC::EXTSB8_rec:
  case PPC::EXTSH8_rec:
  case PPC::EXTSW_rec:
  case PPC::EXTSW_32_64_rec:
  case PPC::CNTLZD_rec: // <= 64
  case PPC::CNTTZD_rec:
    return LowWordCR0::Exact;
  case PPC::ANDIS8_rec: // bits 16..31 only
    return LowWordCR0::EqOnly;
  case PPC::RLDICL_rec: {
    // Clears the MB high bits (IBM numbering): MB > 32 leaves a value below
    // 2^31, MB == 32 a zero-extended word.
    int64_t MB = MI.getOperand(3).getImm();
    return MB > 32 ? LowWordCR0::Exact
                   : MB == 32 ? LowWordCR0::EqOnly : LowWordCR0::None;
  }
  case PPC::RLWINM8_rec: {
    // A mask that does not wrap stays in the low word; MB > 0 also clears its
    // sign bit.
    int64_t MB = MI.getOperand(3).getImm(), ME = MI.getOperand(4).getImm();
    if (MB > ME)
      return LowWordCR0::None;
    return MB > 0 ? LowWordCR0::Exact : LowWordCR0::EqOnly;
  }
  case PPC::SRADI_rec: // >= 32: sign-extended from the low word
    return MI.getOperand(2).getImm() >= 32 ? LowWordCR0::Exact
                                           : LowWordCR0::None;
  default:
    return LowWordCR0::None;
  }
}

// Whether every read of CR0 after MI (until it is redefined) looks only at EQ:
// an EQ/NE branch on cr0, or a read of the cr0eq bit. CR0 must not be live out
// of the block.
static bool cr0UsesAreEqOnly(const MachineInstr &MI,
                             const TargetRegisterInfo &TRI) {
  const MachineBasicBlock &MBB = *MI.getParent();
  for (MachineBasicBlock::const_iterator I = std::next(MI.getIterator()),
                                         E = MBB.end();
       I != E; ++I) {
    for (const MachineOperand &MO : I->operands()) {
      if (!MO.isReg() || !MO.isUse() || !MO.getReg() ||
          !TRI.regsOverlap(MO.getReg(), PPC::CR0))
        continue;
      if (MO.getReg() == PPC::CR0EQ)
        continue;
      if (MO.getReg() == PPC::CR0 && I->isBranch() && I->getNumOperands() > 1 &&
          I->getOperand(0).isImm() && &I->getOperand(1) == &MO) {
        unsigned Cond = PPC::getPredicateCondition(
            (PPC::Predicate)I->getOperand(0).getImm());
        if (Cond == PPC::PRED_EQ || Cond == PPC::PRED_NE)
          continue;
      }
      return false;
    }
    if (I->definesRegister(PPC::CR0, &TRI))
      return true;
  }
  for (const MachineBasicBlock *Succ : MBB.successors())
    for (MCRegAliasIterator A(PPC::CR0, &TRI, /*IncludeSelf=*/true); A.isValid();
         ++A)
      if (Succ->isLiveIn(*A))
        return false;
  return true;
}

static void report(MachineFunction &MF, const MachineInstr &MI,
                   const PPCInstrInfo &TII, const Twine &Why) {
  MF.getFunction().getContext().diagnose(DiagnosticInfoUnsupported(
      MF.getFunction(),
      "Xbox 360: " + Twine(TII.getName(MI.getOpcode())) + ": " + Why,
      MI.getDebugLoc()));
}

bool PPCXbox360ModeCheck::runOnMachineFunction(MachineFunction &MF) {
  const PPCSubtarget &ST = MF.getSubtarget<PPCSubtarget>();
  if (!ST.isXbox360ABI())
    return false;
  const PPCInstrInfo &TII = *ST.getInstrInfo();
  const TargetRegisterInfo &TRI = *ST.getRegisterInfo();
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  for (MachineBasicBlock &MBB : MF) {
    // Forward: the instructions that define CA, and the reads of it.
    const MachineInstr *CADef = nullptr;
    for (const MachineInstr &MI : MBB) {
      if (MI.isDebugInstr())
        continue;
      if (isXenonMissingVMX(MI.getOpcode()))
        report(MF, MI, TII, "Xenon does not implement this VMX instruction");
      if (CADef && MI.readsRegister(PPC::CARRY, &TRI) &&
          is64BitCarryDef(CADef->getOpcode()))
        report(MF, MI, TII,
               "reads the carry of a 64-bit " +
                   Twine(TII.getName(CADef->getOpcode())) +
                   ", which the console computes from the low 32 bits");
      if (MI.modifiesRegister(PPC::CARRY, &TRI))
        CADef = &MI;
    }

    // Backward: 64-bit record forms whose CR0 is live afterwards.
    LivePhysRegs Live(TRI);
    Live.addLiveOuts(MBB);
    for (const MachineInstr &MI : llvm::reverse(MBB)) {
      if (!MI.isDebugInstr() && MI.getNumOperands() > 0 &&
          MI.getOperand(0).isReg() && MI.getOperand(0).isDef() &&
          MI.getOperand(0).getReg().isPhysical() &&
          PPC::G8RCRegClass.contains(MI.getOperand(0).getReg()) &&
          MI.definesRegister(PPC::CR0, &TRI) &&
          !Live.available(MRI, PPC::CR0)) {
        LowWordCR0 Kind = lowWordCR0(MI);
        if (Kind == LowWordCR0::None ||
            (Kind == LowWordCR0::EqOnly && !cr0UsesAreEqOnly(MI, TRI)))
          report(MF, MI, TII,
                 "a record form sets CR0 from the low 32 bits of its 64-bit "
                 "result on the console");
      }
      Live.stepBackward(MI);
    }
  }
  return false;
}
