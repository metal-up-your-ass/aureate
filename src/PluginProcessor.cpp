#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterIds.h"
#include "params/ParameterLayout.h"

#include <BinaryData.h>

namespace
{
    // The small, Aureate-specific config surface PresetManager needs (see
    // src/presets/PresetManager.h's class docs) - everything else about the
    // preset system is fully generic and portable to sibling plugins (see
    // nave's docs/preset-system-notes.md, the M2 pilot this was copied
    // from).
    basilica::presets::PresetManagerConfig makePresetManagerConfig()
    {
        // JucePlugin_CFBundleIdentifier expands to a raw (unquoted) token
        // sequence, not a string literal - JUCE_STRINGIFY() is the
        // documented way to turn it into one. This is always
        // "com.yvesvogl.aureate" here (BUNDLE_ID in CMakeLists.txt),
        // matching the "plugin" field baked into every
        // presets/factory/*.json file.
        basilica::presets::PresetManagerConfig config;
        config.pluginId = JUCE_STRINGIFY (JucePlugin_CFBundleIdentifier);
        config.pluginName = JucePlugin_Name;
        config.manufacturerName = "Yves Vogl";
        config.pluginVersion = JucePlugin_VersionString;
        // userPresetsDirectoryOverrideForTests intentionally left
        // default-constructed (empty) - production instances always use the
        // real platform-standard preset location (see PresetManager.h).
        return config;
    }

    // BinaryData symbol names are derived from the presets/factory/*.json
    // file names passed to juce_add_binary_data() in CMakeLists.txt (dots
    // become underscores) - this list must stay in sync with that SOURCES
    // list. Order here only affects factory-preset iteration order before
    // getAllPresets() re-sorts alphabetically, so it isn't otherwise
    // significant.
    std::vector<basilica::presets::FactoryPresetAsset> makeFactoryPresetAssets()
    {
        return {
            { BinaryData::default_json, BinaryData::default_jsonSize },
            { BinaryData::stringSectionGlue_json, BinaryData::stringSectionGlue_jsonSize },
            { BinaryData::brassBloom_json, BinaryData::brassBloom_jsonSize },
            { BinaryData::choirWarmth_json, BinaryData::choirWarmth_jsonSize },
            { BinaryData::orchestralSubmixCohesion_json, BinaryData::orchestralSubmixCohesion_jsonSize },
            { BinaryData::masterGlueSubtle_json, BinaryData::masterGlueSubtle_jsonSize },
            { BinaryData::vintageTapePad_json, BinaryData::vintageTapePad_jsonSize },
            { BinaryData::valvePush_json, BinaryData::valvePush_jsonSize },
            { BinaryData::parallelGritNewYork_json, BinaryData::parallelGritNewYork_jsonSize },
            { BinaryData::consoleSummingSheen_json, BinaryData::consoleSummingSheen_jsonSize },
            { BinaryData::airAndWeight_json, BinaryData::airAndWeight_jsonSize },
        };
    }

    // v0.1.0 state migration helper (see setStateInformation() below): finds
    // the <PARAM id="..." value="..."/> child element for a given parameter
    // ID inside an APVTS state XmlElement, or nullptr if none exists. APVTS
    // serialises its state as a "PARAMETERS" root with direct "PARAM"
    // children carrying "id"/"value" attributes (JUCE 8.0.14,
    // juce_AudioProcessorValueTreeState.cpp's updateParameterConnectionsToChildTrees()/
    // idPropertyID/valuePropertyID) - this walks that exact shape.
    juce::XmlElement* findParamElement (juce::XmlElement& stateXml, const juce::String& paramId)
    {
        for (auto* child : stateXml.getChildIterator())
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == paramId)
                return child;

        return nullptr;
    }
}

//==============================================================================
AureateAudioProcessor::AureateAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager (apvts, makePresetManagerConfig(), makeFactoryPresetAssets())
{
    driveDb = apvts.getRawParameterValue (ParamIDs::drive);
    warmthPercent = apvts.getRawParameterValue (ParamIDs::warmth);
    tonePercent = apvts.getRawParameterValue (ParamIDs::tone);
    mixPercent = apvts.getRawParameterValue (ParamIDs::mix);
    outputDb = apvts.getRawParameterValue (ParamIDs::output);
    biasPercent = apvts.getRawParameterValue (ParamIDs::bias);
    wowPercent = apvts.getRawParameterValue (ParamIDs::wow);
    flutterPercent = apvts.getRawParameterValue (ParamIDs::flutter);
    hissPercent = apvts.getRawParameterValue (ParamIDs::hiss);
    characterIndex = apvts.getRawParameterValue (ParamIDs::character);
    hfTrimDb = apvts.getRawParameterValue (ParamIDs::hfTrim);
    lfTrimDb = apvts.getRawParameterValue (ParamIDs::lfTrim);

    jassert (driveDb != nullptr);
    jassert (warmthPercent != nullptr);
    jassert (tonePercent != nullptr);
    jassert (mixPercent != nullptr);
    jassert (outputDb != nullptr);
    jassert (biasPercent != nullptr);
    jassert (wowPercent != nullptr);
    jassert (flutterPercent != nullptr);
    jassert (hissPercent != nullptr);
    jassert (characterIndex != nullptr);
    jassert (hfTrimDb != nullptr);
    jassert (lfTrimDb != nullptr);

    // M2 default resolution: user "Default" preset > factory "Default"
    // preset > the ParameterLayout defaults apvts was just constructed with
    // above (see PresetManager::applyStartupDefault()'s docs). Aureate's
    // factory "Default" preset (presets/factory/default.json, category
    // "Init") is the certified passthrough state - every parameter at its
    // off/neutral position - so a fresh instance's out-of-the-box sound is
    // unchanged from what it always was, now reachable as an explicit,
    // one-click preset too (see docs/presets.md).
    presetManager.applyStartupDefault();
}

