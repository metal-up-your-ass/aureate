#pragma once

#include <cmath>

// A single-ended, asymmetric soft-knee saturator - the nonlinearity at the
// heart of Aureate's tape-style "Warmth"/"Character" voicing. Pure,
// allocation-free, and stateless, so it is unit-testable in complete
// isolation from the rest of the signal chain (see
// tests/TapeSaturatorTests.cpp) and safe to call per-sample from the audio
// thread (it runs inside AureateEngine's 4x oversampled block).
//
// Default/Tape transfer function: y = tanh(x + bias) - tanh(bias)
//
// Shifting the curve by a fixed bias before re-centring it at y(0) == 0
// gives the two effects "Warmth"/"Bias" are meant to control together with
// the engine's companion HF-rolloff filter:
//   - The positive and negative half-cycles saturate towards different
//     asymptotic ceilings, i.e. genuine asymmetric clipping, emulating
//     single-ended tape/valve saturation rather than a symmetric fuzz.
//   - Because the curve is not globally linear, a zero-mean AC input
//     produces a small even-harmonic-rich, DC-shifted output - the gentle
//     "glue" character this plugin is voiced around.
// Subtracting f(bias) guarantees processSample(0, bias) == 0 for any bias,
// so the saturator never injects a constant DC offset into silence, for
// every model below.
namespace TapeSaturator
{
    // The "Character" parameter's three curve families. Each keeps the same
    // shift-then-recentre-by-bias shape as the original Tape model (see
    // above), so silence-in/silence-out and asymmetric-ceiling behaviour
    // hold identically for all three - only the underlying curve differs.
    // See docs/manual.md for the musical description of each.
    enum class Model
    {
        tape,    // asymmetric tanh - smooth, "infinite" soft compression, the most odd-harmonic-dominant/least asymmetric of the three (the original/default model)
        console, // asymmetric scaled-tanh soft knee - transparent at low-to-moderate levels, blended even/odd harmonics once pushed (v0.2.0: replaces v0.1's hard-flat cubic soft-clip)
        valve    // asymmetric exponential saturation - the most asymmetric/even-harmonic-forward of the three
    };

    namespace detail
    {
        // Console: a scaled tanh-family soft knee (research notes §4 - real
        // console/transformer summing-bus saturation is a soft, blended
        // even+odd knee, not the hardest-clipping of the three models, so it
        // should stay closer to linear for longer than Tape's plain tanh(x)
        // before curving away). consoleSoftKneeScale > 1 keeps unity slope at
        // the origin (matching Tape/Valve's small-signal gain) while its
        // cubic Taylor coefficient (1/(3*scale^2)) is smaller than tanh's
        // own 1/3, i.e. genuinely more transparent at low-to-moderate levels
        // - only showing character once driven well past unity, per the
        // brief's "least characterful until pushed" reordering (replaces
        // v0.1's hard-flat cubic soft-clip, which inverted this ordering).
        inline constexpr float consoleSoftKneeScale = 2.0f;

        inline float consoleSoftKnee (float v) noexcept
        {
            return consoleSoftKneeScale * std::tanh (v / consoleSoftKneeScale);
        }

        // Exponential saturation: strictly monotonic and asymptotic to
        // +/-1, giving a different even/odd harmonic blend than either the
        // tanh or the console soft-knee curves.
        inline float exponentialSoftClip (float v) noexcept
        {
            return std::copysign (1.0f - std::exp (-std::abs (v)), v);
        }
    }

    // The original two-argument Tape-model entry point. Kept as a separate,
    // unconditional overload (rather than routing through the Model-taking
    // overload below) so it stays trivially inlinable and so existing
    // callers/tests that only know about the Tape model are unaffected by
    // the Character parameter's existence.
    inline float processSample (float x, float bias) noexcept
    {
        return std::tanh (x + bias) - std::tanh (bias);
    }

    // Model-selecting overload used by AureateEngine once Character is
    // wired up to more than just Tape.
    inline float processSample (float x, float bias, Model model) noexcept
    {
        switch (model)
        {
            case Model::console:
                return detail::consoleSoftKnee (x + bias) - detail::consoleSoftKnee (bias);
            case Model::valve:
                return detail::exponentialSoftClip (x + bias) - detail::exponentialSoftClip (bias);
            case Model::tape:
            default:
                return processSample (x, bias);
        }
    }
}
