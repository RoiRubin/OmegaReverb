#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "dsp/HallReverb.h"

// LED-glass display: RT60 across frequency (from the engine's real in-loop filters) and the wet tone curve
class DecayDisplay : public juce::Component, private juce::Timer
{
public:
    explicit DecayDisplay (std::function<omega::HallParams()> paramSource);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::function<omega::HallParams()> getParams;
    omega::HallParams shown;
};
