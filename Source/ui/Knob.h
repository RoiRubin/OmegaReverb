#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Engraved label above a rotary slider with an LED value readout, attached to one parameter
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& labelText);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
