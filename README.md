# Amenity — 3DS · GBC · GBA

Live jungle-style break instruments for the **Nintendo 3DS**, **Game Boy/
Game Boy Color** and **Game Boy Advance**, inspired by Amenity (ejtb76/
amenity, live jungle firmware for the M-VAVE FM-1).

| Build | Platform | What it does |
|---|---|---|
| 3DS CIA | Native 3DS homebrew | Full-quality break slicer (touch UI, pitch-locked stretch, phrase variation, audio clock-out) |
| `gbc/amenclk.gb` | Game Boy Color/DMG | Clock-out tool (link port + headphone JACK sync), metronome, chiptune kit |
| `gbc_dnb/` | Game Boy Color/DMG | DnB III-style sampled-break player (4-bit wave streaming, banked) |
| `gba/amdnb.gba` | Game Boy Advance | Clean 16-bit Direct-Sound break player |

All are original implementations inspired by Amenity's concepts, rebuilt from
scratch for each platform.

It plays **real drum breaks** from a selectable bank (8 royalty-free breaks
from the same MusicRadar pack HobbyChop uses in Drum & Bass III), each sliced
into **16 eighth-note pieces**, with **pitch-locked time-stretch** and an
**Amenity-style phrase variation** engine so the groove stays musical.

## Features

- **Break bank** — 8 clean/processed breaks (86–116 BPM native), switchable
  live with the BRK button; each is sliced into 16 8th-notes.
- **Pitch-locked time-stretch** — tempo changes speed, not pitch (granular
  overlap-add). No chipmunk breaks.
- **Phrase variation** (like Amenity's VARIATION + SEQ fills) — on each new
  phrase the pattern may be re-sequenced (slice swaps) and finished with a
  roll/build/stretch fill. Amount 0–100, default 45.
- **16 touch pads** — fire single hits; select with D-pad + A.
- **Rolls** (2/4/8 hits per 8th, pitched up), **build** (accelerating
  subdivisions), **stretch** loop, **echo** (dotted-8th damped delay),
  **DJ filter** (LP/HP).
- **BPM 61–250**; classic/behaviour with pitch-lock.
- **Audio clock out** on the left channel: MIDI **24 PPQN** (default) or
  S16/S8 click rates, beat-locked to the sequencer.

## Controls

| Control | Action |
|---|---|
| Touch pads 0–15 | Fire that slice now |
| D-pad + A | Move focus / fire focused slice |
| PLAY/STOP | Transport |
| BPM− / BPM+ | Tempo |
| CLK | Clock out cycle: OFF → CLK24 → S16 → S8 |
| BRK | Next break in the bank |
| ROLL / BUILD / ECHO / STRTCH / FILT | Effect toggles (also X/Y/L) |
| SELECT + LEFT/RIGHT | Variation −/+ (0–100) |
| START | Freeze the current phrase (keep it repeating) |
| VOL− / VOL+ | Master volume |

## Build

Requires the toolchain produced by `tools/build_libs.sh` (libctru + citro3d
built against ARM GNU toolchain 13.3; plus makerom, bannertool). Then:

```
make            # -> build/amenity3ds.cia
```

`tools/host_test.c` renders the engine offline to `build/test.wav`
(slicing, variation, bank switching, effects, 24 PPQN clock — verified at
685-frame intervals @175 BPM).

## Install

Copy `build/amenity3ds.cia` to SD and install with **FBI**; launch from the
HOME menu.

## Clock out

The 3DS has no link port, so the clock is an **audio pulse train on the left
channel** (presence = running). Volca/Pocket Operator sync via a mono 3.5mm
cable (use S16 for one-pulse-per-step Volca behavior); true 5-pin MIDI via a
tiny audio→MIDI click converter (Arduino/Teensy + envelope detector, like
ArduinoBoy but fed from the headphone jack).

## The GBC clock-out ROM (`gbc/amenclk.gb`)

For the authentic HobbyChop-style path there is a separate **Game Boy /
Game Boy Color ROM** in `gbc/` that emits beat-locked pulses on the link
port's **serial-out wire** — the same signal DnB III's CLK.OUT/NANO.OUT puts
on the cable:

- **S24** (default) = MIDI clock rate, 24 PPQN → pair with an ArduinoBoy to
  get real 5-pin MIDI clock/start/stop.
- **S16 / S8** = nanoloop click rates → drive a Volca/Pocket Operator
  directly (serial-out wire → tip, ground → sleeve adapter).
- Controls: LEFT/RIGHT = mode, UP/DOWN = BPM 61–250, A/START = run/stop,
  B = sound on/off, L = jungle kit on/off, SELECT = output LINK (serial) /
  JACK (headphone audio sync).
- The ROM is a clock-out tool: it speaks through the link port by default.
  A metronome click (synced to the pulses, accented every 16th) plays from
  the Game Boy speaker so you can hear it working on real hardware and
  emulators; on emulators there is no link port, so the sound is your
  confirmation it is running. In JACK mode the clock is emitted as an audio
  pulse on the headphone/3.5mm out instead (Volca/PO-style sync). The L
  jungle kit is a chiptune kick/snare/hat kit sequenced to the same BPM —
  not a sampled break (that's a bigger, future feature).

Build with `make -C gbc` (needs the GBDK toolchain downloaded there). Note:
like the 3DS app, this is built and cross-compiled but **not yet verified on
real hardware** — treat the first run as a test.

## Samples / license

App code is original (GPL-3.0). The 8 bundled breaks are from MusicRadar's
free 200-break pack (royalty-free; they ask that you not redistribute them,
so this CIA is for your personal use — replace `assets/breaks/*.wav` and
re-run `tools/wav2bank.py` to build your own bank). The original Amen break
from the amenity repo can be added the same way if you have a legal copy.
