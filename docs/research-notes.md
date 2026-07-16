# Aureate Deep-Dive Research Notes (v2 brief prep)

Category: tape/console saturation "glue" for orchestral bus material. Reference class assembled from
primary/technical sources on (a) analog tape hysteresis + transport artefacts, (b) single-ended
valve/tube saturation, (c) analog console/transformer summing-bus saturation, (d) console tilt EQ
topology, (e) glue-compression workflow lore (for calibration/level context, not for a compressor
feature). No hardware was measured directly — everything below is literature/manual/forum-sourced.

---

## 1. Tape hysteresis: symmetric, odd-harmonic-dominant (the central correction)

- "Tape's magnetic hysteresis is roughly symmetric, with the magnetization curve looking about the
  same in both directions, so tape saturation emphasizes odd harmonics." — search synthesis over
  KERN Audio's saturation guide, https://kernaudio.io/guides/saturation/harmonic-distortion-explained
- Physical modelling literature confirms: the Jiles-Atherton hysteresis model is the standard for
  real-time tape sim (used in `chowtape`/`AnalogTapeModel` by Jatin Chowdhury). "Simulations with the
  J-A model starting with zero residual magnetization show the presence of odd-number harmonics
  only." DAFx 2019 paper, "Real-time Physical Modelling for Analog Tape Machines":
  https://www.dafx.de/paper-archive/2019/DAFx2019_paper_3.pdf (also mirrored at
  https://ccrma.stanford.edu/~jatin/420/tape/TapeModel_DAFx.pdf). Model parameters: M_s (saturation
  magnetization), α (coupling), δ_M/δ_k, c (weighting), a (Langevin scaling) — see
  https://github.com/jatinchowdhury18/AnalogTapeModel/blob/master/Notes/Digitizing%20Hysteresis.md
- **Implication**: a tanh(x+bias)-tanh(bias) curve pushed to strong asymmetry is NOT what "tape" sounds
  like at the hysteresis-physics level — it's closer to what single-ended valve stages do (see §2).
  Real tape bias is an ultrasonic (100kHz+) erase/record signal, not an audio-band DC offset; its
  audible effect is on self-erasure/HF loss (§3), not on odd/even balance.

## 2. Valve/single-ended saturation: the actual home of asymmetric/even-harmonic character

- "Asymmetrical circuits like a single-ended triode produce many even-order harmonics… single-ended
  valve stages generate increasing levels of predominantly second harmonic distortion as the level is
  increased, and with sufficient level, such amplifiers will almost invariably clip asymmetrically."
  — search synthesis over sound-au.com "Valves - Distortion+Intermod",
  https://sound-au.com/valves/thd-imd.html
- "The tube conducts more easily in one direction than the other, creating an asymmetric transfer
  function that generates even harmonics." "A sound rich in second harmonic… can be characterized as
  warm and velvety… the single-ended distortion stays 'glued' to the note, giving it notable
  'fatness.'" — search synthesis, multiple sources incl.
  https://midi-audio-expert.com/vacuum-tube-audio-explained-for-music-producers-and-mixers/
- **Implication**: asymmetric-bias saturation as the plugin's primary "warmth" mechanism is authentic
  — but it is a *valve* archetype, not a *tape* one. Naming/behavior should reflect that distinction
  per Character model rather than applying identical asymmetric-shift math to all three curves.

## 3. Tape self-erasure / HF rolloff and head bump — genuinely tape-specific, currently under-modelled

- "Self-erasure is a high-frequency loss that occurs at the record head as the magnetized tape leaves
  the gap area… The record equalizer counteracts the effects of self-erasure at high audio
  frequencies… a higher bias frequency keeps its recorded wavelength much shorter than any audio
  signal, minimizing this self-erasure effect on treble." — search synthesis,
  https://ccrma.stanford.edu/~jay/subpages/Lectures/Lecture7-Magnetic_recording.pdf and
  https://www.tangible-technology.com/media/media_2.html
- Speed-dependence: "For each doubling of speed the head bump and the rolloff moves an octave up…
  15ips serves music with deep notes far better than 30ips… one running at 7.5 ips has [head bump]
  lower, extending the frequency response often lower than one running at 15 ips." — Jack Endino,
  "Response Curves of Analog Recorders," https://www.endino.com/graphs/
- **Implication**: real tape HF rolloff is genuinely a *tape* mechanism (self-erasure), so Aureate's
  "Warmth drives an HF low-pass" idea is directionally right — but v1 has no corresponding LF head
  bump (a resonant low-frequency bump/rolloff, not modelled at all) and no speed/character coupling
  (real machines don't let you dial the rolloff continuously the way Warmth does; it's discretely tied
  to tape speed/formulation).

## 4. Console/transformer summing-bus saturation: blended harmonics, soft (not hard) knee

