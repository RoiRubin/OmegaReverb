#include "ui/OmegaLookAndFeel.h"

namespace omegaui
{
juce::Font monoFont (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

juce::Font engravedFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor (0.18f);
}
} // namespace omegaui

using namespace omegaui;

OmegaLookAndFeel::OmegaLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, amber);
    setColour (juce::Slider::textBoxBackgroundColourId, glass);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff26272b));
    setColour (juce::Slider::textBoxHighlightColourId, amber.withAlpha (0.3f));

    setColour (juce::TextEditor::backgroundColourId, glass);
    setColour (juce::TextEditor::textColourId, amber);
    setColour (juce::TextEditor::highlightColourId, amber.withAlpha (0.3f));
    setColour (juce::TextEditor::highlightedTextColourId, amber);
    setColour (juce::TextEditor::outlineColourId, amber.withAlpha (0.4f));
    setColour (juce::TextEditor::focusedOutlineColourId, amber);
    setColour (juce::CaretComponent::caretColourId, amber);

    setColour (juce::ComboBox::textColourId, amber);
    setColour (juce::ComboBox::backgroundColourId, glass);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2a2b2f));
    setColour (juce::ComboBox::arrowColourId, amber);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff111214));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::headerTextColourId, amber.withAlpha (0.7f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, amber.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, amber);

    setColour (juce::TextButton::textColourOffId, cream);
    setColour (juce::TextButton::textColourOnId, amber);
}

void OmegaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f - 6.0f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const juce::PathStrokeType thin (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    // Track and glowing value arc
    juce::Path track;
    track.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, thin);

    juce::Path value;
    value.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (amber.withAlpha (0.16f));
    g.strokePath (value, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (amber);
    g.strokePath (value, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Hardware knob: drop shadow, knurled skirt, metal cap, cream pointer
    const float knobR = radius * 0.74f;
    const auto knob = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (c);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (knob.translated (0.0f, 2.5f).expanded (1.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3d43), c.x, knob.getY(),
                                             juce::Colour (0xff131417), c.x, knob.getBottom(), false));
    g.fillEllipse (knob);

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::twoPi / 40.0f;
        g.drawLine ({ c.getPointOnCircumference (knobR * 0.86f, a), c.getPointOnCircumference (knobR, a) }, 1.0f);
    }

    const float capR = knobR * 0.72f;
    const auto cap = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff53565e), c.x - capR * 0.4f, c.y - capR * 0.6f,
                                             juce::Colour (0xff1b1c20), c.x + capR * 0.6f, c.y + capR * 0.9f, true));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawEllipse (cap, 1.0f);

    g.setColour (cream);
    g.drawLine ({ c.getPointOnCircumference (capR * 0.2f, angle), c.getPointOnCircumference (knobR * 0.94f, angle) }, 2.5f);
}

void OmegaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                             bool highlighted, bool down)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto top = juce::Colour (down ? 0xff1c1d21 : (highlighted ? 0xff3a3c42 : 0xff2e3035));
    const auto bottom = juce::Colour (down ? 0xff26272b : 0xff18191c);

    g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bottom, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.0f : 0.07f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f);
}

juce::Font OmegaLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return engravedFont (11.0f, true);
}

void OmegaLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);

    g.setColour (glass);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine ((int) r.getBottom() - 1, r.getX() + 3.0f, r.getRight() - 3.0f);

    // LED scanlines
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    for (int ly = 2; ly < height - 2; ly += 3)
        g.drawHorizontalLine (ly, 2.0f, (float) width - 2.0f);

    juce::Path arrow;
    const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (amber.withAlpha (0.8f));
    g.fillPath (arrow);
}

juce::Font OmegaLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return monoFont (15.0f);
}

void OmegaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font OmegaLookAndFeel::getLabelFont (juce::Label&)
{
    return monoFont (12.0f);
}

juce::Font OmegaLookAndFeel::getPopupMenuFont()
{
    return monoFont (14.0f);
}
