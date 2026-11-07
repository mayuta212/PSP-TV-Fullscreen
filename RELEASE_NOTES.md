# PSP TV Fullscreen v1.0.0

**Validated on real hardware:** PSP-3000 (04g), firmware 6.61, ARK-5. All four combinations of TV-out (480i/480p) and aspect ratio (16:9/4:3) were confirmed functional on 2026-11-07, including in-game operation and HOME menu transitions.

The plugin uses Sony's native scaler to expand PSP games to a 698-pixel-wide TV picture. It supports native presets **698×480 (16:9)** and **698×360 (4:3)**, and it preserves normal 480i output timing. Progressive handling follows the previous verified implementation.

**Important:** A GBS-Control setup showed horizontal image splitting with some Motion Adaptive deinterlacer states, even with all PSP PRXs disabled. Explicit BOB selection resolved it. See README for the precise, setup-specific workaround. This is not proof of a general GBS-Control bug.

**Release asset:** Attach the real hardware-tested `PSPTVFullscreen.prx` as a binary asset if available. This repository ZIP includes **source only** and cannot be installed directly as a PRX. Build with PSPSDK: `make clean && make`.

**Scope:** GAME plugin only. PSP-3000 (04g) / 6.61 / ARK-5 verified; other platforms untested. No nearest-neighbor filter changes.

**Freeze note:** The `main.c` and support assembly files are byte-for-byte unchanged from the tested RC7 source. The RC7 label in code is deliberately retained.
