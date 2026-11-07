# Changelog

## v1.0.0 — 2026-11-07

- Frozen release of the **unchanged RC7 implementation**, after successful on-hardware verification of 480i/480p and 16:9/4:3.
- Native Sony scaling presets: 5 (698×480), 1 (698×360).
- 480i uses Sony's normal XMB-to-GAME timing transition without forced LCDC mode refresh.
- 480p continues to use the proven progressive-only refresh path.
- Known GBS-Control issue documented: some MA states displayed PSP 480i with a split image, corrected by selecting BOB in the test setup.
- Updated GitHub packaging only (README, release notes, CI). PSP runtime implementation is unchanged from RC7.

## v1.0.0-rc7 — 2026-11-07

- Restored full 698px width to 480i after narrower RC6 experiments.
- Updated classification for scaled 480i to 698×480 / 698×360.
- Preserved progressive path; retained startup CSV logging and read-only MMIO inspection.

## Prior experimental revisions

RC1 established the progressive path. RC2–RC6 investigated interlaced support and input behavior; those experimental revisions are not part of this release. See `main.c` for the frozen implementation.
