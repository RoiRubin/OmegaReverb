#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "ui/DecayDisplay.h"
#include "ui/Knob.h"
#include "ui/OmegaLookAndFeel.h"
#include "ui/PresetBar.h"

class OmegaReverbEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OmegaReverbEditor (OmegaReverbProcessor&);
    ~OmegaReverbEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Section
    {
        juce::String title;
        int row;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> area;
    };

    void timerCallback() override;

    OmegaLookAndFeel lookAndFeel;   // first, so it outlives every child that uses it
    PresetBar presetBar;
    DecayDisplay display;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;
    juce::Image faceplate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmegaReverbEditor)
};
