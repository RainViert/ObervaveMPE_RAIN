# Changelog

OberVave for the M-VAVE FM-1 (and, as a beta, the M-Vave SMK-37). Newest first. Each release is on
[GitHub](https://github.com/zvenson/obervave/releases) and on [the Firmware Hub](https://thefirmwarehub.com/firmware/obervave/).

## 3.5-rain14 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **DELAY 2 reverse** (DELAY 2 +, knob 4 DIR: FWD / REVERSE): two heads read the last stretch of the delay time
  backwards, crossfaded as triangles (no clicks where a head jumps back); the feedback, tone and RATE work as before,
  the longest time is half (370 ms at FULL up to 2.97 s at 1/8). Kept with the slots (with DELAY 1's STEREO).
- **Less CPU time** (an overload with MPE chords, both delays and the reverb): every delay line's buffer is a power
  of two now, a read is a mask instead of a wrap test (the sound bit-exact as before); the saturation of a line
  written is one test; the chorus reads its two taps linearly (was 4-point Hermite). About 11 % less in all in a
  heavy scene (6 voices, both delays, chorus, reverb), ~17 % of the effects'.

## 3.5-rain12 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **Louder echoes and reverb**: the MIX of DELAY, DELAY 2 and REVERB goes up to twice the old top (1.4 of the dry
  sound at 127, was 0.7). Up to 64 nothing changes; above it the return rises faster (96: x 1.34, 112: x 1.66). The
  soft knee after the effects keeps a hot return from scratching.
- **The matrix on the sends**: three more destinations, DLY SND, DLY2 SND and REV SND, per voice: +100 % adds the whole
  send on top of the track's own (from the mixer), -100 % takes it away. VELO > DLY SND: loud notes echo, soft ones
  stay dry; PRESS > REV SND: pressing a note further sends it into the hall. A delay a slot sends into does not rest.

## 3.5-rain10 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **The matrix belongs to the sound**: MEMORY's SOUND keeps the track's matrix with the program, and a program loaded
  (PRESETS, MIDI program change) brings its matrix (the factory programs and those saved before: none). Changing the
  matrix marks the sound changed (`*`). The 99 matrices live in the settings sector (2376 B), the programs' own
  sectors are full.

## 3.5-rain9 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **The modulation matrix**, as on the Oberheim Matrix-6 / -12: LFO tapped a fourth time opens **MATRIX**, 8 slots per
  track. KNOB 1 the slot, 2 the SOURCE, 3 the DEST, 4 the AMOUNT (-100..+100 %); the eight slots are listed where the
  scope is (the chosen one in amber).
  - Sources: LFO, F-ENV, A-ENV, VELO, KEY (+-5 octaves around C4), PRESS (MPE pressure per note, else the track's
    aftertouch), SLIDE (MPE CC 74 per note), WHEEL, RAND (one value per note).
  - Destinations: PITCH (100 % = an octave), OSC 1, OSC 2, PW, CUTOFF (100 % = 4 octaves), RESO, LEVEL (up to x 2),
    PAN, O1 LVL, O2 LVL, NOISE, X-MOD.
  - Worked per voice every 16 samples; a track with no slot in use costs nothing (two slots: ~0.2 % CPU).
  - Kept with MEMORY's slots (SAVE, SAVE) and with the settings; slots saved before load with an empty matrix. A
    program saved as SOUND (1-99) does not carry it.
- Channel pressure (aftertouch) is now taken per track, for PRESS without MPE.

## 3.5-rain8 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- The screen says which one it is: **RAIN** under the logo, "OBERVAVE RAIN" on the crash and rescue screens.
- `fmhub.json` and screenshots (`docs/rain/`) for the Firmware Hub.

## 3.5-rain7 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **The plate reverb at half the rate** (REV_DIV 2, OberVave's own option): about 12 % less CPU time in all, room
  for the MPE voices and both delays together (with all of it at once the FM-1 ran out of time and crackled). The hall
  sounds the same, its top is a little darker.

## 3.5-rain6 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **The delays no longer scratch with much feedback**: their feedback saturates softly (a knee at -2.5 dB, tanh
  above, as a tape echo) instead of clipping hard, and so does the sum after the effects. The mono delays' return is
  3 dB lower (it is on both sides).

## 3.5-rain5 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **The delays' settings are saved with MEMORY's slot**, as the other effects: DELAY + (RATE, MODE, TIME, STEREO),
  DELAY 2, DELAY 2 + and the mixer's SEND DELAY 2. SAVE, SAVE keeps them, LOAD brings them back. Slots saved before
  still load (with the delays as they are).
