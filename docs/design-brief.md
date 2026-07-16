# Aureate Design Brief v2 (draft) — target v0.2.0

Status: draft for review. Read alongside `aureate-research-notes.md` (sourced findings this brief
draws on). No brand or hardware/person names appear in any proposed parameter or UI naming below —
descriptors are generic ("tape-style", "single-ended", "console-style"), matching v1's existing
convention. Research citations live only in the notes file.

---

## 1. Why v1 falls short

v1 (M1, shipped as v0.1.0) is a competently *engineered* saturation core — real-time-safe, fully
tested, oversampled, latency-compensated — but it is not yet *authentically voiced* against the
reference class of tape/valve/console glue processing it claims to model. Specifically:

1. **The three "Character" models aren't measurably distinct against their own real-world archetypes.**
   Tape, Console, and Valve all route through the identical `f(x+bias) - f(bias)` asymmetric-shift
   construction, differing only in the underlying odd-function `f`. The reference class says these
   three archetypes should differ in *harmonic order balance*, not just clip-curve shape: tape
   hysteresis is close to symmetric and odd-harmonic-dominant; single-ended valve stages are the
   genuinely asymmetric, even-harmonic-forward archetype; console/transformer summing blends both,
   softly. v1's actual behavior (Console = hardest-edged clip of the three) inverts that ordering.
2. **"Warmth" mislabels its own asymmetry mechanism as tape behavior.** Audio-band DC-bias asymmetric
   saturation is authentically a valve/single-ended mechanism. Real tape bias is an ultrasonic
   record/erase signal; its only audible fingerprint is HF self-erasure loss — which v1's Warmth
   already models correctly via its low-pass half. The bias half of Warmth is doing valve work under a
   tape label.
3. **No low-frequency head-bump modelling.** Tape transports produce a speed-dependent LF resonance;
   v1 only shapes the top end.
4. **Hiss has the wrong spectral tilt.** Real tape hiss is HF-forward broadband noise; v1 deliberately
   generates Hiss pre-downsample so it inherits a *darkening* filter — audibly backwards from the
   reference.
5. **Wow/Flutter's two rates are coupled to one knob and sit at/near the edges of their canonical
   bands** rather than being independently controllable and centered for perceptual separation.
6. **"Console-style tilt EQ" is a dual independent-corner shelf pair, not a textbook single-pivot
   tilt.** Defensible as a design choice, but currently over-claims its own topology.
7. **No documented gain-staging anchor.** Nothing ties Drive/Warmth defaults (or the upcoming M2
   presets) to a real calibration convention, so defaults read as "tuned by ear" rather than
   deliberately chosen.

v2's job is to fix (1)-(4) as measurable DSP/test changes, be honest about (5)-(6) as scoped
simplifications, and add (7) as a documented convention feeding the M2 preset system.

---

## 2. Topology (v2)

```
input -> Wow/Flutter (independent wow/flutter rate+depth, modulated delay) -> Drive
      -> [4x oversampled:
            Warmth HF-rolloff (tape self-erasure, low-pass)
         -> LF head-bump shelf/peak (tape transport resonance, new)
         -> Character-selected saturator (Tape/Console/Valve, each with its own
            harmonic-balance profile - see 3.3)
         -> Tone tilt (unchanged topology, honestly documented as dual-shelf)
         -> HF/LF Trim
         -> Hiss (HF-forward noise, now generated post-downsample or with a
            complementary HF-emphasis shaping, see 3.5)
         ]
      -> downsample -> Dry/Wet Mix -> Output trim -> output
```

Only one topology-order change from v1: Hiss moves out of (or is re-shaped within) the pre-downsample
position so its spectral tilt stops being backwards (§3.5). Everything else keeps v1's proven
oversampling/latency-compensation/dry-wet-mix architecture — that machinery is solid and not in scope
for this brief.

---

## 3. Module specs

### 3.1 Drive (unchanged mechanism, defaults re-confirmed)

- Range 0–24 dB, default **6 dB**. Kept from v1: a glue processor's useful range genuinely sits well
  under distortion-pedal gain levels; nothing in the research contradicts this, and 24 dB already
  comfortably covers "obviously driven" tape/console behavior at oversampled 4x. **No change.**