- "A perfectly symmetric transformer core would produce only odd harmonics. However, transformer
  cores produce both even and odd harmonics due to a combination of symmetric hysteresis and
  asymmetric core saturation. This blended harmonic profile is what gives transformers their
  characteristic sound." — search synthesis, https://kernaudio.io/guides/saturation/tube-tape-transformer
- "The console bus resistor network creates the first layer of harmonics as it approaches saturation,
  predominantly 2nd-order content at moderate levels… Unlike a DAW, the analog bus doesn't hit a hard
  ceiling at 0 dBFS; it enters a soft saturation zone." — search synthesis,
  https://vintagemaker.net/analogue-warmth-analog-summing-harmonics/
- **Implication**: v1's "Console" character model (`cubicSoftClip`, hard-flat beyond ±1) is the
  *least* accurate of the three against its own reference — real console/transformer bus saturation is
  a soft, blended even+odd knee, not a harder-edged clip than tape's tanh. The current three-model
  hierarchy (Tape=smooth tanh, Console=harder cubic, Valve=exponential) inverts what the reference
  class suggests: Console should be the gentlest/most linear at low-to-moderate drive (it's designed
  not to be heard until pushed), Valve the most aggressively asymmetric, Tape the most odd-harmonic
  dominant with only mild, physically-motivated asymmetry.

## 5. Wow & flutter: standard rate bands and hardware specs

- "Wow specifically refers to low-frequency speed fluctuations, typically in the range of 0.5 Hz to 6
  Hz… flutter denotes higher-frequency variations, generally from 6 Hz to 100 Hz." — search synthesis,
  https://www.akustik-messen.de/index.php/en/background-information/hi-fi/wow-and-flutter and
  https://grokipedia.com/page/Wow_and_flutter_measurement
- Studer A800 spec: "wobble values (peak weighted) specification is 0.06 max at 7.5 IPS, and 0.04 max
  at other speeds, according to DIN 45507 and IEC 368." — search synthesis over
  https://help.uaudio.com/hc/en-us/articles/4419513099156-Studer-A800-Multichannel-Tape-Recorder-Manual
  and https://www.uaudio.com/products/studer-a800-tape-recorder
- **Implication**: v1's wow rate (0.7 Hz) sits correctly inside the 0.5–6 Hz wow band. v1's flutter
  rate (6.5 Hz) sits at the very bottom edge of the 6–100 Hz flutter band — most musical tape
  emulations bias flutter noticeably higher (double-digit Hz) to keep the two bands perceptually
  distinct (slow pitch "breathing" vs. faster "shimmer"); parking flutter at 6.5 Hz risks it reading
  as a second wow rather than a distinct flutter character.

## 5b. Wow/flutter depth is program-dependent in practice, not a single fixed max

- Multiple current tape-emulation plugins expose wow and flutter as *separate* depth/rate controls
  (not a single joint "amount"), and several pair flutter with scrape-flutter/modulation noise as a
  related but distinct artefact — search synthesis across
  https://babyaud.io/blog/wow-and-flutter and https://manuals.goodhertz.com/3.13/wow-ctrl/.
- **Implication**: v1's single "Wow/Flutter" knob scaling both depths in lockstep from one proportion
  is a simplification versus the reference class's tendency to give wow and flutter (and sometimes a
  hiss/noise coupling) independent controls, or at least independently-tunable rate/depth pairs.

## 6. Tape hiss: high-frequency-weighted noise, not a darkened noise floor

- "Tape machines have a byproduct of their mechanism in the form of tape hiss, a high frequency white
  noise... the frequency spectrum varies depending on the tape formulation and speed" — search
  synthesis, https://theproaudiofiles.com/tape-machine-plugins/ and general survey via
  https://www.kvraudio.com/forum/viewtopic.php?t=207214.
- Reference plugins expose hiss as a simple, audible top-end-forward layer: "Waves Abbey Road J37...
  includes a NOISE level pot that adds modelled tape hiss." "Bad Tape... includes a HISS parameter
  that lets you add a tape hiss like white noise signal." — search synthesis,
  https://www.wavespluginreviews.com/waves-harmonics-audio-plugins/waves-abbey-road-j37-tape-machine-emulation-analogue-modelled-plugin-in-depth-review/
  and https://www.deniseaudio.com/plugins/bad-tape
- **Implication**: v1's Hiss is deliberately routed *before* the downsampling filter so it "inherits a
  natural top-end softening" — this is the opposite spectral tilt from what real/reference tape hiss
  is (HF-forward, hissy, closer to white/pink noise with an audible top end), not a darkened/rolled-off
  noise floor. This is a clear, fixable v1 mischaracterization.

## 7. Console tilt EQ: single pivot frequency, not independent shelf corners