- **Fixed: rain4 crashed right at power-on.** The sound starts before the effects' lines exist; DELAY 2 had a
  MIX from the start and read a line not made yet. No delay runs now before the effects are set up.
- A delay no track sends into rests (less CPU time).
- The crash screen shows three more numbers (the cause bits and the stack pointer), for finding a crash.

## 3.5-rain4 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **Two delays**: the ping-pong delay is now two mono delays, each with the full time (up to 5.94 s at RATE 1/8),
  their echoes in the middle. **DELAY** and **DELAY +** are DELAY 1 as before; FX steps on to **DELAY 2** (TIME as a
  note value, FEEDBK, TONE, MIX) and **DELAY 2 +** (RATE, MODE SYNC / TIME, TIME in ms). The mixer has a row
  **SEND DELAY 2** after SEND DELAY: each track's send into DELAY 2 (kept with the settings, not per pattern).
  rain3's settings stay; DELAY 2 starts on a dotted eighth with no sends.
- **DELAY 1 STEREO** (DELAY +, knob 4): MONO or PINGPONG (the old ping-pong, on the two halves of DELAY 1's line:
  half the longest time, 370 ms at FULL up to 2.97 s at 1/8).

## 3.5-rain3 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **Delay in milliseconds** (FX → **DELAY +**): MODE SYNC (as before, the note value of DELAY's TIME at the tempo) or
  TIME, then TIME sets the delay freely, 10 ms up to what the RATE holds (FULL 740 ms, 1/2 1.48 s, 1/4 2.97 s, 1/8
  5.94 s), about 2 % a click. Kept with the settings; 3.5-rain1's settings stay.

