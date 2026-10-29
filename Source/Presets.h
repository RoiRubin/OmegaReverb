#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/HallReverb.h"

struct FactoryPreset
{
    const char* name;
    omega::HallParams params;
};

const std::vector<FactoryPreset>& factoryPresets();

// Factory presets followed by user presets (XML files in %APPDATA%/OmegaReverb/Presets).
// The current preset's name is stored in the plugin state, so it survives project reloads.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);

    juce::StringArray getPresetNames() const;
    int getNumFactory() const;
    int getCurrentIndex() const;   // -1 if the stored name no longer exists
    juce::String getCurrentName() const;
    bool isModified() const;       // parameters differ from the loaded preset

    void load (int index);
    void loadNext (int delta);
    bool saveUser (const juce::String& name);

    static juce::File userFolder();

private:
    void refreshUserList();
    omega::HallParams presetParams (int index) const;
    omega::HallParams currentParams() const;
    void setCurrentName (const juce::String&);

    juce::AudioProcessorValueTreeState& apvts;
    juce::Array<juce::File> userFiles;

    mutable juce::String cachedName;   // preset whose values are in cachedParams
    mutable omega::HallParams cachedParams;
};