### 3.2 Warmth — split into an honestly-tape mechanism (HF rolloff) and keep its bias contribution, but re-anchor the bias story to Character

- HF rolloff: 20 kHz → 3 kHz, default proportion **35%** — kept from v1; this *is* the tape-authentic
  half of Warmth (self-erasure loss), and the mapping range plausibly covers 30ips-bright to
  15ips/7.5ips-dark tape response without needing a discrete speed selector (scope decision, see
  Honesty §6).
- Bias contribution: kept as a joint control (Warmth still nudges asymmetry) **but its default
  strength is now Character-dependent** rather than a single fixed `warmthMaxBias = 0.3` for all three
  models (§3.3). Reasoning: the research shows asymmetry is *the* valve mechanism, a secondary
  tape mechanism, and a blended-but-present console mechanism — so making Warmth's asymmetry
  contribution scale per-Character is how "Warmth means something different per Character" becomes
  literally true rather than only true via the different `f`.
- **New: LF head-bump.** A resonant low-shelf/peaking bump, default center **80 Hz**, default gain at
  100% Warmth **+1.5 dB** (Q ≈ 0.9, gentle), scaling from 0 dB at 0% Warmth. Sourced reasoning: head
  bump is a well-documented, speed-dependent tape-transport resonance (see notes §3); a fixed
  ~80 Hz center is a reasonable single-machine approximation given Aureate doesn't expose a discrete
  tape-speed control (Honesty §6 covers this explicitly as a simplification, not a measured curve).

### 3.3 Character — three archetypes with genuinely distinct harmonic-balance profiles

Keep the three-model enum and the existing shift-then-recentre-by-bias construction (it's a sound,
DC-safe technique) but change what each model's bias/curve combination is tuned to represent:

