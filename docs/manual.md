<p align="center"><img src="assets/icon.png" alt="Aureate icon" width="120"/></p>

# Aureate User Manual

## What Aureate is

Aureate is a tape/console saturation "glue" plugin for orchestral material - strings, brass, and layered/bussed tracks that need cohesion and a little analog warmth without sounding like a guitar pedal. It combines a 4x oversampled, character-selectable saturator (tanh-based tape, soft-knee console, or exponential valve) with tape-transport artefacts (independent Wow/Flutter, an LF head bump, HF-forward Hiss) and a dual-shelf tilt-style Tone control plus independent HF/LF trim shelves.

**v0.2.0** is a research-derived revision of the saturation core - see `docs/design-brief.md` for the full brief (what changed and why) and `docs/research-notes.md` for its sourced citations. Nothing here is calibrated against measured hardware; every default is either carried over from v0.1 or chosen to sit inside a sourced band/ordering from the literature - see the brief's own Honesty section.

## Where it sits in a heavy production chain

Aureate is designed to run **after** the individual layers of an orchestral/choral stack have been balanced (strings, brass, choir, etc.), typically on:

- A **string or brass bus**, to glue a divisi section together the way tape/console summing naturally does - a little Drive and Warmth, modest Mix, and the section reads as one instrument rather than a pile of close mics.
- An **orchestral/choir submix bus**, sitting after the individual sections and before the final mix bus, to add cohesion before the orchestral material meets the metal instrumentation (guitars, drums, bass).
- The **mix bus** itself (sparingly - low Drive, Mix well under 100%), as a subtle "master glue" pass, in the same role a tape machine or console summing bus would play in a hybrid analog/ITB workflow.

It is not a distortion or amp-sim plugin (that role belongs to `overture`/`tenebrae` elsewhere in the suite) - Drive is deliberately capped at a modest 24 dB and the default Warmth/Character settings stay well inside "adds richness," not "adds grit."

## Signal flow

```
input -> Wow/Flutter (independent wow + flutter) -> Drive
      -> [4x oversampled: Warmth HF-rolloff -> LF head bump -> saturator (Character:
         Tape/Console/Valve, Warmth+Bias-driven asymmetry) -> Tone tilt -> HF/LF Trim
         -> Hiss (HF-forward)]
      -> downsample -> Dry/Wet Mix -> Output trim -> output
```

Wow/Flutter and Drive run at the host sample rate; everything from the Warmth low-pass through Hiss runs inside the 4x oversampled domain, so the saturator's harmonics (and Hiss's noise) are generated and filtered at 4x the host rate before a single downsample step. Mix blends the processed ("wet") signal back with a delay-compensated copy of the untouched input, and Output is a final trim applied to the combined result. See [`docs/architecture.md`](architecture.md) for the full technical breakdown, including latency accounting and real-time-safety notes.

## Gain-staging note

Drive/Warmth's defaults are tuned assuming a nominal **-18 dBFS RMS** input level (the widely-cited tape "0 VU" calibration convention) - not a measurement of anything Aureate-specific, just a documented assumption. This explains why the default 6 dB of Drive feels quite different on a hot, limited digital bus versus a conservatively gain-staged one, and it's the anchor the factory presets use for Drive/Output as a matched pair.

## Parameter reference