## 3.5-rain1 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **MPE on track A** (GLO → **MPE**): MPE OFF or the members' bend range (2, 12, 24, 48, 96 semitones). Channel 1 is the
  master (track A's bend, CCs, pedal as ever), channels 2–16 each carry one note with its own pitch bend, pressure and
  slide (CC 74), followed through a ~3 ms glide. PRESS> and SLIDE> send pressure and slide to LEVEL, CUTOFF, VIBRATO or
  PWM, each with its amount (PR AMT, SL AMT).
- **Own MIDI channels for tracks B, C, D** (GLO → **MPE + CHANNELS**: CH B, CH C, CH D; AUTO = as before).
- **Envelopes to 60 s**: the filter and amp envelopes' times as before up to 3.9 s, then on to 60 s.
- **Delay RATE** (FX → **DELAY +**): FULL (743 ms a side, as before), 1/2 (1.5 s), 1/4 (3 s), 1/8 (6 s) in the same
  memory, the repeats darker at each step; the delay TIME gets 1/2. and 1/1.

## 3.5.2 – 2026-11-13

- **Locked to an external MIDI clock:** with a master that sends START and clock (SYNC AUTO), the sequencer, the
  tracks and the arp now stay on the master's beat for good. Before, the clock only set the tempo and the beat slowly
  slid away (up to half a second in five minutes). Each quarter is now gently nudged back onto the master's (at most
  ~3 %, inaudible); a tempo change takes effect on the next beat, and the tempo is there after one quarter instead of
  four, so a master that starts its clock with PLAY leads from the second beat. Without a START (the clock running,
  OberVave started with its own PLAY) it follows the tempo as before. Written and tested on an FM-1 by adub671
  ([#25](https://github.com/zvenson/obervave/pull/25)), reported by dmochen and others
  ([#10](https://github.com/zvenson/obervave/issues/10), [#17](https://github.com/zvenson/obervave/issues/17)) –
  thank you!
- Identity `FM-1_9535` (as 3.5).

## 3.5.1 – 2026-11-13

- **MASTER works again:** VOLUME, DJ FILTER, DRIVE and COMP on the mixer's MASTER row now change the sound as you turn
  them – before, they only took effect with the next pattern change. Reported by dmochen
  ([#19](https://github.com/zvenson/obervave/issues/19)) – thank you!
- The browser simulator has the same fix.
- Identity `FM-1_9535` (as 3.5).

## 3.5 – 2026-11-12

- **Saving made simple: MEMORY.** SAVE opens one page with the 8 memory slots. A slot holds everything: the steps of
  all four tracks, their four sounds, the mixer, the effects and MASTER. KNOB 1 (or PRESETS) picks the slot, KNOB 2
  (or SELECT) the action, SAVE does it: **SAVE**, **LOAD**, **COPY**, **PASTE**, **INIT** and **SOUND** (the chosen
  track's sound into one of the 99 programs). SAVE, SAVE saves what you hear into its slot.
- **No auto-save any more.** What you play is a working copy of the slot loaded last; a `*` shows changes not saved
  yet. LOAD the same slot again and you are back to how it was saved. Anything that would throw changes away, or
  overwrite another slot, asks once more. Changing patterns with unsaved changes warns first. (Global settings such
  as tempo and MIDI channel still keep themselves.) Thanks to the player on the hub who asked for exactly this.
- Gone: SAVE held + keys 1-8 (go / copy), SAVE PATTERN?, the PATTERNS view, the program "save as". SAVE held + SELECT
  still dices a new sound.
- **Backup:** programs, patterns and settings to a file on your computer and back, from the browser:
  [thefirmwarehub.com/tools/obervave-backup](https://thefirmwarehub.com/tools/obervave-backup/). Asked for by
  dmochen ([#15](https://github.com/zvenson/obervave/issues/15)).
- **VIBE LINK:** the FM-1 as a controller for the VIBE FM from inside OberVave (OUTPUT page, SELECT pressed: OFF / ON /
  CONTROL). ON: OberVave plays on and its knobs, buttons and keys also go to the VIBE; CONTROL: only the controller
  (OCT- and OCT+ held: back).
- Identity `FM-1_9535`.

## 3.3.2 – 2026-11-02

- OberVave's screen, live and as big as you like, on a computer or phone with
  [FM-1 Mirror](https://github.com/pfkellogg/fm1-mirror) by Patrick F. Kellogg. Sent only while FM-1 Mirror asks for it.

## 3.3.1 – 2026-11-02

- Newer FM-1s carry another flash chip (UC25HQ80): OberVave saved to RAM only on them and refused updates. Fixed
  (thanks to Paul Nederhoed).

## 3.3 – 2026-10-31

- Fatter: **LP 24 is a real four-pole ladder** with the resonance around all four stages and a soft stage in the loop,
  so the resonance compresses instead of thinning and the bass stays.
- No hard clipping in a voice any more: the oscillator mix and the filter output go through soft stages; DRIVE goes from
  warm to fuzz. VINTAGE moves the cutoff too, each voice on its own.
- VOICE (mode, unison, detune, glide) is under EDIT after OSC 2; GLO is GLOBAL / OUTPUT.

## 3.0.1 – 2026-10-23

- Its own identity `FM-1_9530` (3.0 shared one with other firmwares, so the hub could not tell them apart).

## 3.0 – 2026-10-23

- **Four tracks** A-D on the eight voices, each its own sound, arpeggiator and sequence.
- A **mixer** (level, pan, sends to chorus, delay, reverb and drive, mute, solo, meters), **DRIVE** and a **MASTER**
  (DJ filter, drive, compressor, limiter).
- Tracks start and stop on the bar, like clips; every pattern keeps its own sounds and mix.
- Recording with a quantise per track, nudge, legato, arp takes and recorded knob moves; SYNC AUTO / INTERNAL.
- 1.5's programs, settings and patterns come over on the first start.

## 1.5.3 – 2026-10-19

- The update loader shows what it does, in the Firmware Hub's look (the idea from Skiff).

## 1.5.2 – 2026-10-19

- The FM-1's **TRS MIDI IN** jack works (through a TRS-to-DIN adapter, type A or B); the keys send on the set MIDI channel.

## 1.5.1 – 2026-10-18

- 1.5, built and signed by GitHub Actions.

## 1.5 – 2026-10-16

- 99 programs: 64 factory sounds (33-64 new), save a sound into any slot.

## 1.4.3 – 2026-10-16

- HOLD per sound: track A's and B's arpeggiators latch on their own.

## 1.4.2 – 2026-10-16

- Notes stay in time while the screen redraws (chords and CC streams no longer stumble).

## 1.4.1 – 2026-10-16

- Each sound its own arpeggiator.

## 1.4 – 2026-10-16

- Two sounds: track B can play its own program; the TRACKS view.

## 1.3.1 – 2026-10-15

- The sequencer coloured by track (A blue, B orange).

## 1.3 – 2026-10-15

- The sequencer on the full screen; patterns switched on the loop's end.

## 1.2.1 – 2026-10-13

- Held function buttons take the knobs on the GLOBAL page too.

## 1.2 – 2026-10-13

- The step sequencer (two tracks, 256 steps, chords, ratchets, chance), the click, USB-MIDI out fixed.

## 1.1 – 2026-10-13

- The first release: 8 voices, two oscillators with sync and cross-mod, 12 / 24 dB filter with the SEM-style morph,
  two envelopes, LFO, arpeggiator, chorus, delay, plate reverb, 32 programs. Do not install it: its USB-MIDI out is
  broken (1.2 fixed it).
