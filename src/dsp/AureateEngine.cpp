#include "AureateEngine.h"

namespace
{
    // Keeps a requested filter frequency safely below Nyquist regardless of
    // the (possibly oversampled) rate it runs at, so
    // juce::dsp::IIR::Coefficients::makeLowPass/makeLowShelf/makeHighShelf
    // never receive an out-of-range value (which would produce invalid/NaN
    // coefficients).
    float clampBelowNyquist (float frequencyHz, double rateHz) noexcept
    {
        const auto nyquist = static_cast<float> (rateHz) * 0.5f;
        return juce::jlimit (10.0f, nyquist * 0.9f, frequencyHz);
    }

    // Warmth (0-1) -> HF-rolloff cutoff, log-interpolated between
    // warmthMinLowPassHz (0%, effectively transparent) and
    // warmthMaxLowPassHz (100%, gentle darkening).
    float mapWarmthToLowPassHz (float proportion01, float minHz, float maxHz) noexcept
    {
        const auto clamped = juce::jlimit (0.0f, 1.0f, proportion01);
        return minHz * std::pow (maxHz / minHz, clamped);
    }

    // Warmth (0-1) -> saturator bias, linear from 0 (symmetric) to
    // maxBias (maximally asymmetric).
    float mapWarmthToBias (float proportion01, float maxBias) noexcept
    {
        return juce::jlimit (0.0f, 1.0f, proportion01) * maxBias;
    }

    // Tone tilt (-1 to 1) -> shelf gain in dB, linear.
    float mapTiltToDb (float proportionMinus1To1, float maxTiltDb) noexcept
    {
        return juce::jlimit (-1.0f, 1.0f, proportionMinus1To1) * maxTiltDb;
    }
}

AureateEngine::AureateEngine() = default;

void AureateEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    driveGain.setRampDurationSeconds (smoothingTimeSeconds);
    driveGain.prepare (spec);

    // 4x oversampling (2^2), half-band polyphase IIR: much lower latency
    // than the equiripple FIR alternative for a given stopband quality,
    // which matters here because that latency is exactly what has to be
    // compensated on the dry path below. useIntegerLatency=true so the
    // reported latency (and therefore setLatencySamples()) is an exact
    // integer sample count rather than something we'd have to round.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        spec.numChannels,
        oversamplingFactorPow2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true);
    oversampler->initProcessing (static_cast<size_t> (spec.maximumBlockSize));

    latencySamples = static_cast<int> (std::round (oversampler->getLatencyInSamples()));

    // Warmth's HF-rolloff filter and the Tone tilt shelves run inside the
    // oversampled block (see AureateEngine.h), so they must be prepared at
    // the oversampled rate/block size, not the host's.
    const auto oversamplingMultiplier = 1u << static_cast<unsigned> (oversamplingFactorPow2);
    oversampledRate = sampleRate * static_cast<double> (oversamplingMultiplier);

    juce::dsp::ProcessSpec oversampledSpec;
    oversampledSpec.sampleRate = oversampledRate;
    oversampledSpec.maximumBlockSize = spec.maximumBlockSize * oversamplingMultiplier;
    oversampledSpec.numChannels = spec.numChannels;

    warmthLowPass.prepare (oversampledSpec);
    tiltLowShelf.prepare (oversampledSpec);
    tiltHighShelf.prepare (oversampledSpec);

    outputGain.setRampDurationSeconds (smoothingTimeSeconds);
    outputGain.prepare (spec);

    dryWetMixer.prepare (spec);
    dryWetMixer.setWetLatency (static_cast<float> (latencySamples));

    // juce::dsp::DryWetMixer defaults its internal mix to fully wet (1.0)
    // until told otherwise, and its own reset() (called from our reset()
    // below) snaps its internal dry/wet gain smoothers' *current* value to
    // whatever their *target* happens to be at that moment - it does not
    // know about lastMixProportion. Priming the real target here, before
    // reset() runs, means the mixer is already sitting at the correct dry/
    // wet balance for the very first process() call instead of ramping up
    // from "fully wet" over its internal 50ms default ramp.
    dryWetMixer.setWetMixProportion (lastMixProportion);

    // Re-seed the smoothers at the new rate, but pin current == target to
    // whatever was last requested (defaulting to the ParameterLayout
    // defaults on first prepare) - otherwise the ramp would sweep up from a
    // default-constructed 0 on the very first block.
    const auto warmthLowPassHz = mapWarmthToLowPassHz (lastWarmthProportion01, warmthMinLowPassHz, warmthMaxLowPassHz);
    const auto warmthBias = mapWarmthToBias (lastWarmthProportion01, warmthMaxBias);
    const auto tiltDb = mapTiltToDb (lastTiltProportion, maxTiltDb);

    warmthLowPassHzSmoothed.reset (oversampledRate, smoothingTimeSeconds);
    warmthLowPassHzSmoothed.setCurrentAndTargetValue (warmthLowPassHz);
    warmthBiasSmoothed.reset (sampleRate, smoothingTimeSeconds);
    warmthBiasSmoothed.setCurrentAndTargetValue (warmthBias);
    tiltDbSmoothed.reset (sampleRate, smoothingTimeSeconds);
    tiltDbSmoothed.setCurrentAndTargetValue (tiltDb);
    mixSmoothed.reset (sampleRate, smoothingTimeSeconds);
    mixSmoothed.setCurrentAndTargetValue (lastMixProportion);

    reset();

    // Prime the filter coefficients immediately so the very first
    // process() call runs with correct, non-default coefficients rather
    // than an identity/uninitialised state.
    *warmthLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
        oversampledRate, clampBelowNyquist (warmthLowPassHz, oversampledRate), filterQ);
    *tiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        oversampledRate, clampBelowNyquist (tiltLowShelfHz, oversampledRate), filterQ, juce::Decibels::decibelsToGain (-tiltDb));
    *tiltHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        oversampledRate, clampBelowNyquist (tiltHighShelfHz, oversampledRate), filterQ, juce::Decibels::decibelsToGain (tiltDb));
}