- "A tilt EQ consists of both a low shelf and high shelf joined at a single corner frequency (in this
  case 700Hz), offering a smooth, single control for adjusting the overall tonality of a source." —
  search synthesis, GroupDIY tilt EQ threads, https://groupdiy.com/threads/tilt-eq.15732/ and
  https://groupdiy.com/threads/tilting-equalizer-design.57582/
- "The Phoenix Audio Pivot Tone Channel (API module) uses a unique 3-position Tilt EQ with 3 carefully
  selected tilt positions... dark/bright control to tilt the EQ." —
  https://vintageking.com/phoenix-audio-tilt-tc-500-eq-api-module
- Shelf-frequency labelling caveat: "the frequency that is quoted for shelving EQs is not the ±3dB
  point; it's the ±15 or 18 dB point, and each manufacturer/designer has his own definition." —
  GroupDIY, https://groupdiy.com/threads/shelving-equalizer-and-frequencies.88222/
- **Implication**: canonical tilt EQs pivot around a *single* frequency (commonly cited ~700 Hz–1 kHz
  in the wild) where response is flat, with the low and high shelves mirroring each other around it.
  v1 instead uses two independently-parameterized shelves at fixed but *different* corners (200 Hz /
  4000 Hz) that both move in the Tone direction — closer to a "dual shelf" EQ than a textbook tilt.
  This is defensible as a deliberate design choice (it does what v1's docs claim: separates Tone from
  Trim), but it should be described honestly as a dual-shelf tilt approximation, not "console-style
  tilt EQ" without qualification, since real tilt topology is single-pivot.

## 8. Gain-staging / calibration reference point

- "In tape emulation contexts, −18 dBFS RMS reads around 0 VU on professional tape machines, serving
  as a reference calibration point for analog tape simulation plugins." — search synthesis across
  general tape-calibration discussion (multiple plugin-manual sources).
- **Implication**: useful anchor for factory presets and for documenting what Drive/Warmth defaults
  are "aimed at" — i.e. presets should be described relative to a -18 dBFS-ish nominal signal, not an
  arbitrary "sounds good at 0 dBFS" assumption.

## 9. Glue-compression workflow lore (context only — NOT a feature to add)

- SSL G-bus glue compressor conventions: attack 10–30 ms, ratio 2:1–4:1, "aim for just 2 to 3 dB of
  gain reduction on your loudest sections," release often Auto/program-dependent. — search synthesis,
  https://www.nailthemix.com/andrew-wade-master-bus-compression and
  https://recordmixandmaster.com/2025-10-the-magic-of-the-ssl-g-series-bus-compressor
- Parallel/New-York bus-processing convention: aggressive settings blended back at 40–60% wet is
  explicitly called out as useful "for orchestral material where you want cohesion without losing
  natural dynamics." — search synthesis, same sources.
- **Implication**: Aureate is explicitly NOT a compressor (no gain-reduction stage) — this is included
  only to confirm the *workflow* framing already in `docs/manual.md` ("run after balancing, before the
  mix bus") matches how engineers actually deploy glue processors, and to validate the Mix parameter's
  parallel-blend design (40–100% wet is the sane working range for "glue" use, matching the SSL-glue
  parallel convention) rather than to justify adding a compressor stage.

---

## Summary: v1 gap list (feeds directly into the v2 brief's "why v1 falls short" section)

1. **Character models are not measurably distinct against their own reference archetypes.** All three
   apply the identical shift-then-recentre asymmetric-bias trick to three curve shapes; the reference
   class says Tape should be the most symmetric/odd-dominant, Valve the most asymmetric/even-dominant,
   Console a soft (not hard) blended-harmonic knee. v1's actual ordering (Console=hardest-edged) is
   close to backwards.
2. **"Warmth = tape bias" is a mislabelled mechanism.** Audio-band asymmetric bias saturation is a
   valve/single-ended archetype, not a tape-hysteresis one; real tape bias is ultrasonic and its
   audible fingerprint is HF self-erasure loss, which v1 already does correctly via the low-pass half
   of Warmth — the asymmetry half is misattributed.
3. **No head bump (LF resonance).** Real tape has a speed-dependent low-frequency bump from head
   geometry; v1 only rolls off highs, never adds/shapes a low-frequency bump.
4. **Hiss has the wrong spectral tilt.** Real tape hiss is HF-forward; v1's hiss is generated pre-
   downsample specifically to inherit a darkening filter, i.e. the opposite tilt.
5. **Wow/flutter rates sit at the boundary, not centered in, their bands**, and are coupled into one
   knob where the reference class more often exposes independent wow/flutter depth or rate controls.
6. **"Console-style tilt EQ" is technically a dual independent-corner shelf, not a single-pivot tilt** —
   fine as a design choice, needs honest labelling.
7. **No documented calibration anchor** (e.g. -18 dBFS nominal) tying Drive/Warmth defaults or future
   presets to a real gain-staging convention.
