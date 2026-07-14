#include "ParameterLayout.h"
#include "ParameterIds.h"

namespace aur
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        //======================================================================
        // Drive: gain into the oversampled tape-style saturator. Kept modest
        // (0-24 dB) - Aureate is a "glue" saturator for orchestral material,
        // not a high-gain distortion.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::drive, 1 },
            "Drive",
            juce::NormalisableRange<float> (0.0f, 24.0f, 0.01f),
            6.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        //======================================================================
        // Warmth: joint asymmetry-bias + pre-clip HF-rolloff amount (see
        // AureateEngine / TapeSaturator). 0% is a symmetric, HF-transparent
        // tanh saturator; 100% is maximally biased and darkened.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::warmth, 1 },
            "Warmth",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            35.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        //======================================================================
        // Tone: console-style tilt EQ. -100% fully dark, 0% flat, +100% fully
        // bright.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::tone, 1 },
            "Tone",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        //======================================================================
        // Mix: dry/wet. Default 100% (fully wet) - glue processing is
        // normally run fully in the signal path, not blended, though the
        // control is there for parallel/New-York-style use.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::mix, 1 },
            "Mix",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        //======================================================================
        // Output: final trim, applied after the dry/wet mix.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::output, 1 },
            "Output",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        return layout;
    }
}
