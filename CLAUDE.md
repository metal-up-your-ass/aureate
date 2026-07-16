# Aureate — tape / console saturation glue (mix)

Per-repo working memory for Claude Code sessions on this plugin. Part of the **Basilica Audio** plugin suite — sacred-architecture DSP for heavy music (`github.com/basilica-audio`).

## What this is
Aureate is the "tape / console saturation glue (mix)" member of the suite. AU / VST3 / Standalone, JUCE 8.

## Status (v0.2.0 — deep-dive voicing rework + M2 preset system done)
Core DSP reworked for v0.2.0 against research-derived voicing corrections (Character harmonic-balance ordering, a new LF head bump, Hiss's spectral tilt, independent Wow/Flutter), plus the suite's M2 preset system (`src/presets/`, ported from sibling `basilica-audio/nave`'s pilot) and a German frame localisation. **96 Catch2 tests green**. GUI is a functional v0.1/v0.2 slider/combo-box editor plus a preset bar (custom LookAndFeel is still roadmap M3). No signing yet (roadmap M4). See `docs/design-brief.md` (+ `docs/research-notes.md`) for the full v0.2.0 rationale and `docs/presets.md` for the 11 factory presets. Open work is tracked in this repo's GitHub **milestones/issues**.

## DSP
Chain: input -> Wow/Flutter (independent wow+flutter modulated delay, host rate) -> Drive (0-24 dB) -> [4x oversampled: Warmth-driven gentle HF-rolloff low-pass -> LF head-bump peak (80 Hz, Q 0.9, up to +1.5 dB at 100% Warmth) -> Character-selected saturator (Tape tanh / Console scaled-tanh soft knee / Valve exponential, asymmetry driven by Warmth+Bias combined, Warmth's own bias ceiling now Character-dependent - see `AureateEngine::characterMaxWarmthBias()`) -> dual-shelf tilt-style Tone (low/high shelf pair, +/-6 dB at +/-100%) -> HF/LF Trim (independent fixed-frequency shelves, 8 kHz/150 Hz, +/-6 dB) -> Hiss (shaped noise routed through its own fixed +4dB-above-4kHz HF shelf, linear 0-100% amount)] -> downsample -> DryWetMixer (delay-compensated) -> Output trim (-24..+24 dB, applied post-mix as a master trim, unlike Drive which only affects the wet path). Warmth (0-100%) jointly maps to the pre-clip low-pass cutoff (20 kHz -> 3 kHz, log-interpolated), the saturator's asymmetry bias (0 -> Character-dependent max: Tape 0.12/Console 0.10/Valve 0.30), and the head-bump gain (0 -> 1.5 dB); Bias (-100..100%) adds an independent +/-0.3 bias contribution on top, combined bias clamped to +/-0.9. Latency is the 4x half-band polyphase IIR oversampler's reported latency (useIntegerLatency=true) **plus** Wow/Flutter's fixed base delay (6 ms, rounded to an exact integer sample count at prepare() time so it never depends on either live Wow/Flutter amount) - reported via setLatencySamples and compensated on the dry path via DryWetMixer::setWetLatency, primed before reset() per the documented JUCE 8.0.14 DryWetMixer gotcha (own regression tests in DryWetMixerContractTests.cpp). DryWetMixer's internal delay-line capacity is 8192 samples (not 1024) to stay above the worst-case total latency at 192 kHz now that Wow/Flutter's base delay is part of getLatencySamples(). Full parameter reference and musical rationale: `docs/manual.md`.

**Known pre-existing real-time-safety caveat** (inherited from v0.1.0, not introduced in v0.2.0): `AureateEngine::process()` recomputes several filters' IIR coefficients once per block via `juce::dsp::IIR::Coefficients<float>::makeLowPass`/`makeHighShelf`/`makeLowShelf`/`makePeakFilter`, each of which heap-allocates a reference-counted `Coefficients` object internally (`new` inside JUCE's own factory methods) before being copied into the persistent filter state and discarded. This is a genuine per-block audio-thread allocation, present in the v0.1.0 code this repo builds on (`warmthLowPass`/`tiltLowShelf`/`tiltHighShelf`/`hfTrimShelf`/`lfTrimShelf`) and now also true of the new `headBumpPeak` filter, which follows the same established pattern. `hissShelf`'s coefficients are fixed and only computed once in `prepare()`, so it does not add a new per-block allocation. Not fixed in this pass (would require a materially different filter-coefficient architecture, out of scope for the v0.2.0 voicing brief) - flagged here for a future real-time-safety hardening pass.

## Build & test
```sh
export CPM_SOURCE_CACHE="$HOME/.cache/CPM"      # shared JUCE 8.0.14 + Catch2 cache
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target Tests Aureate_Standalone --parallel 4
ctest --test-dir build --output-on-failure
```
Release/universal + pluginval + auval run in CI, not locally.

## Conventions & guardrails
- JUCE 8.0.14 via CPM · C++20 · AGPLv3 · Pamplejuce `SharedCode` pattern · manufacturer `Yvsv`, plugin code `Aurt`, `com.yvesvogl.aureate`.
- **Real-time safety:** no alloc/lock/file-IO/logging on the audio thread; allocate in `prepareToPlay`; `reset()` clears all state; `ScopedNoDenormals`; smoothed params; report latency via `setLatencySamples` where the chain adds any.
- **DryWetMixer gotcha (JUCE 8.0.14):** prime `setWetMixProportion(mix)` before `reset()` in `prepare()` (else it ramps from 100% wet). See sibling `overture`.
- **`main` is protected** — no direct commits; feature branch + PR, green CI required (Conventional Commits). New DSP needs tests (null/reference, NaN/Inf sweep, state round-trip, latency).

## Roadmap
GitHub milestones (M1 DSP & tests · M2 presets/state · M3 GUI & a11y · M4 release/signing/v1.0.0) + issues. Read with `gh issue list --repo basilica-audio/aureate`.

## Suite context
Style references: sibling `basilica-audio/overture` and `basilica-audio/crypta`. The suite: overture, tenebrae, nave, silentium, requiem, seraph, aureate, firmament, triptych, apotheosis, crypta.
