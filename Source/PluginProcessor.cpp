#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

OmegaReverbProcessor::OmegaReverbProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OmegaReverb", createParameterLayout()),
      presets (apvts)
{
    for (const auto& spec : paramSpecs())
        paramValues.push_back (apvts.getRawParameterValue (spec.id));
}

omega::HallParams OmegaReverbProcessor::readParams() const
{
    omega::HallParams p;
    const auto& specs = paramSpecs();

    for (size_t i = 0; i < specs.size(); ++i)
        p.*(specs[i].field) = paramValues[i]->load();

    return p;
}

void OmegaReverbProcessor::prepareToPlay (double sampleRate, int)
{
    reverb.prepare (sampleRate);
    reverb.setParams (readParams());
}

bool OmegaReverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void OmegaReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    reverb.setParams (readParams());
    reverb.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
}

double OmegaReverbProcessor::getTailLengthSeconds() const
{
    const auto p = readParams();
    return p.predelayMs * 0.001 + p.decaySec * std::max (p.bassMult, 1.0f) * 1.5;
}

int OmegaReverbProcessor::getCurrentProgram()
{
    const int index = presets.getCurrentIndex();
    return juce::isPositiveAndBelow (index, presets.getNumFactory()) ? index : 0;
}

const juce::String OmegaReverbProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, presets.getNumFactory()) ? juce::String (factoryPresets()[(size_t) index].name)
                                                                     : juce::String();
}

juce::AudioProcessorEditor* OmegaReverbProcessor::createEditor()
{
    return new OmegaReverbEditor (*this);
}

void OmegaReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OmegaReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OmegaReverbProcessor();
}
