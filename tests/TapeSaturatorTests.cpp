#include "dsp/TapeSaturator.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>

TEST_CASE ("TapeSaturator: silence in, silence out, for any bias", "[dsp][saturator]")
{
    CHECK (TapeSaturator::processSample (0.0f, 0.0f) == Catch::Approx (0.0f).margin (1e-9));
    CHECK (TapeSaturator::processSample (0.0f, 0.15f) == Catch::Approx (0.0f).margin (1e-9));
    CHECK (TapeSaturator::processSample (0.0f, -0.3f) == Catch::Approx (0.0f).margin (1e-9));
}

TEST_CASE ("TapeSaturator: near-linear for small-signal input", "[dsp][saturator]")
{
    // For small x, tanh(x + bias) - tanh(bias) ~= x * sech^2(bias) (first-
    // order Taylor expansion around x=0). Checking against that predicted
    // small-signal gain - rather than against raw unity gain - is the
    // correct "near-linear" check for a curve that has a fixed insertion
    // loss/gain by design (the bias itself changes the slope at the
    // operating point, independent of any drive/gain staging elsewhere in
    // the chain).
    constexpr float bias = 0.3f;
    const auto sech2 = 1.0f - std::tanh (bias) * std::tanh (bias);

    for (float x : { 1.0e-4f, 5.0e-4f, 1.0e-3f, -1.0e-3f, 2.0e-3f })
    {
        const auto y = TapeSaturator::processSample (x, bias);
        const auto predictedLinear = x * sech2;

        // Margin slightly looser than a single-ULP-scale bound: y is
        // produced by float-precision std::tanh calls chained together
        // (tanh(x+bias) - tanh(bias)), so it accumulates a little more
        // rounding error than the single-multiply predictedLinear.
        CHECK (y == Catch::Approx (predictedLinear).margin (2.0e-6));
    }
}

TEST_CASE ("TapeSaturator: monotonically increasing (no folding)", "[dsp][saturator]")
{
    constexpr float bias = 0.3f;
    float previous = TapeSaturator::processSample (-5.0f, bias);

    for (int i = 1; i <= 200; ++i)
    {
        const auto x = -5.0f + static_cast<float> (i) * 0.05f;
        const auto y = TapeSaturator::processSample (x, bias);

        CHECK (y > previous);
        previous = y;
    }
}

TEST_CASE ("TapeSaturator: bounded output for extreme input, no NaN/Inf", "[dsp][saturator]")
{
    constexpr float bias = 0.3f;

    for (float x : { 1.0e6f, -1.0e6f, std::numeric_limits<float>::max() * 0.5f,
                      -std::numeric_limits<float>::max() * 0.5f, 0.0f })
    {
        const auto y = TapeSaturator::processSample (x, bias);

        CHECK (std::isfinite (y));
        CHECK (std::abs (y) < 2.5f); // tanh() saturates well inside +/-2.5 for any finite bias here
    }
}

TEST_CASE ("TapeSaturator: positive and negative half-cycles saturate at different ceilings", "[dsp][saturator]")
{
    constexpr float bias = 0.3f;

    const auto positiveCeiling = TapeSaturator::processSample (50.0f, bias);
    const auto negativeCeiling = TapeSaturator::processSample (-50.0f, bias);

    // Genuinely asymmetric: the two ceilings must differ in magnitude.
    CHECK (std::abs (positiveCeiling) != Catch::Approx (std::abs (negativeCeiling)).margin (1.0e-3));

    // Zero bias recovers a symmetric (odd) tanh curve as a sanity check that
    // the bias term - not some other bug - is what causes the difference
    // above.
    const auto symmetricPositive = TapeSaturator::processSample (50.0f, 0.0f);
    const auto symmetricNegative = TapeSaturator::processSample (-50.0f, 0.0f);
    CHECK (symmetricPositive == Catch::Approx (-symmetricNegative).margin (1.0e-6));
}
