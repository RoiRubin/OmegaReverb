#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace omegaui
{
inline const juce::Colour amber    { 0xffffa93a };
inline const juce::Colour cream    { 0xffe9e2d3 };
inline const juce::Colour creamDim { 0xff8f897e };
inline const juce::Colour glass    { 0xff0a0b0c };
inline const juce::Colour panel    { 0xff1a1b1f };

juce::Font monoFont (float height);
juce::Font engravedFont (float height, bool bold = false);
} // namespace omegaui

// Hybrid of 1980s rack hardware (knurled knobs, faceplate, LED glass) and a modern dark UI (glowing value arcs)
class OmegaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OmegaLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;
};
