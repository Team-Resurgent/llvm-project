# Team Resurgent's llvm-project fork

Home branch for [Team Resurgent](https://github.com/Team-Resurgent)'s original Xbox and Xbox 360
toolchain forks of [llvm/llvm-project](https://github.com/llvm/llvm-project). This branch carries
no LLVM source — it only exists to host both platforms' CI workflows in one place (so they show up
under the Actions tab's manual "Run workflow" dispatch without switching branches) and to be a
sensible default branch for repo visitors.

## Branches

| Branch | Platform | Target | Purpose |
|---|---|---|---|
| [`xbox360`](https://github.com/Team-Resurgent/llvm-project/tree/xbox360) | Xbox 360 | `powerpc-unknown-xbox360` (MS-PPC ABI) | Patched clang + lld — [RXDK-360](https://github.com/Team-Resurgent/RXDK-360) fetches the CI-built toolchain zips at build time. |
| [`xboxog`](https://github.com/Team-Resurgent/llvm-project/tree/xboxog) | Original Xbox | `i686-pc-windows-gnu` | Source for [RXDK-Libs](https://github.com/Team-Resurgent/RXDK-Libs)' `libcxx`/`libcxxabi`/`libunwind` (built with Zig's own toolchain, not a standalone clang) + patches. |

Each platform branch has every source RXDK doesn't build stripped out and RXDK-specific patches
applied directly, rather than carried as external `.patch` files.

## CI

- **Xbox 360 clang** (`build-xbox360-clang.yml`) — builds on push to `xbox360`, publishes
  `xbox360-<os>-<arch>.zip` to the moving `latest` release. Consumed by RXDK-360.
- **Original Xbox clang** (`build-xboxog-clang.yml`) — builds on push to `xboxog`, publishes
  `xboxog-<os>-<arch>.zip` to the moving `latest` release. Not currently consumed by anything
  (RXDK-Libs uses Zig's bundled toolchain) — built for future use.

Both are also manually dispatchable from this branch's Actions tab.
