cmake_minimum_required(VERSION 3.22)

# Bump this with every update: it sets the VST3 version FL Studio reports and the one shown in the editor
project(OmegaReverb VERSION 0.3.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Plain C++ reverb engine (no JUCE), shared by the plugin and the offline render/measure tool
add_library(OmegaDSP STATIC
    Source/dsp/DelayLine.h
    Source/dsp/Filters.h
    Source/dsp/RandomMod.h
    Source/dsp/HallReverb.h  Source/dsp/HallReverb.cpp)

target_include_directories(OmegaDSP PUBLIC Source)
target_compile_features(OmegaDSP PUBLIC cxx_std_17)

juce_add_plugin(OmegaReverb
    COMPANY_NAME "Me"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Bbxd
    PLUGIN_CODE Omrv
    FORMATS VST3
    VST3_CATEGORIES Fx Reverb
    COPY_PLUGIN_AFTER_BUILD TRUE
    PRODUCT_NAME "OmegaReverb")

set(OMEGA_PLUGIN_SOURCES
    Source/PluginProcessor.h  Source/PluginProcessor.cpp
    Source/PluginEditor.h     Source/PluginEditor.cpp
    Source/Parameters.h       Source/Parameters.cpp
    Source/Presets.h          Source/Presets.cpp

    Source/ui/OmegaLookAndFeel.h  Source/ui/OmegaLookAndFeel.cpp
    Source/ui/Knob.h              Source/ui/Knob.cpp
    Source/ui/DecayDisplay.h      Source/ui/DecayDisplay.cpp
    Source/ui/PresetBar.h         Source/ui/PresetBar.cpp)

target_sources(OmegaReverb PRIVATE ${OMEGA_PLUGIN_SOURCES})

target_compile_definitions(OmegaReverb PUBLIC
    OMEGA_VERSION="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(OmegaReverb
    PRIVATE OmegaDSP juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Offline tool: renders impulse responses to WAV and prints objective measurements
add_executable(OmegaRender tools/OmegaRender.cpp)
target_link_libraries(OmegaRender PRIVATE OmegaDSP)

# Developer tool: renders the editor to a PNG, so the interface can be checked without a DAW
juce_add_console_app(OmegaSnapshot PRODUCT_NAME "OmegaSnapshot")
target_sources(OmegaSnapshot PRIVATE tools/OmegaSnapshot.cpp ${OMEGA_PLUGIN_SOURCES})
target_include_directories(OmegaSnapshot PRIVATE Source)
target_compile_definitions(OmegaSnapshot PRIVATE
    OMEGA_VERSION="${PROJECT_VERSION}" JucePlugin_Name="OmegaReverb"
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(OmegaSnapshot PRIVATE OmegaDSP juce::juce_audio_utils juce::juce_recommended_config_flags)
