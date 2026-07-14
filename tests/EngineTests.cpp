#include "dsp/AureateEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
    constexpr double testSampleRate = 48000.0;
    constexpr int testBlockSize = 8192; // large single block: keeps the null/
                                         // correlation tests below simple by
                                         // avoiding multi-block bookkeeping.
    constexpr double testFrequencyHz = 1000.0;

    juce::dsp::ProcessSpec makeTestSpec (int numChannels)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = testSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (testBlockSize);
        spec.numChannels = static_cast<juce::uint32> (numChannels);
        return spec;
    }
}

TEST_CASE ("Engine null test: 0% mix nulls against the input once shifted by latency", "[dsp][engine][null]")
{
    AureateEngine engine;

    // Parameters other than Mix are deliberately set to non-neutral values:
    // a true null test has to prove the *entire* wet chain is bypassed, not
    // just that it happens to be quiet at default settings.
    engine.setMixProportion (0.0f);
    engine.setDriveDb (18.0f);
    engine.setWarmthProportion (0.8f);
    engine.setToneProportion (0.6f);
    // Output is a post-mix master trim, so it applies even at Mix=0% - it
    // must stay at unity (0 dB) here, or the "passthrough" property under
    // test would be broken by design, not by a bug.
    engine.setOutputDb (0.0f);

    const auto spec = makeTestSpec (2);
    engine.prepare (spec);

    const auto latency = engine.getLatencySamples();
    REQUIRE (latency >= 0);
    // Sanity bound: the oversampling latency must be well inside both the
    // DryWetMixer's fixed dry-delay capacity (1024, see AureateEngine.h)
    // and the test block size, or the overlap window below would be
    // meaningless.
    REQUIRE (latency < testBlockSize / 2);

    juce::AudioBuffer<float> reference (2, testBlockSize);
    TestHelpers::fillWithSine (reference, testSampleRate, testFrequencyHz, 0.5f);

    juce::AudioBuffer<float> processed;
    processed.makeCopyOf (reference);

    juce::dsp::AudioBlock<float> block (processed);
    engine.process (block);

    const auto overlapLength = testBlockSize - latency;
    REQUIRE (overlapLength > testBlockSize / 2);

    // < -90 dBFS residual, in linear amplitude.
    constexpr float tolerance = 3.1623e-5f; // 10^(-90/20)

    for (int channel = 0; channel < reference.getNumChannels(); ++channel)
    {
        const auto* refData = reference.getReadPointer (channel);
        const auto* outData = processed.getReadPointer (channel);

        float maxResidual = 0.0f;

        for (int i = 0; i < overlapLength; ++i)
            maxResidual = std::max (maxResidual, std::abs (outData[latency + i] - refData[i]));

        CHECK (maxResidual < tolerance);
    }
}

TEST_CASE ("Engine sanity test: minimum drive, flat tone, no warmth keeps the wet path near-unity", "[dsp][engine]")
{
    AureateEngine engine;

    // Minimum drive (0 dB), Warmth at 0% (no HF rolloff, symmetric/unbiased
    // saturator), Tone at 0% (both tilt shelves flat), Mix fully wet, Output
    // at unity so we are measuring the wet chain itself.
    engine.setDriveDb (0.0f);
    engine.setWarmthProportion (0.0f);
    engine.setToneProportion (0.0f);
    engine.setMixProportion (1.0f);
    engine.setOutputDb (0.0f);

    const auto spec = makeTestSpec (2);
    engine.prepare (spec);

    const auto latency = engine.getLatencySamples();
    REQUIRE (latency < testBlockSize / 2);

    // Low amplitude (-34 dBFS): comfortably inside the saturator's
    // near-linear region even with zero headroom above unity drive gain.
    // This is the correct way to probe "is the wet path near-unity at
    // minimum drive" - the saturator is a tanh curve, so it is never
    // perfectly linear for any amplitude, but it approaches linearity/unity
    // gain as amplitude shrinks.
    juce::AudioBuffer<float> warmup (2, testBlockSize);
    TestHelpers::fillWithSine (warmup, testSampleRate, testFrequencyHz, 0.02f, 0);

    // Run one full block through first purely to let any filter state
    // settle out of its zero-state turn-on transient before measuring.
    {
        juce::dsp::AudioBlock<float> warmupBlock (warmup);
        engine.process (warmupBlock);
    }

    juce::AudioBuffer<float> reference (2, testBlockSize);
    TestHelpers::fillWithSine (reference, testSampleRate, testFrequencyHz, 0.02f, testBlockSize);

    juce::AudioBuffer<float> processed;
    processed.makeCopyOf (reference);

    juce::dsp::AudioBlock<float> block (processed);
    engine.process (block);

    const auto overlapLength = testBlockSize - latency;
    REQUIRE (overlapLength > testBlockSize / 2);

    // Search a small window of sample-alignment offsets around the reported
    // oversampling latency: the Warmth low-pass and Tone tilt shelves (both
    // running inside the oversampled domain, see AureateEngine.h) have their
    // own small group delay at 1 kHz which is not part of
    // getLatencySamples() and is not what this test is probing. Unlike a
    // single filter's near-integer delay, three cascaded oversampled-domain
    // filters can leave a residual *fractional*-sample delay after
    // downsampling that no integer shift search can fully cancel - this
    // caps the achievable correlation somewhat below 1.0 even for a
    // genuinely near-linear chain, which is why the bound below is 0.995
    // (still very high) rather than Overture's 0.9999. The RMS-gain check
    // further down is the primary "near-unity" measure and is unaffected by
    // this residual sub-sample phase error.
    constexpr int maxUnaccountedGroupDelaySamples = 8;

    for (int channel = 0; channel < reference.getNumChannels(); ++channel)
    {
        const auto* refData = reference.getReadPointer (channel);
        const auto* outData = processed.getReadPointer (channel) + latency;

        const auto correlation = TestHelpers::bestCorrelationOverShift (outData, refData, overlapLength, maxUnaccountedGroupDelaySamples);
        CHECK (correlation > 0.995);

        // "Near-unity", not just "linearly related": the RMS gain between
        // wet output and the reference input must be close to 1.
        double sumOfSquaresOut = 0.0;
        double sumOfSquaresRef = 0.0;

        for (int i = 0; i < overlapLength; ++i)
        {
            sumOfSquaresOut += static_cast<double> (outData[i]) * static_cast<double> (outData[i]);
            sumOfSquaresRef += static_cast<double> (refData[i]) * static_cast<double> (refData[i]);
        }

        const auto rmsGain = std::sqrt (sumOfSquaresOut / sumOfSquaresRef);
        CHECK (rmsGain == Catch::Approx (1.0).margin (0.02));
    }
}

