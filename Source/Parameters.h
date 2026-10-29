#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/HallReverb.h"

struct ParamSpec
{
    const char* id;
    const char* name;
    float min, max;
    float skewCentre;   // 0 = linear range
    const char* unit;
    float omega::HallParams::* field;   // where the value goes; its HallParams default is the parameter default
};

const std::vector<ParamSpec>& paramSpecs();
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
