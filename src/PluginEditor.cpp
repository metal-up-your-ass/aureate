#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "params/ParameterIds.h"
#include "presets/Localisation.h"

#include <BinaryData.h>

namespace
{
    constexpr int knobSize = 90;
    constexpr int textBoxHeight = 20;
    constexpr int labelHeight = 20;
    constexpr int margin = 16;
    constexpr int slotWidth = knobSize + margin;
    constexpr int columns = 6;
    constexpr int numControls = 12;
    constexpr int rows = (numControls + columns - 1) / columns; // ceil
    constexpr int presetBarHeight = 28;
    constexpr int editorWidth = margin * 2 + columns * slotWidth - margin;
    constexpr int editorHeight = margin * 2 + presetBarHeight + margin + rows * (labelHeight + knobSize + textBoxHeight + margin) - margin;

    // M2 i18n frame (.scaffold/specs/preset-system-m2.md): selects German
    // (resources/i18n/de.txt) or falls through to English, once, at editor
    // construction - see Localisation.h's docs. `presetBar` is a member
    // initialised via the constructor's initialiser list, and its own
    // constructor already calls TRANS() on every button label - member
    // initialisers run in declaration order regardless of the order they're
    // written in, so this helper (called from presetBar's own initialiser
    // expression below) is what actually guarantees installLocalisation()
    // runs before presetBar exists, not an installLocalisation() call in the
    // constructor *body*, which would run too late.
    basilica::presets::PresetManager& initLocalisationThenGetPresetManager (AureateAudioProcessor& processor)
    {
        basilica::presets::installLocalisation (BinaryData::de_txt, BinaryData::de_txtSize);
        return processor.presetManager;
    }
}

AureateAudioProcessorEditor::AureateAudioProcessorEditor (AureateAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      audioProcessor (processorToEdit),
      presetBar (initLocalisationThenGetPresetManager (processorToEdit))
{
    addAndMakeVisible (presetBar);

    // Signal-flow order (see docs/architecture.md).
    configureKnob (wowKnob, ParamIDs::wow, "Wow");
    configureKnob (flutterKnob, ParamIDs::flutter, "Flutter");
    configureKnob (driveKnob, ParamIDs::drive, "Drive");
    configureKnob (warmthKnob, ParamIDs::warmth, "Warmth");
    configureKnob (biasKnob, ParamIDs::bias, "Bias");
    configureChoice (characterChoice, ParamIDs::character, "Character");
    configureKnob (toneKnob, ParamIDs::tone, "Tone");
    configureKnob (hfTrimKnob, ParamIDs::hfTrim, "HF Trim");
    configureKnob (lfTrimKnob, ParamIDs::lfTrim, "LF Trim");
    configureKnob (hissKnob, ParamIDs::hiss, "Hiss");
    configureKnob (mixKnob, ParamIDs::mix, "Mix");
    configureKnob (outputKnob, ParamIDs::output, "Output");

    setResizable (false, false);
    setSize (editorWidth, editorHeight);
}

AureateAudioProcessorEditor::~AureateAudioProcessorEditor() = default;

void AureateAudioProcessorEditor::configureKnob (Knob& knob, const juce::String& parameterId, const juce::String& labelText)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, knobSize, textBoxHeight);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    // false => label sits above the slider it tracks; JUCE repositions it
    // automatically whenever the slider's bounds change, so resized() only
    // needs to place the sliders/combo boxes themselves.
    knob.label.attachToComponent (&knob.slider, false);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, parameterId, knob.slider);
}

void AureateAudioProcessorEditor::configureChoice (Choice& choice, const juce::String& parameterId, const juce::String& labelText)
{
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter (parameterId)))
        choice.box.addItemList (param->choices, 1);

    addAndMakeVisible (choice.box);

    choice.label.setText (labelText, juce::dontSendNotification);
    choice.label.setJustificationType (juce::Justification::centred);
    choice.label.attachToComponent (&choice.box, false);
    addAndMakeVisible (choice.label);

    choice.attachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, parameterId, choice.box);
}

void AureateAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (margin);

    presetBar.setBounds (bounds.removeFromTop (presetBarHeight));
    bounds.removeFromTop (margin);

    bounds.removeFromTop (labelHeight); // room for the attached labels above each control

    // A simple wrapping grid, `columns` wide, in signal-flow order. Controls
    // whose bounds happen to need slightly different heights (the combo box
    // vs the rotary sliders) are each given the same row height/cell for
    // simplicity - this is deliberately a plain, functional v0.1 layout (see
    // PluginEditor.h), not the final GUI.
    const auto rowHeight = labelHeight + knobSize + textBoxHeight + margin;

    auto placeInGrid = [&] (juce::Component& component, int index, int componentHeight)
    {
        const auto column = index % columns;
        const auto row = index / columns;

        component.setBounds (bounds.getX() + column * slotWidth,
                              bounds.getY() + row * rowHeight,
                              knobSize,
                              componentHeight);
    };

    int index = 0;
    for (auto* knob : { &wowKnob, &flutterKnob, &driveKnob, &warmthKnob, &biasKnob })
        placeInGrid (knob->slider, index++, knobSize + textBoxHeight);

    placeInGrid (characterChoice.box, index++, textBoxHeight);

    for (auto* knob : { &toneKnob, &hfTrimKnob, &lfTrimKnob, &hissKnob, &mixKnob, &outputKnob })
        placeInGrid (knob->slider, index++, knobSize + textBoxHeight);
}