| Model | Curve family (unchanged core math) | New default asymmetry strength (replaces the shared `warmthMaxBias=0.3`) | Reasoning |
|---|---|---|---|
| **Tape** | `tanh(x+bias) - tanh(bias)` | Reduced max bias contribution: **0.12** at 100% Warmth (was 0.3 shared) | Real hysteresis is close to symmetric/odd-dominant (research notes §1); Tape should be the *least* asymmetric of the three, with its character coming mostly from the tanh curve's odd-harmonic compression plus the HF-rolloff/head-bump, not from strong asymmetry. |
| **Console** | Replace hard-flat `cubicSoftClip` with a **softer**, blended-harmonic curve: a scaled `tanh`-family or rational soft-knee that stays closer to linear until well past unity, with a modest, non-zero default asymmetry | Moderate: **0.10** at 100% Warmth | Research notes §4: real summing-bus/transformer saturation is soft-kneed and blends even+odd, not the hardest-clipping of the three. Console should be the most transparent at low-to-moderate Drive, only showing character once pushed — the opposite ordering from v1. |
| **Valve** | `exponentialSoftClip` (unchanged) | Increased max bias contribution: **0.30** at 100% Warmth (kept at v1's old shared max, now valve-exclusive) | Research notes §2: single-ended valve stages are *the* authentically-asymmetric, even-harmonic-forward archetype. Valve keeps v1's strongest asymmetry setting because that's where the reference class actually puts it. |

`maxExplicitBias` (Bias parameter's own trim) and `maxCombinedBias` clamp stay as engineering safety
rails (0.3 / 0.9) — these are implementation guardrails, not tuned-to-reference values, and are
correctly documented as such already.

### 3.4 Tone / HF / LF Trim (topology honesty only, no parameter change)

No range/default changes. Documentation (README, manual, in-code comments) is updated to describe Tone
as a **dual independent-corner tilt-style EQ** rather than unqualified "console-style tilt EQ," since
research notes §7 show canonical tilt EQs pivot around a single frequency. This is a docs-only change;
the 200 Hz/4000 Hz shelf pair and ±6 dB range are kept as-is — they already do what the manual promises
(separates broad tonal balance from Trim's fine top/bottom adjustments) and nothing in the research
argues the current range is wrong, only that the "tilt" label over-claims topology purity.

### 3.5 Hiss — corrected spectral tilt

- Keep: 0–100% linear amount, off-by-default, deterministic fixed-seed noise, ~-44 dBFS peak ceiling
  at 100% (unchanged — nothing in the research argues this level is wrong).
- Change: generate/shape Hiss so it is **HF-forward** (closer to white/lightly-tilted-bright noise)
  rather than inheriting the downsampler's darkening filter. Concretely: keep the noise generator
  inside the oversampled domain for real-time-safety/determinism reasons, but insert a dedicated
  gentle high-shelf boost (default **+4 dB above 4 kHz**) on the noise tap specifically, so the
  post-downsample result reads as "hiss," not "muffled static." This is the single most clearly
  "backwards vs. reference" v1 behavior (research notes §6) and the cheapest to fix.

### 3.6 Wow/Flutter — independent depth controls, re-centered flutter rate

- **Breaking parameter change (pre-1.0, allowed):** split the single `Wow/Flutter` proportion into two
  independent parameters, **Wow** (0–100%) and **Flutter** (0–100%), each still off-by-default (0%).
  Reasoning: research notes §5b — the reference class more often exposes these independently; a single
  joint knob can't express "slow pitch drift with no shimmer" or vice versa, both of which are
  legitimate, distinct "worn tape" characters.
- Wow rate: keep **0.7 Hz** (already well-centered inside the sourced 0.5–6 Hz wow band).
- Flutter rate: raise from v1's 6.5 Hz to **11 Hz** default, moving it off the wow/flutter band
  boundary and further into the audibly-distinct "shimmer" register (sourced band: 6–100 Hz; 11 Hz is
  low-but-clearly-separated from wow's 0.7 Hz, avoiding a "second, faster wow" read).
- Depths: keep v1's existing max depths (2.5 ms wow / 0.35 ms flutter) as the ceiling for each new
  independent parameter — nothing in the research argues these magnitudes are wrong, only that they
  should be independently reachable.
- `wowFlutterBaseDelayMs` (6 ms fixed latency contributor) is unchanged — pure engineering plumbing,
  out of scope.

### 3.7 Mix / Output / Drive calibration note (new documentation, no DSP change)

Document (manual + in-code comment) that Aureate's Drive/Warmth defaults are tuned assuming a nominal
**-18 dBFS RMS** input level (research notes §8's sourced tape-calibration convention, i.e. roughly
"0 VU"). This doesn't change any DSP — it's a documented assumption that (a) explains why the default
Drive of 6 dB feels different on a hot digital bus vs. a conservatively-gain-staged one, and (b)
anchors the M2 preset system's use of Drive/Output together as a matched gain-staging pair rather than
independent knobs.

---

## 4. Factory Presets (for the upcoming M2 preset system)

All settings given as `Character | Drive | Warmth | Bias | Tone | HF Trim | LF Trim | Wow | Flutter |
Hiss | Mix | Output`, proportions in %/dB matching the parameter ranges above. These are starting
points for M2 implementation, not final-tuned values — actual numbers should be ear-checked against
real orchestral material once the v2 Character/Hiss/head-bump changes land.

1. **String Section Glue** — cohesion pass for a divisi string bus.
   `Tape | 5 dB | 30% | 0% | +5% | 0 dB | 0 dB | 0% | 0% | 0% | 100% | 0 dB`
2. **Brass Bloom** — a little push for a brass bus without grit.
   `Console | 8 dB | 40% | +10% | +10% | +1 dB | 0 dB | 0% | 0% | 0% | 100% | -1 dB`
3. **Choir Warmth** — gentle darkening + roundness for a choral bus.
   `Tape | 4 dB | 50% | 0% | -15% | -1 dB | +1 dB | 0% | 0% | 0% | 100% | 0 dB`
4. **Orchestral Submix Cohesion** — the "before it meets the metal instrumentation" bus pass the
   manual already describes.
   `Console | 6 dB | 35% | 0% | 0% | 0 dB | 0 dB | 0% | 0% | 0% | 100% | 0 dB`
5. **Master Glue (Subtle)** — sparing mix-bus pass.
   `Tape | 3 dB | 25% | 0% | 0% | 0 dB | 0 dB | 0% | 0% | 0% | 45% | 0 dB`
6. **Vintage Tape Pad** — deliberate worn-tape character for sustained pads/mellotron-style patches.
   `Tape | 10 dB | 70% | -10% | -10% | 0 dB | +2 dB | 20% | 15% | 25% | 100% | -1 dB`
7. **Valve Push** — the most aggressively asymmetric/even-harmonic option, for a section that wants
   audible tube-style push.
   `Valve | 14 dB | 60% | +15% | 0% | +1 dB | 0 dB | 0% | 0% | 0% | 100% | -2 dB`
8. **Parallel Grit (New York)** — heavily driven wet signal blended under a clean dry signal.
   `Valve | 20 dB | 80% | 0% | 0% | 0 dB | 0 dB | 0% | 0% | 0% | 50% | -3 dB`
9. **Console Summing Sheen** — the "least characterful until pushed" console archetype at a level
   where it's mostly transparent.
   `Console | 4 dB | 20% | 0% | +5% | 0 dB | 0 dB | 0% | 0% | 0% | 100% | 0 dB`
10. **Air & Weight** — Trim-forward preset showcasing HF/LF Trim independent of Tone/Warmth.
    `Tape | 5 dB | 25% | 0% | 0% | +3 dB | +2 dB | 0% | 0% | 0% | 100% | 0 dB`

---

## 5. Catch2 test guarantees (v2 additions on top of v1's 60 tests)

Existing v1 guarantees (silence-in/out, near-linear small-signal, monotonic, bounded/no-NaN,
asymmetric-ceiling-differs) are kept unconditionally for all three Character models — no regression.
New/changed guarantees needed for v2's category-specific claims:

1. **Character harmonic-balance ordering (spectral/THD test).** Feed each Character model a pure sine
   at a fixed drive level; compute an FFT (or Goertzel bank) of the output and measure H2/H3 energy
   ratio. Assert: `Tape.H2/H3 < Console.H2/H3 < Valve.H2/H3` at matched Bias=Warmth=0 settings — i.e.
   Tape is the most odd-dominant, Valve the most even-dominant, Console in between. This directly
   encodes §3.3's reordering as a measurable proof, not just a doc claim.
2. **Console transparency-at-low-drive test.** At a fixed low input level (e.g. -12 dBFS pre-Drive)
   and Warmth=0, assert Console's THD is the *lowest* of the three models (was previously the
   highest/hardest via `cubicSoftClip`'s hard-flat region) — proves the "soft until pushed" reordering.
3. **Tape near-symmetry test.** At Warmth=100% (max per-Character bias), assert Tape's
   positive/negative ceiling asymmetry ratio is smaller in magnitude than Valve's at the same Warmth —
   a relative (not just each-nonzero) asymmetry-ordering proof.
4. **Head-bump filter response test.** At Warmth=100%, assert the LF shelf/peak stage's magnitude
   response at 80 Hz exceeds its magnitude response at 20 Hz and at 500 Hz by the documented amount
   (within tolerance) — a static-curve proof analogous to existing Warmth-rolloff-style filter tests.
   At Warmth=0%, assert the head-bump contributes ~0 dB everywhere (regression/off-state proof).
5. **Hiss spectral-tilt test.** Compare Hiss-only output (Character/Drive/Warmth neutral, Hiss=100%)
   band energy in a low band (e.g. 100–500 Hz) vs. a high band (e.g. 6–12 kHz): assert high-band
   energy is now greater than or equal to low-band energy (inverts v1's implicit
   darker-than-flat behavior) — the direct regression proof for §3.5's fix. Keep v1's existing
   peak-ceiling (~-44 dBFS) and off-at-0% guarantees unchanged.
6. **Wow/Flutter independence test.** With Wow=100%/Flutter=0%, assert modulation spectral content is
   concentrated near the Wow rate only (e.g. via zero-crossing-rate or narrowband energy at ~0.7 Hz vs.
   ~11 Hz); with Wow=0%/Flutter=100%, assert the inverse. This is the direct proof that splitting the
   parameter (§3.6) actually decouples the two effects rather than just adding a second knob that
   still moves both.
7. **Wow/Flutter latency-independence regression (kept from v1's existing pattern).** Extend the
   existing "reported latency doesn't depend on live Wow/Flutter parameter value" test to cover both
   new independent Wow and Flutter parameters separately.
8. **State/preset round-trip tolerant-import test (new, M2-adjacent but exercised here since Wow/
   Flutter is a breaking parameter split).** A state blob saved under the v1 single `wowFlutter`
   parameter ID must import cleanly under v2's `wow`/`flutter` pair via a documented migration
   (§7) — assert both new parameters land at a sane derived value (e.g. both set to the old single
   value) rather than failing to load or silently defaulting both to 0.
9. **Character-dependent bias ceiling regression.** Assert `setWarmthProportion(1.0)` under each
   Character model produces the *new* per-model max bias (0.12 Tape / 0.10 Console / 0.30 Valve), not
   the old shared 0.3, closing the loop on §3.3's table as an executable spec.

---

## 6. Honesty section (research-derived, not hardware-measured)

Every numeric default in this brief is either (a) carried over unchanged from v1's already-shipped,
tested values where research found no contradiction, or (b) newly chosen to sit inside a sourced
*band* or match a sourced *qualitative ordering* from the research-notes citations — none of it is
calibrated against an actual tape machine, valve stage, or console captured/measured by this project.
Concretely:

- Wow (0.7 Hz) and Flutter (11 Hz) are chosen to sit inside their respective sourced bands
  (0.5–6 Hz / 6–100 Hz) and to be perceptually separated from each other — not measured off a specific
  transport.
- The Character harmonic-balance *ordering* (Tape odd-dominant < Console blended < Valve even-dominant)
  is directly sourced (research notes §1/§2/§4); the *specific* bias-strength numbers (0.12/0.10/0.30)
  chosen to realize that ordering are engineering choices tuned to produce a measurable, testable
  separation, not transcribed from any measured device.
- The LF head-bump center (80 Hz) and gain (+1.5 dB at 100% Warmth) are a single-point approximation
  of a phenomenon the research confirms is real and speed-dependent (notes §3) but for which Aureate
  has no discrete tape-speed control to key off of — this is a deliberate scope simplification, not a
  measured curve for any specific machine/speed.
- The -18 dBFS calibration anchor (§3.7) is a widely-cited convention, not a measurement of anything
  Aureate-specific — it's documented as an assumption other defaults are consistent with, not a
  derived constant.
- The Hiss HF-shelf correction (+4 dB above 4 kHz) is chosen to make Hiss read as broadband/HF-forward
  per the sourced qualitative description ("high frequency white noise"), not fit to a measured tape
  noise spectrum.
- Factory preset settings (§4) are proposals for M2 implementation to ear-check against real material;
  none are transcribed from a documented factory preset of any reference product (none was found/used
  as a source — the research notes deliberately avoid citing any product's actual preset values).

**This brief should be read as "engineering choices made responsibly consistent with sourced literature
on real tape/valve/console behavior," not as "measurements of specific hardware."**

---

## 7. Versioning

- Target: **v0.2.0**.
- Breaking parameter changes are explicitly allowed pre-1.0 (per project convention): the
  Wow/Flutter → Wow + Flutter split (§3.6) and the Character-dependent Warmth bias ceilings (§3.3) are
  both breaking changes to saved automation/state relative to v0.1.0.
- **State migration policy: tolerant import.** A v0.1.0-saved state (single `wowFlutter` parameter,
  shared `warmthMaxBias`) must still load in v0.2.0 without erroring:
  - `wowFlutter` (old, single) → map to both new `wow` and `flutter` parameters at the same value on
    import, so old sessions retain a recognizable (if not identical) character rather than silently
    resetting to 0%.
  - Warmth/Bias/Character combination: no explicit parameter renumbering needed (the bias ceiling
    change is a DSP-mapping change, not a parameter-ID change), but the manual/changelog must call out
    that identical Warmth/Bias/Character values will sound different (less asymmetric for Tape/Console,
    same for Valve) after upgrading — an audible-but-non-crashing behavior change, not a state-loading
    failure.
  - Add a `StateTests.cpp` case loading a synthetic v0.1.0-shaped state blob (single `wowFlutter` key)
    and asserting successful, non-throwing load plus the documented `wow`/`flutter` fallback mapping
    (ties to §5 guarantee 8).
- No other v1 parameter IDs are removed or renumbered in this brief.
