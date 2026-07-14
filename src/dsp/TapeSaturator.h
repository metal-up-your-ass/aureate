#pragma once

#include <cmath>

// A single-ended, asymmetric tanh soft-knee saturator - the nonlinearity at
// the heart of Aureate's tape-style "Warmth" voicing. Pure, allocation-free,
// and stateless, so it is unit-testable in complete isolation from the rest
// of the signal chain (see tests/TapeSaturatorTests.cpp) and safe to call
// per-sample from the audio thread (it runs inside AureateEngine's 4x
// oversampled block).
//
// Transfer function: y = tanh(x + bias) - tanh(bias)
//
// Shifting the tanh curve by a fixed bias before re-centring it at y(0) == 0
// gives the two effects "Warmth" is meant to control together with the
// engine's companion HF-rolloff filter:
//   - The positive and negative half-cycles saturate towards different
//     asymptotic ceilings (1 - tanh(bias) vs -(1 + tanh(bias))), i.e.
//     genuine asymmetric clipping, emulating single-ended tape/valve
//     saturation rather than a symmetric fuzz.
//   - Because the curve is not globally linear, a zero-mean AC input
//     produces a small even-harmonic-rich, DC-shifted output - the gentle
//     "glue" character this plugin is voiced around.
// Subtracting tanh(bias) guarantees processSample(0, bias) == 0 for any
// bias, so the saturator never injects a constant DC offset into silence.
namespace TapeSaturator
{
    inline float processSample (float x, float bias) noexcept
    {
        return std::tanh (x + bias) - std::tanh (bias);
    }
}
