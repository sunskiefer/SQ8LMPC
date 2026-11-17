# Licences: who owns what

SQ8LMPC is made of four kinds of material. Each keeps its own owner and its own terms.

| Material | Where | Owner | Terms |
|---|---|---|---|
| **The original SQ8L**: its behaviour, panel artwork, sprites and factory bank C | `src/engine/data/`, the original bitmaps used for the panel (`art/orig/`), the sounds built into the plugin | © 2006-2008 **Siegfried Kullmann**. Factory bank C: Ole Jeppesen and Siegfried Kullmann | **No licence is granted by this repository.** Published for preservation only, free of charge, in good faith. See "Takedown" below |
| **The 64-bit port of SQ8L** (the engine and editor logic in `src/engine/`, `src/gui/`) | `src/engine/`, `src/gui/`, `third_party/core-math/` | **sq8l-revival** (github.com/sq8l-revival/SQ8L) | Their own terms, kept unchanged in [third_party/sq8l-revival/LICENSE.md](third_party/sq8l-revival/LICENSE.md) and [THIRD_PARTY_NOTICES.md](third_party/sq8l-revival/THIRD_PARTY_NOTICES.md). Nothing in this repository changes them |
| **The MPC / Force wrapper** (framework) | `third_party/mpc-vst-plugins/` | **sd88me** and the mpc-vst-plugins contributors | MIT, see `third_party/mpc-vst-plugins/LICENSE` |
| **CORE-MATH** (exp, log, sin, cos, tan) | `third_party/core-math/` | The CORE-MATH authors (Inria) | MIT, see `third_party/core-math/LICENSE` |
| **Michroma** (the lettering printed on the panel) | `art/fonts/` | The Michroma Project Authors | SIL Open Font License 1.1, see `art/fonts/Michroma-OFL.txt` |
| **The Force / MPC port written here**: the plugin glue, the parameter table, the panel and skin builder, the Panel Editor, the tests and the build | `src/sq8lmpc/`, `tools/`, `tests/`, `build.sh`, `editor/`, `layout.json` | © 2026 **L'Cronx** (sunskiefer) | **PolyForm Noncommercial 1.0.0**, see [LICENSE](LICENSE): free to use, copy and change for any noncommercial purpose, not to be sold |

## Not included: Ensoniq's data

The SQ-80 wave ROM and the SQ-80 factory sounds (bank D) are **Ensoniq**'s. Since 0.2.0 they are not in this repository
and not in the release: the plugin reads the user's own copies from `data/ROM/` and `data/Banks/` on the device. This
project does not provide them and does not point to a source for them.

## What this does not do

- It does not relicense anyone else's work. A file that names another licence in its header keeps that licence.
- PolyForm Noncommercial applies only to the work of L'Cronx in the last row. It does not apply to SQ8L, to Kullmann's or Ensoniq's material, or to the third-party code above.

## Trademarks

Ensoniq, ESQ-1 and SQ-80 are trademarks of their respective owners. SQ8LMPC is an unofficial project. It is not affiliated with or endorsed by Siegfried Kullmann, Ensoniq, Creative Labs (owner of the Ensoniq brand) or its other successors, sq8l-revival, or Akai Professional.

## Takedown

If you hold rights to any part of this work and object to its publication, open an issue on this repository and ask for a private contact. The material will be removed.
