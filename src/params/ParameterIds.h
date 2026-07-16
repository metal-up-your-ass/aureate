#pragma once

// Central definition of all AudioProcessorValueTreeState parameter IDs for
// Aureate. See docs/architecture.md for the corresponding signal-flow
// diagram.
//
// FROZEN AS OF THE v0.1.0 PARAMETER LAYOUT:
// Parameter IDs below must NEVER change once shipped - saved sessions and
// presets persist the APVTS state keyed by these string IDs, and renaming or
// removing one would silently break every user's saved state. Ranges,
// defaults, and skew MAY still be refined during voicing/tuning milestones;
// only the IDs themselves are frozen.
namespace ParamIDs
{
    // Input gain into the oversampled tape-style saturation stage.
    inline constexpr auto drive = "drive";

    // "Warmth": jointly controls the saturator's asymmetry bias (tape-style
    // single-ended saturation character) and a gentle pre-clip high-frequency
    // rolloff (tape self-erasure/bias-oscillator character), both applied
    // inside the oversampled domain.
    inline constexpr auto warmth = "warmth";

    // Console-style tilt tone control: negative darkens (low-shelf boost +
    // high-shelf cut), positive brightens (the inverse), 0 is flat/unity.
    inline constexpr auto tone = "tone";

    // Dry/wet mix. At 0% the plugin is a delay-compensated passthrough of
    // the input (see AureateEngine's DryWetMixer usage).
    inline constexpr auto mix = "mix";

    // Final output trim, applied after the dry/wet mix (a master trim on the
    // combined signal, unlike Drive which only affects the wet path).
    inline constexpr auto output = "output";

    // Additional saturator asymmetry trim, independent of (and added to)
    // Warmth's own bias contribution - lets a session dial in more/less
    // single-ended character without disturbing Warmth's HF-rolloff amount.
    inline constexpr auto bias = "bias";

    // v0.2.0 (docs/design-brief.md §3.6): the single v0.1.0 "Wow/Flutter"
    // proportion was split into two independent amounts - a slow tape-
    // transport pitch "wow" and a faster "flutter" shimmer - applied to the
    // wet path via a shared modulated delay line. 0% on both is a fixed
    // (non-modulated) delay - see AureateEngine for why a small fixed delay
    // is always present regardless of either amount.
    inline constexpr auto wow = "wow";
    inline constexpr auto flutter = "flutter";

    // FROZEN, retired v0.1.0 parameter ID - NOT a live APVTS parameter in
    // v0.2.0's ParameterLayout (superseded by wow/flutter above). Kept as a
    // named constant purely so AureateAudioProcessor::setStateInformation()'s
    // v0.1.0->v0.2.0 state migration can recognise a pre-split saved state
    // (a single <PARAM id="wow_flutter" .../> entry) and map it onto both new
    // parameters at the same value (docs/design-brief.md §7's migration
    // policy) rather than silently resetting Wow/Flutter to 0% on load.
    inline constexpr auto legacyWowFlutter = "wow_flutter";

    // Amount of shaped noise ("tape hiss") mixed into the wet path inside
    // the oversampled domain, after the saturator/tone stages and before
    // downsampling (so it inherits the downsampler's anti-aliasing filter).
    inline constexpr auto hiss = "hiss";

    // Selects which saturation transfer-function family the Drive/Warmth/
    // Bias stage uses (see TapeSaturator::Model): Tape (default), Console,
    // or Valve.
    inline constexpr auto character = "character";

    // Fixed-frequency high-shelf trim, independent of and in addition to
    // Tone's tilt shelves - a finer top-end adjustment at a higher corner
    // frequency than Tone's high shelf.
    inline constexpr auto hfTrim = "hf_trim";

    // Fixed-frequency low-shelf trim, independent of and in addition to
    // Tone's tilt shelves - a finer low-end adjustment at a lower corner
    // frequency than Tone's low shelf.
    inline constexpr auto lfTrim = "lf_trim";
}