void AureateEngine::reset()
{
    driveGain.reset();

    if (oversampler != nullptr)
        oversampler->reset();

    warmthLowPass.reset();
    tiltLowShelf.reset();
    tiltHighShelf.reset();
    outputGain.reset();
    dryWetMixer.reset();
}

void AureateEngine::setDriveDb (float newDriveDb)
{
    driveGain.setGainDecibels (newDriveDb);
}

void AureateEngine::setWarmthProportion (float newProportion01)
{
    lastWarmthProportion01 = newProportion01;
    warmthLowPassHzSmoothed.setTargetValue (mapWarmthToLowPassHz (newProportion01, warmthMinLowPassHz, warmthMaxLowPassHz));
    warmthBiasSmoothed.setTargetValue (mapWarmthToBias (newProportion01, warmthMaxBias));
}

void AureateEngine::setToneProportion (float newProportionMinus1To1)
{
    lastTiltProportion = newProportionMinus1To1;
    tiltDbSmoothed.setTargetValue (mapTiltToDb (newProportionMinus1To1, maxTiltDb));
}

void AureateEngine::setMixProportion (float newProportion01)
{
    lastMixProportion = newProportion01;
    mixSmoothed.setTargetValue (newProportion01);
}

void AureateEngine::setOutputDb (float newOutputDb)
{
    outputGain.setGainDecibels (newOutputDb);
}

void AureateEngine::process (juce::dsp::AudioBlock<float>& block)
{
    const auto numSamples = block.getNumSamples();

    if (numSamples == 0)
        return;

    // Coefficient recomputation involves trig calls (tan/cos), so the
    // Warmth low-pass and Tone tilt shelves are smoothed and re-derived once
    // per block rather than per sample - a standard real-time-safe
    // compromise for IIR filters, whose coefficients aren't cheap to
    // interpolate directly. Drive/Output still ramp sample-accurately via
    // juce::dsp::Gain's internal SmoothedValue, and the saturator's bias and
    // Mix are re-applied every block below.
    const auto warmthLowPassHz = clampBelowNyquist (
        warmthLowPassHzSmoothed.skip (static_cast<int> (numSamples)), oversampledRate);
    const auto warmthBias = warmthBiasSmoothed.skip (static_cast<int> (numSamples));
    const auto tiltDb = tiltDbSmoothed.skip (static_cast<int> (numSamples));
    const auto wetMix = mixSmoothed.skip (static_cast<int> (numSamples));

    *warmthLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (oversampledRate, warmthLowPassHz, filterQ);
    *tiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        oversampledRate, clampBelowNyquist (tiltLowShelfHz, oversampledRate), filterQ, juce::Decibels::decibelsToGain (-tiltDb));
    *tiltHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        oversampledRate, clampBelowNyquist (tiltHighShelfHz, oversampledRate), filterQ, juce::Decibels::decibelsToGain (tiltDb));
    dryWetMixer.setWetMixProportion (wetMix);

    juce::dsp::ProcessContextReplacing<float> context (block);

    // Capture the pre-processing signal as "dry" before any wet-path
    // processing touches `block`. DryWetMixer internally delays this by
    // getLatencySamples() (set via setWetLatency in prepare()) so it stays
    // time-aligned with the oversampled wet path below.
    dryWetMixer.pushDrySamples (block);

    driveGain.process (context);

    auto oversampledBlock = oversampler->processSamplesUp (block);
    juce::dsp::ProcessContextReplacing<float> oversampledContext (oversampledBlock);

    // Tape-style saturation stage: gentle pre-clip HF rolloff (models tape
    // bias/self-erasure dulling highs before the nonlinearity), then the
    // asymmetric tanh saturator itself. Both driven by Warmth.
    warmthLowPass.process (oversampledContext);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
    {
        auto* channelData = oversampledBlock.getChannelPointer (channel);

        for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
            channelData[sample] = TapeSaturator::processSample (channelData[sample], warmthBias);
    }

    // Console-style tilt Tone, also run inside the oversampled domain
    // (immediately after the saturator, before downsampling).
    tiltLowShelf.process (oversampledContext);
    tiltHighShelf.process (oversampledContext);

    oversampler->processSamplesDown (block);

    dryWetMixer.mixWetSamples (block);

    // Output is a final master trim applied after the dry/wet mix, so it
    // scales the combined (not just wet) signal.
    outputGain.process (context);
}
