# Changelog

## 0.2.0
- The SQ-80 wave ROM is no longer built in: the plugin reads `data/ROM/SQ80.ROM` (raw, 262,144 bytes) at start. Without
  it the display shows `NO ROM` and the output is silent; everything else works.
- Bank D (the 40 SQ-80 factory sounds) is no longer built in either; load your copy through `data/Banks/` and LOAD.
  Bank C (SQ8L's 128 factory sounds) stays.
- With the ROM file the audio is byte-identical to 0.1.0 (checked under QEMU).
- The release manifest lists the ROM under `user_files`, so catalog-aware installers can check and copy it; the zip
  carries `plugin.json` for MPC OS 3.10.1 and later.
- Display glyphs are drawn as an exact 2x of the original pixel font (no more rounded blobs); V, W, X and Y keep the
  original font's narrow shapes.
- Release tools updated to the current mpc-vst-plugins (`plugin.json`, `user_files`), with the L'Cronx apostrophe
  escaped in `install.sh` / `uninstall.sh`.
- Tested on an Akai Force, MPC OS 3.9.1 (2026-11-16).

## 0.1.0
- First release: SQ8L 0.91b on the Akai Force / MPC OS 3.x (ARMv7 VST2), same engine and sound as the 64-bit port by
  sq8l-revival, with the original x87 rounding emulated on ARMv7.
- One-screen panel from the SQ8L artwork: ten big knobs that turn in place, the 44 x 2 display with a glowing
  character look, the page buttons, program number and name, BANK, SAVE, INIT, EXP, LOAD, MIX, SYNC, AM, MONO.
- 168 factory sounds (banks C and D), user banks A and B with a working SAVE, unlimited drop-in banks, `.syx` export.
- Q-Link pages PANEL 1 and PANEL 2, data wheel on the selected parameter.
- Every control and label placed from `layout.json`; Panel Editor included.
- Packaged with the fixed L'Cronx release tools (the ' in L'Cronx is escaped in install.sh and uninstall.sh).
- Tested on an Akai Force, MPC OS 3.9.1 (2026-11-14).
