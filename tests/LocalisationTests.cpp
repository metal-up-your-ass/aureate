#include "presets/Localisation.h"

#include <juce_core/juce_core.h>

#include <BinaryData.h>

#include <catch2/catch_test_macros.hpp>

// M2 i18n frame tests (.scaffold/specs/preset-system-m2.md's "I18N" section):
// the German mapping parses, every PresetBar-visible TRANS() key has a
// translation, and core/DSP parameter names are verifiably NOT translated.
namespace
{
    // Every literal string PresetBar.cpp/PresetManager.cpp wrap in TRANS() -
    // kept as an independent list here (not derived from the source) so this
    // test fails loudly if a new frame string is ever added to PresetBar/
    // PresetManager without a matching resources/i18n/de.txt entry.
    const char* const presetFrameKeys[] = {
        "Init",
        "Factory",
        "User",
        "Set current as default",
        "Save",
        "Save As...",
        "Delete",
        "Import...",
        "Export...",
        "Enter a name for the new preset:",
        "Preset name",
        "Cancel",
        "Import a preset or preset bank...",
        "Import failed",
        "Export preset...",
        "This file is not a valid preset.",
        "This preset was saved by an incompatible version of the preset format.",
        "This preset file belongs to a different plugin.",
    };

    // Core/DSP terminology (parameter names, units) - per the spec, these
    // must NEVER appear as translated keys anywhere in the mapping.
    const char* const parameterNames[] = {
        "Drive", "Warmth", "Tone", "Mix", "Output", "Bias",
        "Wow", "Flutter", "Hiss", "Character", "HF Trim", "LF Trim",
    };
}

TEST_CASE ("Localisation: the German mapping parses without error", "[i18n]")
{
    const auto text = juce::String::fromUTF8 (BinaryData::de_txt, BinaryData::de_txtSize);
    REQUIRE (text.isNotEmpty());

    // LocalisedStrings's constructor does the actual line-based parsing;
    // constructing it (and reading back a known key) is the practical
    // "parses without error" proof - a malformed mapping either throws
    // (asserts in debug) or simply fails every lookup below.
    juce::LocalisedStrings localised (text, true);
    CHECK (localised.translate ("Save") == juce::String ("Speichern"));
}

TEST_CASE ("Localisation: every PresetBar/PresetManager TRANS() key has a German translation", "[i18n]")
{
    const auto text = juce::String::fromUTF8 (BinaryData::de_txt, BinaryData::de_txtSize);
    juce::LocalisedStrings localised (text, true);

    // translate(text, resultIfNotFound) with a sentinel value is the
    // reliable "is this key actually present in the mapping" check - unlike
    // comparing translate(text) against the original string, this is
    // correct even for a key whose German value happens to be identical to
    // its English source (e.g. "Init" = "Init" - a real, present mapping
    // entry, not a missing one).
    constexpr const char* sentinel = "__MISSING_KEY_SENTINEL__";

    for (const auto* key : presetFrameKeys)
    {
        CAPTURE (key);
        CHECK (localised.translate (juce::String (key), sentinel) != juce::String (sentinel));
    }
}

TEST_CASE ("Localisation: core/DSP parameter names are verifiably NOT present in the German mapping", "[i18n]")
{
    const auto text = juce::String::fromUTF8 (BinaryData::de_txt, BinaryData::de_txtSize);
    juce::LocalisedStrings localised (text, true);

    constexpr const char* sentinel = "__MISSING_KEY_SENTINEL__";

    for (const auto* name : parameterNames)
    {
        CAPTURE (name);
        // No entry for this key => translate() falls through to the sentinel.
        CHECK (localised.translate (juce::String (name), sentinel) == juce::String (sentinel));
    }
}

TEST_CASE ("Localisation: installLocalisation() is idempotent and safe to call with null/empty data", "[i18n]")
{
    CHECK_NOTHROW (basilica::presets::installLocalisation (nullptr, 0));
    CHECK_NOTHROW (basilica::presets::installLocalisation (BinaryData::de_txt, BinaryData::de_txtSize));
    CHECK_NOTHROW (basilica::presets::installLocalisation (BinaryData::de_txt, BinaryData::de_txtSize));
}
