#include "Parameters.h"

const std::vector<ParamSpec>& paramSpecs()
{
    using P = omega::HallParams;

    static const std::vector<ParamSpec> specs {
        { "mix",       "Mix",             0.0f,    100.0f,   0.0f,    "%",  &P::mix },
        { "predelay",  "Pre-Delay",       0.0f,    500.0f,   60.0f,   "ms", &P::predelayMs },
        { "decay",     "Decay",           0.2f,    20.0f,    3.0f,    "s",  &P::decaySec },
        { "size",      "Size",            0.0f,    100.0f,   0.0f,    "%",  &P::size },
        { "earlyDiff", "Early Diffusion", 0.0f,    100.0f,   0.0f,    "%",  &P::earlyDiff },
        { "lateDiff",  "Late Diffusion",  0.0f,    100.0f,   0.0f,    "%",  &P::lateDiff },
        { "modRate",   "Mod Rate",        0.05f,   5.0f,     0.8f,    "Hz", &P::modRateHz },
        { "modDepth",  "Mod Depth",       0.0f,    100.0f,   0.0f,    "%",  &P::modDepth },
        { "bassMult",  "Bass Mult",       0.25f,   4.0f,     1.0f,    "x",  &P::bassMult },
        { "bassXover", "Bass Xover",      50.0f,   1000.0f,  250.0f,  "Hz", &P::bassXover },
        { "highMult",  "High Mult",       0.1f,    1.0f,     0.0f,    "x",  &P::highMult },
        { "highXover", "High Xover",      1000.0f, 16000.0f, 5000.0f, "Hz", &P::highXover },
        { "lowCut",    "Low Cut",         10.0f,   1000.0f,  100.0f,  "Hz", &P::lowCut },
        { "highCut",   "High Cut",        1000.0f, 20000.0f, 6000.0f, "Hz", &P::highCut },
    };

    return specs;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    const omega::HallParams defaults;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& spec : paramSpecs())
    {
        juce::NormalisableRange<float> range (spec.min, spec.max);
        if (spec.skewCentre > 0.0f)
            range.setSkewForCentre (spec.skewCentre);

        const auto attributes = juce::AudioParameterFloatAttributes()
                                    .withLabel (spec.unit)
                                    .withStringFromValueFunction ([] (float v, int)
                                    {
                                        return juce::String (v, v < 10.0f ? 2 : (v < 100.0f ? 1 : 0));
                                    });

        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { spec.id, 1 }, spec.name, range,
                                                                 defaults.*(spec.field), attributes));
    }

    return layout;
}
