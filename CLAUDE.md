# Aureate — tape / console saturation glue (mix)

Per-repo working memory for Claude Code sessions on this plugin. Part of the **Metal up your ass** symphonic-metal plugin suite (`github.com/metal-up-your-ass`).

## What this is
Aureate is the "tape / console saturation glue (mix)" member of the suite. AU / VST3 / Standalone, JUCE 8.

## Status (v0.1 — bootstrap complete)
Core DSP working, **24 Catch2 tests green**, CI (macOS + Windows, pluginval strictness 10 + auval) green. GUI is a functional v0.1 slider editor (custom LookAndFeel is roadmap M3). No signing yet (roadmap M4). Open work is tracked in this repo's GitHub **milestones/issues**.

## DSP
Chain: input -> Drive (0-24 dB) -> [4x oversampled: Warmth-driven gentle HF-rolloff low-pass -> asymmetric tanh tape saturator (TapeSaturator, y = tanh(x+bias)-tanh(bias)) -> console-style tilt Tone (low/high shelf pair, +/-6 dB at +/-100%)] -> downsample -> DryWetMixer (delay-compensated) -> Output trim (-24..+24 dB, applied post-mix as a master trim, unlike Drive which only affects the wet path). Warmth (0-100%) jointly maps to the pre-clip low-pass cutoff (20 kHz -> 3 kHz, log-interpolated) and the saturator's asymmetry bias (0 -> 0.3, linear) so one control drives both the "tape darkening" and "tape bias" character. Latency is exactly the 4x half-band polyphase IIR oversampler's reported latency (useIntegerLatency=true), reported via setLatencySamples and compensated on the dry path via DryWetMixer::setWetLatency, primed before reset() per the documented JUCE 8.0.14 DryWetMixer gotcha (own regression tests in DryWetMixerContractTests.cpp).

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
GitHub milestones (M1 DSP & tests · M2 presets/state · M3 GUI & a11y · M4 release/signing/v1.0.0) + issues. Read with `gh issue list --repo metal-up-your-ass/aureate`.

## Suite context
Style references: sibling `metal-up-your-ass/overture` and `metal-up-your-ass/twist-your-guts`. The suite: overture, tenebrae, nave, silentium, requiem, seraph, aureate, firmament, triptych, apotheosis, twist-your-guts.