TEST_CASE ("Engine: wet output magnitude is monotonically non-decreasing with Drive, and bounded", "[dsp][engine]")
{
    constexpr float driveStepsDb[] = { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f };

    double previousRms = -1.0;

    for (const auto driveDb : driveStepsDb)
    {
        AureateEngine engine;
        engine.setDriveDb (driveDb);
        engine.setWarmthProportion (0.0f); // isolate Drive's effect: no bias/rolloff
        engine.setToneProportion (0.0f);
        engine.setMixProportion (1.0f);
        engine.setOutputDb (0.0f);

        const auto spec = makeTestSpec (2);
        engine.prepare (spec);

        const auto latency = engine.getLatencySamples();

        juce::AudioBuffer<float> buffer (2, testBlockSize);
        TestHelpers::fillWithSine (buffer, testSampleRate, testFrequencyHz, 0.5f);

        juce::dsp::AudioBlock<float> block (buffer);
        engine.process (block);

        // Measure RMS over the settled region only (past the oversampling
        // latency), so the filter/oversampler turn-on transient at the very
        // start of the block doesn't skew the comparison across drive
        // settings.
        juce::AudioBuffer<float> settled (2, testBlockSize - latency);

        for (int channel = 0; channel < 2; ++channel)
            settled.copyFrom (channel, 0, buffer, channel, latency, testBlockSize - latency);

        const auto currentRms = TestHelpers::rms (settled);

        CHECK (TestHelpers::allSamplesFinite (buffer));
        CHECK (TestHelpers::peakAbsolute (buffer) < 2.5f); // tanh saturates well inside +/-2.5 at unity output

        // Small numerical margin: successive drive steps must not produce a
        // *lower* RMS than the previous, less-driven step.
        CHECK (currentRms >= previousRms - 1.0e-6);
        previousRms = currentRms;
    }
}

TEST_CASE ("Engine reset() clears filter/oversampler/delay state without crashing", "[dsp][engine]")
{
    AureateEngine engine;
    engine.setDriveDb (20.0f);
    engine.setWarmthProportion (0.7f);
    engine.setMixProportion (1.0f);

    const auto spec = makeTestSpec (2);
    engine.prepare (spec);

    juce::AudioBuffer<float> buffer (2, testBlockSize);
    TestHelpers::fillWithSine (buffer, testSampleRate, testFrequencyHz, 0.9f);

    juce::dsp::AudioBlock<float> block (buffer);
    engine.process (block);

    CHECK_NOTHROW (engine.reset());
    CHECK (TestHelpers::allSamplesFinite (buffer));

    // Processing again straight after reset() must not crash or produce
    // non-finite output.
    TestHelpers::fillWithSine (buffer, testSampleRate, testFrequencyHz, 0.9f);
    CHECK_NOTHROW (engine.process (block));
    CHECK (TestHelpers::allSamplesFinite (buffer));
}
