# Building & using Amenity3DS

A live jungle-style **break slicer** for the Nintendo 3DS (native homebrew CIA)
plus a **Game Boy Color clock-out ROM** (`gbc/`).

## Repository layout

| Path | What it is |
|---|---|
| `source/` | 3DS app: engine, UI, gfx, startup/CRT, linker script |
| `tools/` | `wav2bank.py` (breaks → C bank), `host_test.c` (offline audio check), `make_assets.py`, `rsf.cia`, `build_libs.sh` |
| `gbc/` | GBC clock-out ROM (`main.c`, Makefile, GBDK) |
| `docs/BUILDING.md` | This file |

## Quickstart

```
# 1. add your breaks (required — the bank is generated from them)
mkdir -p assets/breaks
# copy 16-slice-friendly WAVs (e.g. two-bar loops) into assets/breaks/

# 2. build the 3DS CIA (requires the toolchain from tools/build_libs.sh)
make                     # -> build/amenity3ds.cia

# 3. offline audio sanity check (needs only gcc + libm)
make hosttest            # renders build/test.wav, prints stats

# 4. build the GBC clock ROM (downloads GBDK on first use)
make -C gbc             # -> gbc/amenclk.gb
```

### Toolchain prerequisites
- 3DS: `tools/build_libs.sh` builds libctru + citro3d from source against ARM GNU
  toolchain 13.3, and builds `makerom` + `bannertool` (all fetched/compiled
  locally; no devkitPro server needed). It installs libs to `~/devkitpro`.
- GBC: `make -C gbc` downloads the GBDK-2020 Linux toolchain into `gbc/gbdk/`.

## Adding / swapping breaks

1. Put WAVs in `assets/breaks/` (16/24-bit stereo or mono; any sample rate).
2. Name them so the BPM is obvious: `wav2bank.py` reads the first 2–3 digit
   group as the break's native BPM (used for auto-tempo on BRK press).
3. `rm -f source/break_bank.h && make` — the bank is regenerated.
   Each break is converted to 44.1 kHz mono, DC-blocked, normalized, and its
   loop length is rounded down to a multiple of 16 slices.

## Install (3DS)

Copy `build/amenity3ds.cia` to the SD card and install with **FBI**; launch
from the HOME menu.

## Controls — 3DS app

| Input | Action |
|---|---|
| Touch pads 0–15 | Fire that slice |
| D-pad + A | Select / fire pad |
| PLAY / STOP | Transport |
| BPM− / BPM+ | Tempo 61–250 |
| CLK | Clock out: OFF → 24 PPQN → S16 → S8 |
| BRK | Next break (auto-matches tempo) |
| ROLL / BUILD / ECHO / STRTCH / FILT | Effects |
| X / Y / L | Roll / echo / filter shortcuts |
| START | Freeze phrase |
| SELECT + LEFT/RIGHT | Variation −/+ |
| SELECT + UP/DOWN | Pattern slot −/+ |
| SELECT + A / B | Save / load pattern (session) |
| L+R | Guide menu |
| VOL− / VOL+ | Master volume |

## Controls — GBC clock-out ROM

| Input | Action |
|---|---|
| LEFT/RIGHT | Mode: S24 (MIDI 24 PPQN) / S16 / S8 |
| UP/DOWN | BPM 61–250 |
| A / START | Run / stop |
| B | Metronome click on/off (default on) |

Output is on the link port's **serial-out wire** — pair with an ArduinoBoy for
5-pin MIDI clock, or use the serial-out → tip / ground → sleeve adapter for
Volca / Pocket Operator. An audible metronome click (synced to the pulses,
accented every 16th) plays from the Game Boy speaker so you can hear it on
real hardware and emulators; B mutes it.

## Audio / licensing notes

- The app code is original (GPL-3.0).
- The breaks are **not** committed to this repo (they are royalty-free but not
  redistributable; this is a private, personal build). Drop your own in
  `assets/breaks/` and rebuild.
- `source/break_bank.h` is generated and also not committed.

## Test status

- Offline: the audio engine is validated (slicing, variation, bank switching,
  effects, 24 PPQN clock timing) via `make hosttest`.
- Not yet verified on real hardware: 3DS boot/audio/UI and the GBC link-port
  pulse timing. Treat first runs as tests; report any symptom
  (e.g. rotated UI, no sound, wrong tempo) to fix quickly.
