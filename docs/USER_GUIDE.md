# SQ8LMPC user guide

SQ8LMPC 0.2.0. The sound engine, panel and factory bank C are SQ8L by Siegfried Kullmann (see the README for credits).

## Before anything: the ROM

The plugin makes no sound until your own copy of the SQ-80 wave ROM is in `data/ROM/SQ80.ROM` inside the plugin folder
on the device (raw image, exactly 262,144 bytes). Until then the display reads `NO ROM` and the output is silent.
Reload the plugin after copying it. The SQ-80 factory sounds (bank D) are not shipped either: drop your copy into
`data/Banks/` and press LOAD.

## The screen

Everything is on one screen:

- **Program** (top left): the bank letter and number, and the sound's name beside it. **up / down** step through sounds;
  **BANK** moves to the next bank (A, B user; C factory; D empty until you load the SQ-80 sounds).
- **SAVE** writes the current sound into banks A or B. On banks C and D the display shows `** BANK C/D IS FACTORY **`
  and nothing is written. **INIT** loads the initial sound. **EXP** writes the sound as a `.syx` in `data/Export/`.
  **LOAD** steps through the drop-in banks in `data/Banks/`.
- **Page buttons**: OSC 1-3, DCA 1-4, FILTER, MODES, LFO 1-4, ENV 1-4, MATRIX 1-3 and the waves page choose what the ten
  knobs and the display show. **SYNC**, **AM** and **MONO** are switches with a lamp.
- **Display** (middle): 44 x 2 characters, as on the original. Its arrows page up and down.
- **Knobs**: ten knobs, in two rows. They always show the parameter the display names, with its value. They turn in place.

## Your own sounds

SAVE keeps the whole set as `data/User/<set name>.8XL`. It is never touched by an update. Banks C and D cannot be
overwritten.

## More sounds

Put `.8XL`, `.syx`, `.dat`, `.bnk` or `.8XB` files into `data/Banks/`, then press LOAD. A set you saved in `data/User/`
wins over the dropped-in file of the same name.

## Q-Links and the data wheel

PANEL 1 carries the ten knobs (eight at a time per Q-Link page); PANEL 2 carries the rest of the knobs and the main
buttons. The data wheel works on the selected parameter.

## Moving the controls

Open `editor/SQ8LMPC-panel-editor.html`, move or resize any control or label, press **Download layout.json**, put the
file in the repository root and run `./build.sh`. Nothing is moved automatically.

Not made or endorsed by Creative Labs, who own the Ensoniq brand. Use only a ROM you are entitled to use.
