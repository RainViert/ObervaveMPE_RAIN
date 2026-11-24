# Changelog

OberVave for the M-VAVE FM-1 (and, as a beta, the M-Vave SMK-37). Newest first. Each release is on
[GitHub](https://github.com/zvenson/obervave/releases) and on [the Firmware Hub](https://thefirmwarehub.com/firmware/obervave/).

## 3.5-rain44 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- SELECT held + two white keys held together: the chosen track plays from the one bar to the other (keys 3 and 11:
  bars 3-11). The steps outside the window stay as they are; one key alone: bars 1 to it, as rain43. The window is
  kept with the pattern (each track's start bar, in a byte that was unused), recording, lanes and an MPE pattern's
  expression follow it.

## 3.5-rain43 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- SELECT held + a white key: the chosen track's length in bars, the 16 white keys 1-16 bars (16 steps each), on any
  page (in the SEQ, EDIT, GLO layers the keys keep their own work). SELECT's own work on OUTPUT, MPE + CHANNELS and
  GLOBAL is done when it is let go, unless a key set a length with it.

## 3.5-rain42 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- A fifth note onto a step (four at most): the first one goes and the new one is kept (it was left out). The same for
  an MPE pattern's expression per note.

## 3.5-rain41 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- An MPE pattern shows no tracks C and D: the letters at the top go, their rows say MPE PATTERN, and ALGORITHM stops at
  B. Track A cleared (REC held 1.2 s) frees them again.

## 3.5-rain40 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- RECORD page, knob 3 MIDI IN: ALL CH (new, the default): while REC is armed or recording, a note on any MIDI channel
  plays and records on the track recording (track A with MPE: its member channels as before, with their expression).
  OWN CH: as before, each track its own channel.

## 3.5-rain39 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- REC held 1.2 s while recording empties the track (steps, lanes, an MPE pattern's expression) and goes on recording.
- Recording, the screens show the bar the track is in ("2/4") as PLAY does, in red, instead of REC.

## 3.5-rain38 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- A page RECORD under GLO (after MPE + CHANNELS), kept with the settings:
  - REC (knob 1): OVERDUB (as ever) or REPLACE: each step of the recording track the playhead comes to loses its
    notes and recorded expression; what this take plays in stays (the knobs' lanes stay).
  - MPE REC (knob 2): NOTES (rain37: an MPE pattern, each note its own expression) or TRACK (rain36: the expression
    in lanes for the whole track, C and D stay free). A pattern that is an MPE pattern already stays one.

## 3.5-rain37 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- MPE patterns: recording MPE on track A while tracks C and D are empty makes the pattern one ("MPE PATTERN"). Tracks
  C and D's steps then hold A's notes' own expression: bend, pressure and slide per note (up to four on a step), one
  value per step, held until the next. Playing back, each note bends and breathes on its own, as played. Track B stays
  a normal track (drums); the four sounds stay as they are, live and over MIDI.
- In an MPE pattern C and D play nothing and take no steps; choosing them says so. REC held 1.2 s on A, C or D ends
  the MPE pattern (C and D empty tracks again). With C or D in use, the expression goes into A's lanes for the whole
  track (rain36).
- A pattern stays the same size: slots keep their format.

## 3.5-rain36 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- REC records the expression too, into the track's lanes, one value per step (the track's step raster): pitch bend,
  pressure and slide (CC 74) of the MPE note played last (member channels, track A), and pitch bend and channel
  pressure of the main channel. Playing back, the track's notes take them as an MPE note would: the bend over the
  MPE range (MPE off: the sound's BEND range), PRESS > and SLIDE > to their destinations, the matrix's PRESS and SLIDE.
  A lane holds its value until the next one. The four lanes per track are shared with the recorded knobs.
- CC 1 and CC 74 on the main channel were recorded already (as the knobs they move).
- Without such lanes the sound is bit for bit as before.

## 3.5-rain35 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- REC keeps the velocity of a long note: a note held over more than a step and a half was written with velocity 100
  in all its steps, the first one too (the FM-1's keys have none; MIDI and MPE do). Now its ties keep the played one,
  as on the SMK.

## 3.5-rain34 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- REC records MIDI from MPE's member channels (2-16, track A) and from a track's own channel (B-D) too, when that
  track is the one recording: an LinnStrument in MPE mode records like the keys (pitch, velocity, length; the per-note
  slide and pressure are not stored in the steps).
- Includes rain32 (output leveller) and rain33 (kit tuning).

## 3.5-rain33 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- Kits (DRUM KIT 71, SOUND KIT 76): each key can be tuned. Hold the key and turn ALGORITHM: -24..+24 semitones
  ("C TUNE +5"). Per track, kept with the settings and the MEMORY slot (slots from rain31-32 load untuned).
- Includes rain32's output leveller.

## 3.5-rain32 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- A leveller in front of the output stage's tanh: a big chord held at full sustain drove it far into saturation (the
  sum of six voices at their peak, for seconds). Above 0.75 of full scale its gain comes down softly (attack ~0.2 ms,
  release ~190 ms); below, the sound is bit for bit as before.

## 3.5-rain31 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- **Every key of a kit can be set**, in the DRUM KIT (71) and the SOUND KIT (76): hold the key and turn PRESETS: one
  of the ten synthesized drums (KICK ... CYMBAL), any program 1-99 (not a kit), or OFF ("C: 01 DARK ARP", "F: OFF").
  The DRUM KIT starts from its ten synthesized drums, the SOUND KIT from its five programs.
- The kits' keys are the track's and are **kept with the MEMORY slot** (and with the settings), as the matrix; slots
  saved before load with the kits' defaults. (rain30 kept the SOUND KIT's five places in the program: gone, they were
  only five.)
- A program on a key plays at the key's note, a synthesized drum at its own pitch; F# and G# cut A# (the hats).

## 3.5-rain30 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- **SOUND KIT** (program 76, MODE SNDKIT): five places, each any program of 1-99, edited and saved like every sound:
  C (and B) kick, D (and E) snare, D# (and C#) clap, F# (and G#) closed hat, A# open hat, in every octave (General
  MIDI's keys). A drum plays at the key's note (tune it by the octave), as a one-shot; the closed hat cuts the open
  one. **Hold a drum's key and turn PRESETS** to put another program on that place ("SNARE: 74 RIM"); SAVE keeps the
  kit with its places. A program saved meanwhile is heard in the kit at once. The track's PERFORM knobs move all five,
  as on the DRUM KIT. Defaults: 65 KICK, 66 SNARE, 68 CLAP, 67 HIHAT, 72 OPEN HAT.
- New drum programs to build kits from: 72 OPEN HAT, 73 TOM, 74 RIM, 75 CYMBAL (77-99 stay INIT).
- The DRUM KIT (71) stays as it was: ten synthesized drums over the octave.
- Messages ("SNARE: 74 RIM", "2 CORES FROM THE NEXT POWER-ON" ...) show on every page, over the voice LEDs.

## 3.5-rain29 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- The DRUM KIT follows its track's knobs: CUTOFF, RESO, ENV AMT, RELEASE (the decay too: the drums are one-shots) and
  DRIVE move every drum by how far they are from their defaults, so each keeps its own character (CUTOFF up: a
  brighter kick, thinner hats; RELEASE down: a tighter kit). The PERFORM page and the FILTER / AMP ENV pages, the MIDI
  CCs (74, 71, 72) and the matrix's knob lanes work on it.

## 3.5-rain28 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release.

- **DRUM KIT** (program 71, a new MODE: KIT): every key plays its own drum, in every octave, in General MIDI's order
  where it fits: C kick, C# rim, D snare, D# clap, E snare, F low tom, F# closed hat, G mid tom, G# closed hat,
  A high tom, A# open hat, B cymbal. One-shots (a drum sounds out however short the key or the step), the velocity
  sets the level, a closed hat cuts the open one. All made by OberVave's own oscillators, noise and filters; a drum
  costs what a voice costs, and only the drums that sound are worked out. Plays from the keys, the sequencer and MIDI
  (a track's channel: a drum machine on one track).

## 3.5-rain27 – 2026-11-24

A personal variant (RainViert/ObervaveMPE_RAIN), not an official OberVave release. A diagnosis build.

- rain26: S15 (the start hung waiting for the second core's answer), then 1C-2.0 (no answer, it never ran): not
  the screen or the inputs. rain27 waits 3 s before the start (as Clipper, whose second core runs on that unit,
  after its splash) and shows the two supplies' codes on the CHANNELS page (U7F = VDC14 1.60 V, SYSVDD 1.38 V).

## 3.5-rain26 – 2026-11-24

A personal variant (RainViert/Obervave-MPE_RAIN), not an official OberVave release. A diagnosis build.

- rain25 on the unit in question: S1, R04 (the watchdog): the FM-1 froze as soon as the second core was started.
  rain26 starts it earlier (before the screen, the inputs and the ADC) and marks every step of the start (S11 entry
  word, S12 clock, S13 run bit, S14 release, S15 waiting, S16 answered or not, S17 done), so the page shows where it froze.

## 3.5-rain25 – 2026-11-24

A personal variant (RainViert/obervave), not an official OberVave release. A diagnosis build for the second core.

- The second core's start goes in steps, each kept through a reset: it answered, a job in RAM, the self-test in
  flash, the effects running (and how many blocks it did). The CHANNELS page shows the step it got to (S) and the
  blocks (J) after a failure.
- A reset while the second core runs (not a power-on) starts the next boot on one core and switches it off: no
  second crash in a row, so no rescue mode (rain24 crashed twice). A real power-on clears every old mark.
- 1C-5: a reset while it ran; 1C-6: a job in RAM came back wrong.

## 3.5-rain24 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- The CHANNELS page's diagnosis gets a second line: the last crash record (PC, vector, C1 when core 1 faulted), the
  chip's reset reason and the tries. rain23 reset silently (no crash screen) while starting the second core.

## 3.5-rain23 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- The mark "the second core's start went wrong" is read once and cleared, and a start that only got no answer
  leaves none: the RAM kept rain21's through the update and a short power-off, so rain22 said 1C-1 (crashed
  before) without trying.

## 3.5-rain22 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- The second core's first instructions run from RAM, not the flash (on the unit where it never answered, 1C-2.0,
  it did not even reach them; Clipper's second core runs on that unit).
- When the second core failed, the CHANNELS page shows why in its lower half: the reason, the bus guard's flags
  during the start and how far the core got, for the diagnosis.

## 3.5-rain21 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **The second core's start as Clipper's.** On the FM-1 where rain19 said 1C-2 (core 1 never answered), Clipper
  RAIN starts its second core fine: Clipper starts it seconds after the supplies are raised, OberVave did a few
  milliseconds after. Now the supplies get 300 ms to settle first, and the start is tried up to three times.
- 1C-2.N on the CHANNELS page: how far core 1 got (0 it never ran, 1 its entry, 2 its loop).

## 3.5-rain20 – 2026-11-23

The same as rain19 (released before rain21's change was in).

## 3.5-rain19 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- After the second core failed, SELECT on the CHANNELS page can switch it on again at once (rain18 turned it off
  again until the next power-on). The page says why it failed: 1C-1 its start crashed before, 1C-2 it never
  answered, 1C-3 its self-test came back wrong, 1C-4 it was late; the message at the start says it in words.

## 3.5-rain18 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

rain17 crashed while starting the second core on the FM-1 it was made for, and the crash screen then stayed for half
an hour instead of restarting. rain18 makes that impossible to get stuck in:

- **One core by default.** Everything plays as in rain16, bit for bit. The second core is a choice: on the MPE +
  CHANNELS page, SELECT switches 1C / 2C, from the next power-on (the page's title shows it).
- **A failed second core switches itself off for good.** If its start crashes, or it answers wrong or late, the next
  start is on one core and the setting is saved as off ("2 CORES OFF: CORE 2 FAILED").
- **OCT+ held at power-on**: one core this time, whatever the setting.
- **The crash screen always ends in a restart**, within seconds: its wait no longer trusts a timer the second core's
  start can stop, the second core is held first, and the reset has the watchdog behind it.
- The bus traps paused around the second core's start (17.1), built without the constant the build check refused.

## 3.5-rain17 – 2026-11-23 (withdrawn: crashed at power-on on two cores)

A personal variant (RainViert/obervave), not an official OberVave release.

- **The effects on the FM-1's second core.** Chorus, both delays and the reverb run on core 1, one block (5.8 ms)
  behind the voices on core 0, beside them instead of after them: core 0 keeps only the voices, the mixer, DRIVE
  and MASTER. The delays' times and the chorus' base are shortened by that block, so they sound as before; the
  reverb's pre-delay is 5.8 ms longer. GLOBAL's title says 2C (two cores) or 1C.
- The second core is the one Melodee (Kerem Kilic) found, through X0X (Charles Vestal), SLOOP (Andre Rezemini's
  dualcore branch, 2.8.3) and Clipper: hal/fm1_cpu1.h / .S, the supplies raised to M-VAVE's own levels at power-on,
  core 1 held around resets and faults. Started before the audio and proved with a self-test job; a unit whose
  core 1 does not answer, answers wrong or once runs late plays on one core until power-off, the effects back on
  core 0 exactly as in rain16 (bit for bit).

## 3.5-rain16 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- The mixer: a track's level and pan as one gain in 32 bits (two 64-bit products a sample and channel before, slow on
  the FM-1's 32-bit core); a track that sends into no effect skips the five send sums. The sound differs by 3 LSB at
  most.

## 3.5-rain15 – 2026-11-23

A personal variant (RainViert/obervave), not an official OberVave release.

- **RATE 1/16** for both delays (DELAY +, DELAY 2 +): the line at 2.76 kHz, up to 11.9 s (ping-pong and reverse:
  5.9 s), very dark, the bandwidth about 1.4 kHz, as a worn tape.

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