| Parameter | Range | Default | Unit | What it does |
|---|---|---|---|---|
| **Wow** | 0-100 | 0 | % | Amount of slow tape-transport pitch drift (~0.7 Hz), applied via a modulated delay ahead of Drive. 0% is a fixed (non-modulated) delay - a true off state, not "very little." Independent of Flutter (v0.2.0) - use Wow alone for a slow "breathing" pitch instability without any faster shimmer. |
| **Flutter** | 0-100 | 0 | % | Amount of faster tape-transport pitch shimmer (~11 Hz), applied via the same modulated delay. Independent of Wow (v0.2.0) - use Flutter alone for a faster "wobble"/shimmer character without any slow drift. Use Wow and Flutter together, sparingly (10-25% each), for a classic vintage-tape character on sustained pads/strings; higher settings are an obvious, deliberate effect. |
| **Drive** | 0-24 | 6 | dB | Gain into the saturator. Kept modest by design - Aureate is a glue processor, not a distortion pedal. Higher settings push the Character-selected curve harder, adding more harmonic content and compression. |
| **Warmth** | 0-100 | 35 | % | Controls three things from one knob, all Character-dependent in strength: the saturator's asymmetry bias (single-ended, tape-like character - the bias ceiling differs per Character, see below), a gentle pre-clip high-frequency rolloff (tape self-erasure/bias-oscillator darkening), and a gentle LF head-bump resonance around 80 Hz (tape-transport head geometry, up to +1.5 dB). Higher Warmth = more asymmetric saturation, a darker top end, and a touch more low-end weight - the single most important "character" knob on the plugin. |
| **Bias** | -100 to 100 | 0 | % | An additional, independent saturator asymmetry trim, added on top of Warmth's own bias contribution. Use this to skew the asymmetry further (or in the opposite direction) without touching Warmth's HF-rolloff/head-bump amount - useful for dialing in odd-vs-even harmonic balance to taste. |
| **Character** | Tape / Console / Valve | Tape | - | Selects the saturator's transfer-function family, each with a genuinely distinct harmonic-balance profile. **Tape**: smooth asymmetric tanh, the most odd-harmonic-dominant and least asymmetric of the three (Warmth's bias ceiling is lowest here) - the classic, forgiving tape-glue sound. **Console**: an asymmetric soft-knee curve (v0.2.0) that stays transparent at low-to-moderate drive, only showing character once pushed hard - the "least characterful until pushed" archetype, closer to a solid-state/transformer summing bus. **Valve**: an asymmetric exponential saturation curve, the most asymmetric/even-harmonic-forward of the three (Warmth's bias ceiling is highest here) - a rounder, tube-like push. |
| **Tone** | -100 to 100 | 0 | % | A dual-shelf tilt-style EQ (two independent shelf corners, not a textbook single-pivot tilt): negative darkens (low shelf up, high shelf down), positive brightens (the inverse), 0% is flat/unity. Use this for the broad tonal balance of the whole processed signal. |
| **HF Trim** | -6 to 6 | 0 | dB | A fixed-frequency (8 kHz) high-shelf trim, independent of Tone - a finer top-end adjustment (add air or tame harshness) after the broader Tone tilt has set the overall balance. |
| **LF Trim** | -6 to 6 | 0 | dB | A fixed-frequency (150 Hz) low-shelf trim, independent of Tone - a finer low-end adjustment (add weight or tighten up mud) after the broader Tone tilt. |
| **Hiss** | 0-100 | 0 | % | Amount of shaped noise ("tape hiss") mixed into the processed signal, generated inside the oversampled domain and shaped by a dedicated high-frequency-forward shelf (v0.2.0) so it reads as broadband hiss, not muffled static. 0% is genuinely silent (no noise floor at all) - a deliberate "vintage" option for material that should sound like it came off a tape machine, not a mixing-desk artefact to leave on by default. |
| **Mix** | 0-100 | 100 | % | Dry/wet blend. At 0% the plugin is a sample-accurate (latency-compensated) passthrough of the input - useful for parallel/New-York-style blending, or for confirming Aureate isn't colouring a signal when you want to A/B it out. |
| **Output** | -24 to 24 | 0 | dB | Final output trim, applied *after* the dry/wet mix - unlike Drive (which only affects the wet path), Output scales the combined dry+wet signal as a whole. Use it to compensate for level changes introduced by Drive/Warmth/Character before the signal moves further down the chain. |

## Presets

The preset bar at the top of the editor lets you browse Factory and User presets (`<` / preset name / `>` to step through, click the name for the full menu), Save/Save As/Delete user presets, Import/Export single presets or bank zips, and set the current state as your own startup default. Eleven factory presets ship in v0.2.0 - see `docs/presets.md` for what each one is for. Presets are stored per-user at `~/Library/Audio/Presets/Yves Vogl/Aureate/` on macOS (`%APPDATA%/Yves Vogl/Aureate/Presets/` on Windows).

## Upgrading from v0.1.x

v0.2.0 makes two breaking changes to saved automation/state, both explicitly allowed pre-1.0: the single "Wow/Flutter" parameter is now two independent parameters (Wow/Flutter), and each Character model's Warmth-driven bias ceiling changed (Tape and Console are now less asymmetric than before at the same Warmth/Bias settings; Valve is unchanged). A v0.1.0-saved session still loads without error - its old Wow/Flutter value is copied onto both new Wow and Flutter parameters - but it will sound different after upgrading if you were using Warmth/Bias with Tape or Console. This is an audible, deliberate voicing correction (see `docs/design-brief.md`), not a bug.

## Tips

- **Start with Character before reaching for Drive.** The three models are genuinely distinct even at matched settings - Tape is the most forgiving, odd-harmonic-dominant glue sound; Console stays transparent until pushed hard, then blends in; Valve is the most aggressively asymmetric, even-harmonic-forward push. Pick the character first, then dial in Drive/Warmth to taste.
- **Warmth does triple duty.** It jointly drives the saturator's Character-dependent asymmetry, the HF-rolloff, and the LF head bump together, so a single Warmth move can sound like "more tape" without any other control changing - this is the fastest way to explore the plugin's core character. If you want the darkening/weight effect without more saturation asymmetry, reach for the separate LF/HF Trim shelves instead, or offset the asymmetry back out with a negative Bias.
- **Bias is for fine-tuning, not the main event.** Leave it at 0% until Warmth (and Character) have you most of the way there, then nudge Bias if you want a touch more (or less, or reversed) asymmetric character without moving the HF-rolloff/head-bump.
- **Wow, Flutter, and Hiss are all off by default on purpose.** They're deliberate "worn tape" effects for material that wants to sound like it came off a physical machine (a mellotron-style string patch, a vintage-tape-emulation pass on a whole mix) - most orchestral glue use cases should leave all three at 0% and only reach for them when that specific character is the goal. Wow and Flutter are independent as of v0.2.0, so you can dial in a slow drift without any faster shimmer, or vice versa.
- **Use Mix for parallel processing.** Because Mix is sample-accurately delay-compensated, you can blend a heavily driven, characterful wet signal back under the clean dry signal (New-York-style parallel saturation) without any phase smearing from the oversampling latency.
- **HF/LF Trim vs. Tone.** Tone's tilt shelves move the whole spectral balance in one gesture; HF/LF Trim are independent, so you can, for example, brighten with Tone and then pull a little top end back with a negative HF Trim if Tone's brightening also over-emphasizes something specific near 8 kHz.
- **Use Output to gain-match.** Since Drive/Warmth/Character all change the wet signal's level and perceived loudness, use Output to bring the processed signal back to roughly the same loudness as the dry input before comparing them (avoids the "louder always sounds better" trap when A/B-ing Mix or bypass).
