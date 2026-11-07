# Maintainer release checklist

The released runtime implementation is the **unmodified RC7 code** verified on real PSP hardware. Avoid changing `main.c`, `lcdc_imports.S`, `reg_imports.S`, `exports.exp`, or the Makefile without a new hardware verification pass.

1. Build with a configured PSPSDK on the same source tree: `./build_psp.sh`.
2. Confirm 480i/480p × 16:9/4:3, normal game operation, and HOME-menu transitions on PSP-3000 (04g), firmware 6.61 / ARK-5.
3. Run host checks: `./tests/run_host_tests.sh`.
4. Create a GitHub repository (suggested name: `PSP-TV-Fullscreen`) and upload the **contents of this folder** at its root. Do not upload the parent ZIP as a single repository file.
5. Decide on an appropriate redistribution license, if public derivative work is intended. No license has been granted by default.
6. Tag the tested commit `v1.0.0` and create a GitHub Release. Use `RELEASE_NOTES.md` as the release description.
7. If desired, attach **the exact PRX compiled and hardware-tested by the maintainer** as `PSPTVFullscreen.prx` to the GitHub Release. Do not falsely label a source ZIP as a compiled PRX.
8. GitHub automatically offers snapshots of tagged source. Manual source ZIP is optional.