AureateAudioProcessor::~AureateAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AureateAudioProcessor::createParameterLayout()
{
    return aur::createParameterLayout();
}

//==============================================================================
const juce::String AureateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AureateAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AureateAudioProcessor::producesMidi() const
{
    return false;
}

bool AureateAudioProcessor::isMidiEffect() const
{
    return false;
}

double AureateAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AureateAudioProcessor::getNumPrograms()
{
    return 1;
}

int AureateAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AureateAudioProcessor::setCurrentProgram (int)
{
}

const juce::String AureateAudioProcessor::getProgramName (int)
{
    return {};
}

void AureateAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void AureateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    // Seed the engine's parameters from the current APVTS state before
    // prepare() primes the filter coefficients, so the very first block
    // after prepareToPlay() already reflects the host/session's actual
    // parameter values rather than the engine's built-in defaults.
    pushParametersToEngine();

    engine.prepare (spec);

    // Oversampling (4x, applied around the tape saturator and tilt Tone
    // stage) is the only source of reported latency; the dry path is
    // compensated against it internally by AureateEngine's DryWetMixer (see
    // docs/architecture.md).
    setLatencySamples (engine.getLatencySamples());
}

void AureateAudioProcessor::releaseResources()
{
}

void AureateAudioProcessor::reset()
{
    engine.reset();
}

bool AureateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();

    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn = layouts.getMainInputChannelSet();

    if (mainOut != mono && mainOut != stereo)
        return false;

    if (mainOut != mainIn)
        return false;

    return true;
}

void AureateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Buses are constrained to in == out (mono or stereo), so this is
    // normally a no-op, but it's cheap insurance against stray channels.
    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    pushParametersToEngine();

    juce::dsp::AudioBlock<float> block (buffer);
    engine.process (block);
}

void AureateAudioProcessor::pushParametersToEngine()
{
    engine.setDriveDb (driveDb->load (std::memory_order_relaxed));
    engine.setWarmthProportion (warmthPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setToneProportion (tonePercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setMixProportion (mixPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setOutputDb (outputDb->load (std::memory_order_relaxed));
    engine.setBiasProportion (biasPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setWowProportion (wowPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setFlutterProportion (flutterPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setHissProportion (hissPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setCharacter (static_cast<TapeSaturator::Model> (juce::roundToInt (
        characterIndex->load (std::memory_order_relaxed))));
    engine.setHfTrimDb (hfTrimDb->load (std::memory_order_relaxed));
    engine.setLfTrimDb (lfTrimDb->load (std::memory_order_relaxed));
}

//==============================================================================
bool AureateAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AureateAudioProcessor::createEditor()
{
    return new AureateAudioProcessorEditor (*this);
}

//==============================================================================
void AureateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    const std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AureateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState == nullptr || ! xmlState->hasTagName (apvts.state.getType()))
        return;

    // v0.1.0 -> v0.2.0 state migration (docs/design-brief.md §7, binding
    // "tolerant import" policy): v0.1.0 saved a single "wow_flutter"
    // parameter, which v0.2.0 splits into independent "wow"/"flutter"
    // parameters (ParamIDs::wow/ParamIDs::flutter). A pre-split state still
    // carries a <PARAM id="wow_flutter" .../> child and none for "wow"/
    // "flutter" (which didn't exist yet in v0.1.0's layout) - detect that
    // shape and add both new PARAM entries at the legacy value before
    // handing the tree to APVTS, so an old session retains a recognisable
    // (if not identical - see the brief's honesty note on Character's bias
    // ceilings changing too) character rather than silently resetting
    // Wow/Flutter to 0% on load. A state that already has "wow"/"flutter"
    // entries (i.e. already v0.2.0-shaped) is left untouched.
    if (auto* legacyWowFlutterParam = findParamElement (*xmlState, ParamIDs::legacyWowFlutter))
    {
        const auto legacyValue = legacyWowFlutterParam->getStringAttribute ("value");

        if (findParamElement (*xmlState, ParamIDs::wow) == nullptr)
        {
            auto* migratedWow = xmlState->createNewChildElement ("PARAM");
            migratedWow->setAttribute ("id", ParamIDs::wow);
            migratedWow->setAttribute ("value", legacyValue);
        }

        if (findParamElement (*xmlState, ParamIDs::flutter) == nullptr)
        {
            auto* migratedFlutter = xmlState->createNewChildElement ("PARAM");
            migratedFlutter->setAttribute ("id", ParamIDs::flutter);
            migratedFlutter->setAttribute ("value", legacyValue);
        }
    }

    apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AureateAudioProcessor();
}
