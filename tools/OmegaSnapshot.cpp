// Developer tool: renders the plugin editor to a PNG so the interface can be checked without a host.
// Usage: OmegaSnapshot <output.png> [preset index]

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    const juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "snapshot.png"));

    OmegaReverbProcessor processor;
    if (argc > 2)
        processor.setCurrentProgram (juce::String (argv[2]).getIntValue());

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);

    out.deleteFile();
    juce::FileOutputStream stream (out);
    if (! stream.openedOk() || ! juce::PNGImageFormat().writeImageToStream (image, stream))
        return 1;

    editor.reset();
    return 0;
}
