#pragma once

#include <juce_dsp/juce_dsp.h>

#include "TapeSaturator.h"

// The complete Aureate signal path, independent of juce::AudioProcessor so
// it can be exercised directly by unit tests without instantiating a full
// plugin (see tests/EngineTests.cpp). Owns all DSP state; every buffer/
// filter/oversampler is allocated in prepare() and never reallocated on the
// audio thread.
//
// Signal flow (see docs/architecture.md for the full diagram and the
// latency-compensation rationale):
//
//   input -> Drive -> [4x oversampled: Warmth HF-rolloff -> tape saturator
//         -> console tilt Tone] -> downsample -> Dry/Wet mix -> Output trim
//
// The Warmth HF-rolloff filter and the Tone tilt shelves both run inside the
// oversampled domain, alongside the saturator - this keeps their coefficient
// updates and the nonlinearity's harmonics all generated/filtered at 4x the
// host sample rate before a single downsample step, rather than filtering
// twice at two different rates. Drive (pre-oversampling) and Output
// (post-mix) are plain gain stages at the host sample rate.
//
// The dry path is delay-compensated against the oversampler's reported
// latency via juce::dsp::DryWetMixer, so Mix at 0% is a sample-accurate
// (once shifted by getLatencySamples()) passthrough of the input - this is
// what the plugin's null test (tests/EngineTests.cpp) exercises. Output is
// a final master trim applied after the dry/wet mix, so it affects the
// blended signal as a whole rather than only the wet path.
class AureateEngine
{
public:
    AureateEngine();

    // Allocates all DSP state. Must be called (and completed) before the
    // first process() call, and again whenever sample rate/block size/
    // channel count change.
    void prepare (const juce::dsp::ProcessSpec& spec);

    // Clears all filter/oversampler/delay-line state without deallocating.
    // Safe to call from the audio thread (e.g. on playback stop/loop).
    void reset();

    // Processes `block` in place. `block` must have at most the maximum
    // sample/channel counts declared to prepare(); a zero-sample block is a
    // safe no-op. No allocation occurs here.
    void process (juce::dsp::AudioBlock<float>& block);

    // Parameter setters, in real units (dB, 0-1/-1-1 proportion). Safe to
    // call every block from the audio thread - no allocation/locks. Drive
    // and Output are smoothed by the underlying juce::dsp::Gain ramp;
    // Warmth/Tone/Mix are smoothed internally and re-applied once per block
    // (see process()).
    void setDriveDb (float newDriveDb);
    void setWarmthProportion (float newProportion01);
    void setToneProportion (float newProportionMinus1To1);
    void setMixProportion (float newProportion01);
    void setOutputDb (float newOutputDb);

    // Oversampling latency in samples, valid after prepare() has run.
    int getLatencySamples() const noexcept { return latencySamples; }

private:
    static constexpr int oversamplingFactorPow2 = 2; // 2^2 = 4x oversampling
    static constexpr double smoothingTimeSeconds = 0.05;

    // Warmth mapping: 0% -> no audible rolloff (cutoff pinned high, near the
    // oversampled Nyquist) and a symmetric (unbiased) saturator; 100% ->
    // gentle darkening (cutoff down to 3 kHz) and maximally biased/
    // asymmetric saturation.
    static constexpr float warmthMinLowPassHz = 20000.0f;
    static constexpr float warmthMaxLowPassHz = 3000.0f;
    static constexpr float warmthMaxBias = 0.3f;

    // Tilt Tone mapping: +/-100% maps to +/-maxTiltDb at each shelf, in
    // opposite directions, so 0% leaves both shelves at 0 dB (flat/unity).
    static constexpr float maxTiltDb = 6.0f;
    static constexpr float tiltLowShelfHz = 200.0f;
    static constexpr float tiltHighShelfHz = 4000.0f;
    // Butterworth (maximally-flat) Q for both the Warmth low-pass and the
    // Tone shelves.
    static constexpr float filterQ = juce::MathConstants<float>::sqrt2 / 2.0f;

    double sampleRate = 44100.0;
    double oversampledRate = 44100.0;

    juce::dsp::Gain<float> driveGain;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // These three run inside the oversampled block (see class comment).
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> warmthLowPass;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> tiltLowShelf;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> tiltHighShelf;

    juce::dsp::Gain<float> outputGain;

    // Sized generously above any realistic oversampling latency (4x
    // half-band polyphase IIR latency is on the order of tens of samples)
    // so setWetLatency() never exceeds the mixer's internal delay-line
    // capacity regardless of sample rate.
    juce::dsp::DryWetMixer<float> dryWetMixer { 1024 };

    // Warmth's low-pass cutoff uses multiplicative smoothing (appropriate
    // for a quantity perceived logarithmically, like Hz); bias/tilt/mix use
    // linear smoothing.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> warmthLowPassHzSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> warmthBiasSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> tiltDbSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;

    // Last commanded values (ParameterLayout defaults until a setter is
    // called), re-applied to the smoothers on every prepare() so re-prepare
    // (sample-rate change, etc.) never resets a live parameter back to a
    // default or lets a smoother start from an invalid 0 Hz.
    float lastWarmthProportion01 = 0.35f;
    float lastTiltProportion = 0.0f;
    float lastMixProportion = 1.0f;

    int latencySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AureateEngine)
};
