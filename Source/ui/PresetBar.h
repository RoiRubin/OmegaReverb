#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Presets.h"

// Prev / LED preset list / next / save. Saving turns the LED display into a name field (Enter saves, Esc cancels).
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (PresetManager&);

    void resized() override;
    void refresh();   // sync the list, current preset and its modified marker

private:
    void startSaving();
    void finishSaving (bool commit);

    PresetManager& presets;
    juce::TextButton prev { "<" }, next { ">" }, save { "SAVE" };
    juce::ComboBox list;
    juce::TextEditor nameEditor;
    juce::StringArray shownNames;
};
