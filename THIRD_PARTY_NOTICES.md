# Third-party notices

SQ8LMPC would not exist without the work of the people below. Full terms: [LICENSES.md](LICENSES.md).

## SQ8L, by Siegfried Kullmann

SQ8L is an emulation of the Ensoniq SQ-80 synthesizer, © 2006-2008 **Siegfried Kullmann**. The rights to the original work belong to him. The sound engine, the editor behaviour, the panel artwork and the factory bank C in SQ8LMPC are his work, brought to the Akai Force with the 64-bit port below. Factory bank C is credited to **Ole Jeppesen** and Siegfried Kullmann. The SQ-80 wave ROM and the SQ-80 factory sounds (bank D) are **Ensoniq**'s and are not included since 0.2.0: the user supplies their own copies. Ensoniq, ESQ-1 and SQ-80 are trademarks of their respective owners.

SQ8LMPC grants no licence on any of this. It is published free of charge, for preservation only, in good faith. If you hold rights and object, open an issue and a private contact will be arranged.

## The 64-bit port, by sq8l-revival

[github.com/sq8l-revival/SQ8L](https://github.com/sq8l-revival/SQ8L) brought SQ8L 0.91b to 64-bit systems and made its emulation reproducible. The engine and editor logic used here come from that project, with their changes for ARMv7 listed in the CHANGELOG. Their licence and their own third-party notices are kept unchanged in [third_party/sq8l-revival/](third_party/sq8l-revival/).

## mpc-vst-plugins, by sd88me

The VST2 wrapper that lets a plugin run on MPC OS (the Akai Force and MPC), the skin generator and the release tools. MIT, Copyright (c) 2026 sd88me. Text: `third_party/mpc-vst-plugins/LICENSE`. https://github.com/sd88me/mpc-vst-plugins

## CORE-MATH

Correctly rounded exp, log, sin, cos and tan, used so the sound matches the original bit for bit. MIT, by the CORE-MATH authors (Inria). Text: `third_party/core-math/LICENSE`. https://core-math.gitlabpages.inria.fr/

## Michroma

The letters printed on the panel are set in Michroma, Copyright 2011 The Michroma Project Authors (https://github.com/googlefonts/Michroma-font), SIL Open Font License 1.1. Text: `art/fonts/Michroma-OFL.txt`.

## Tools used to build, not distributed

Zig (as a cross compiler), Python with Pillow, NumPy and SciPy.
