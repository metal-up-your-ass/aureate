#pragma once

// Central definition of all AudioProcessorValueTreeState parameter IDs for
// Aureate. See docs/architecture.md for the corresponding signal-flow
// diagram.
//
// FROZEN AS OF THE v0.1 PARAMETER LAYOUT:
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
}
