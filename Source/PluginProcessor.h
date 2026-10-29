#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/HallReverb.h"
#include "Presets.h"

class OmegaReverbProcessor : public juce::AudioProcessor
{
public:
    OmegaReverbProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    // Factory presets are exposed as programs so they also show up in FL Studio's preset menu
    int getNumPrograms() override { return presets.getNumFactory(); }
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override { presets.load (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    omega::HallParams readParams() const;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;

private:
    std::vector<std::atomic<float>*> paramValues;   // parallel to paramSpecs()
    omega::HallReverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmegaReverbProcessor)
};
