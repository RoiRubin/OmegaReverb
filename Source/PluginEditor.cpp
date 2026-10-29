#include "PluginEditor.h"

using namespace omegaui;

namespace
{
    constexpr int kWidth = 880, kHeight = 560;
    constexpr int kRowTop = 268, kRowHeight = 136, kRowGap = 6, kSectionGap = 14;

    struct KnobDef { const char* id; const char* label; };
    struct SectionDef { const char* title; int row; std::vector<KnobDef> knobs; };

    const std::vector<SectionDef> kLayout {
        { "MAIN",       0, { { "mix", "MIX" }, { "predelay", "PRE-DELAY" }, { "decay", "DECAY" }, { "size", "SIZE" } } },
        { "DIFFUSION",  0, { { "earlyDiff", "EARLY" }, { "lateDiff", "LATE" } } },
        { "MODULATION", 0, { { "modRate", "RATE" }, { "modDepth", "DEPTH" } } },
        { "DAMPING",    1, { { "bassMult", "BASS MULT" }, { "bassXover", "BASS X" }, { "highMult", "HIGH MULT" }, { "highXover", "HIGH X" } } },
        { "OUTPUT EQ",  1, { { "lowCut", "LOW CUT" }, { "highCut", "HIGH CUT" } } },
    };

    // Brushed dark faceplate with rack screws, rendered once
    juce::Image makeFaceplate (int w, int h)
    {
        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Graphics g (img);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222429), 0.0f, 0.0f, juce::Colour (0xff121316), 0.0f, (float) h, false));
        g.fillAll();

        juce::Random rng (1984);
        for (int y = 0; y < h; ++y)
        {
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (rng.nextFloat() * 0.035f));
            g.drawHorizontalLine (y, 0.0f, (float) w);
        }

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRect (0, 0, w, h, 1);

        for (const auto& pos : { juce::Point<float> (14.0f, 14.0f), juce::Point<float> ((float) w - 14.0f, 14.0f),
                                 juce::Point<float> (14.0f, (float) h - 14.0f), juce::Point<float> ((float) w - 14.0f, (float) h - 14.0f) })
        {
            const auto screw = juce::Rectangle<float> (12.0f, 12.0f).withCentre (pos);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6d74), screw.getX(), screw.getY(),
                                                     juce::Colour (0xff25262a), screw.getRight(), screw.getBottom(), false));
            g.fillEllipse (screw);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawEllipse (screw, 1.0f);
            const float a = rng.nextFloat() * juce::MathConstants<float>::pi;
            g.drawLine ({ pos.getPointOnCircumference (4.5f, a), pos.getPointOnCircumference (4.5f, a + juce::MathConstants<float>::pi) }, 1.6f);
        }

        return img;
    }
}

OmegaReverbEditor::OmegaReverbEditor (OmegaReverbProcessor& p)
    : AudioProcessorEditor (p),
      presetBar (p.presets),
      display ([&p] { return p.readParams(); })
{
    addAndMakeVisible (presetBar);
    addAndMakeVisible (display);

    for (const auto& def : kLayout)
    {
        Section section { def.title, def.row, {}, {} };
        for (const auto& k : def.knobs)
        {
            knobs.push_back (std::make_unique<Knob> (p.apvts, k.id, k.label));
            addAndMakeVisible (*knobs.back());
            section.knobs.push_back (knobs.back().get());
        }
        sections.push_back (std::move (section));
    }

    // Set after the children exist, so the change propagates to them (slider text boxes copy colours on that event)
    setLookAndFeel (&lookAndFeel);

    faceplate = makeFaceplate (kWidth, kHeight);
    setSize (kWidth, kHeight);
    startTimerHz (10);
}

OmegaReverbEditor::~OmegaReverbEditor()
{
    setLookAndFeel (nullptr);
}

void OmegaReverbEditor::timerCallback()
{
    presetBar.refresh();
}

void OmegaReverbEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (faceplate, 0, 0);

    // Brand
    g.setColour (cream);
    g.setFont (engravedFont (28.0f, true).withExtraKerningFactor (0.35f));
    g.drawText ("OMEGA", 30, 14, 220, 32, juce::Justification::centredLeft);
    g.setColour (creamDim);
    g.setFont (engravedFont (10.0f, true));
    g.drawText ("DIGITAL REVERBERATOR", 32, 46, 200, 14, juce::Justification::centredLeft);
    g.setColour (amber.withAlpha (0.7f));
    g.setFont (monoFont (11.0f));
    g.drawText ("v" OMEGA_VERSION, 200, 46, 80, 14, juce::Justification::centredLeft);

    // Recessed section panels with engraved titles
    for (const auto& section : sections)
    {
        const auto area = section.area.toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (area, 6.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine ((int) area.getBottom(), area.getX() + 6.0f, area.getRight() - 6.0f);

        const auto font = engravedFont (10.5f, true);
        const float textW = juce::GlyphArrangement::getStringWidth (font, section.title) + 16.0f;
        const float titleY = area.getY() + 11.0f;
        g.setColour (creamDim.withAlpha (0.35f));
        g.drawHorizontalLine ((int) titleY, area.getX() + 10.0f, area.getCentreX() - textW * 0.5f);
        g.drawHorizontalLine ((int) titleY, area.getCentreX() + textW * 0.5f, area.getRight() - 10.0f);
        g.setColour (cream.withAlpha (0.8f));
        g.setFont (font);
        g.drawText (section.title, juce::Rectangle<float> (area.getCentreX() - textW * 0.5f, titleY - 7.0f, textW, 14.0f), juce::Justification::centred);
    }
}

void OmegaReverbEditor::resized()
{
    presetBar.setBounds (kWidth - 30 - 470, 20, 470, 32);
    display.setBounds (20, 78, kWidth - 40, 176);

    const int knobW = (kWidth - 40 - 2 * kSectionGap) / 8;

    for (int row = 0; row < 2; ++row)
    {
        int total = -kSectionGap;
        for (const auto& s : sections)
            if (s.row == row)
                total += (int) s.knobs.size() * knobW + kSectionGap;

        int x = (kWidth - total) / 2;
        const int y = kRowTop + row * (kRowHeight + kRowGap);

        for (auto& s : sections)
        {
            if (s.row != row)
                continue;

            s.area = { x, y, (int) s.knobs.size() * knobW, kRowHeight };
            for (size_t i = 0; i < s.knobs.size(); ++i)
                s.knobs[i]->setBounds (x + (int) i * knobW, y + 22, knobW, kRowHeight - 28);
            x += s.area.getWidth() + kSectionGap;
        }
    }
}
