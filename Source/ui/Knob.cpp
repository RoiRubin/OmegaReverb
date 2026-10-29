#include "ui/Knob.h"
#include "ui/OmegaLookAndFeel.h"

Knob::Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& labelText)
    : label (labelText)
{
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18);

    if (auto* param = apvts.getParameter (paramId); param != nullptr && param->getLabel().isNotEmpty())
        slider.setTextValueSuffix (" " + param->getLabel());

    // The attachment also sets the value <-> text conversion and double-click-to-default
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
    addAndMakeVisible (slider);
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (omegaui::creamDim);
    g.setFont (omegaui::engravedFont (10.5f, true));
    g.drawText (label, getLocalBounds().removeFromTop (16), juce::Justification::centred);
}

void Knob::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (16));
}
