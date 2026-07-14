#include "PluginProcessor.h"
#include "params/ParameterIds.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("State round-trip preserves non-default values of every parameter", "[state]")
{
    AureateAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto* driveParam = processor.apvts.getParameter (ParamIDs::drive);
    auto* warmthParam = processor.apvts.getParameter (ParamIDs::warmth);
    auto* toneParam = processor.apvts.getParameter (ParamIDs::tone);
    auto* mixParam = processor.apvts.getParameter (ParamIDs::mix);
    auto* outputParam = processor.apvts.getParameter (ParamIDs::output);

    REQUIRE (driveParam != nullptr);
    REQUIRE (warmthParam != nullptr);
    REQUIRE (toneParam != nullptr);
    REQUIRE (mixParam != nullptr);
    REQUIRE (outputParam != nullptr);

    driveParam->setValueNotifyingHost (driveParam->convertTo0to1 (17.0f));
    warmthParam->setValueNotifyingHost (warmthParam->convertTo0to1 (72.0f));
    toneParam->setValueNotifyingHost (toneParam->convertTo0to1 (-40.0f));
    mixParam->setValueNotifyingHost (mixParam->convertTo0to1 (63.0f));
    outputParam->setValueNotifyingHost (outputParam->convertTo0to1 (-4.5f));

    const auto savedDrive = driveParam->getValue();
    const auto savedWarmth = warmthParam->getValue();
    const auto savedTone = toneParam->getValue();
    const auto savedMix = mixParam->getValue();
    const auto savedOutput = outputParam->getValue();

    juce::MemoryBlock savedState;
    processor.getStateInformation (savedState);
    REQUIRE (savedState.getSize() > 0);

    // Reset every parameter back to its default before restoring, so the
    // round-trip assertion below can't pass by accident.
    driveParam->setValueNotifyingHost (driveParam->getDefaultValue());
    warmthParam->setValueNotifyingHost (warmthParam->getDefaultValue());
    toneParam->setValueNotifyingHost (toneParam->getDefaultValue());
    mixParam->setValueNotifyingHost (mixParam->getDefaultValue());
    outputParam->setValueNotifyingHost (outputParam->getDefaultValue());

    REQUIRE (driveParam->getValue() != Catch::Approx (savedDrive));
    REQUIRE (warmthParam->getValue() != Catch::Approx (savedWarmth));
    REQUIRE (toneParam->getValue() != Catch::Approx (savedTone));
    REQUIRE (mixParam->getValue() != Catch::Approx (savedMix));
    REQUIRE (outputParam->getValue() != Catch::Approx (savedOutput));

    processor.setStateInformation (savedState.getData(), static_cast<int> (savedState.getSize()));

    CHECK (driveParam->getValue() == Catch::Approx (savedDrive).margin (1e-6));
    CHECK (warmthParam->getValue() == Catch::Approx (savedWarmth).margin (1e-6));
    CHECK (toneParam->getValue() == Catch::Approx (savedTone).margin (1e-6));
    CHECK (mixParam->getValue() == Catch::Approx (savedMix).margin (1e-6));
    CHECK (outputParam->getValue() == Catch::Approx (savedOutput).margin (1e-6));
}
