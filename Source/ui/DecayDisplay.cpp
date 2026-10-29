#include "ui/DecayDisplay.h"
#include "ui/OmegaLookAndFeel.h"

#include <cstring>

using namespace omegaui;

namespace
{
    constexpr float kMinHz = 20.0f, kMaxHz = 20000.0f;
    constexpr float kToneMinDb = -30.0f, kToneMaxDb = 3.0f;

    float niceCeiling (float v)
    {
        for (float step : { 0.5f, 1.0f, 2.0f, 3.0f, 5.0f, 8.0f, 10.0f, 15.0f, 20.0f, 30.0f, 40.0f, 60.0f, 80.0f })
            if (v <= step)
                return step;
        return std::ceil (v / 20.0f) * 20.0f;
    }

    juce::String hzLabel (float hz)
    {
        return hz >= 1000.0f ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz);
    }
}

DecayDisplay::DecayDisplay (std::function<omega::HallParams()> paramSource)
    : getParams (std::move (paramSource)), shown (getParams())
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void DecayDisplay::timerCallback()
{
    const auto p = getParams();
    if (std::memcmp (&p, &shown, sizeof (p)) != 0)
    {
        shown = p;
        repaint();
    }
}

void DecayDisplay::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();

    // Glass
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101214), 0.0f, r.getY(), juce::Colour (0xff060708), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine ((int) r.getBottom() - 1, r.getX() + 6.0f, r.getRight() - 6.0f);

    auto inner = r.reduced (16.0f, 10.0f);
    const auto readout = inner.removeFromTop (20.0f);
    const auto freqLabels = inner.removeFromBottom (14.0f);
    const auto plot = inner.withTrimmedLeft (34.0f).withTrimmedTop (4.0f);

    auto xOf = [&] (float hz) { return plot.getX() + plot.getWidth() * std::log (hz / kMinHz) / std::log (kMaxHz / kMinHz); };

    // Curve data
    constexpr int points = 220;
    std::array<float, points> rt {}, tone {};
    float maxRt = 0.1f;
    for (int i = 0; i < points; ++i)
    {
        const float hz = kMinHz * std::pow (kMaxHz / kMinHz, (float) i / (points - 1));
        rt[(size_t) i] = omega::HallReverb::rt60At (shown, hz);
        tone[(size_t) i] = omega::HallReverb::wetToneDbAt (shown, hz);
        maxRt = std::max (maxRt, rt[(size_t) i]);
    }
    const float yMax = niceCeiling (maxRt * 1.15f);
    auto yOfRt = [&] (float v) { return plot.getBottom() - plot.getHeight() * std::min (v / yMax, 1.0f); };
    auto yOfTone = [&] (float db) { return plot.getBottom() - plot.getHeight() * (std::clamp (db, kToneMinDb, kToneMaxDb) - kToneMinDb) / (kToneMaxDb - kToneMinDb); };
    auto xAt = [&] (int i) { return plot.getX() + plot.getWidth() * (float) i / (points - 1); };

    // Grid
    g.setFont (monoFont (10.0f));
    for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xOf (hz);
        g.setColour (amber.withAlpha (0.07f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (creamDim.withAlpha (0.6f));
        g.drawText (hzLabel (hz), juce::Rectangle<float> (x - 20.0f, freqLabels.getY(), 40.0f, freqLabels.getHeight()), juce::Justification::centred);
    }
    for (int i = 1; i <= 4; ++i)
    {
        const float v = yMax * (float) i / 4.0f;
        g.setColour (amber.withAlpha (0.07f));
        g.drawHorizontalLine ((int) yOfRt (v), plot.getX(), plot.getRight());
        g.setColour (creamDim.withAlpha (0.6f));
        g.drawText (juce::String (v, v < 10.0f ? 1 : 0) + "s", juce::Rectangle<float> (r.getX() + 8.0f, yOfRt (v) - 7.0f, 38.0f, 14.0f), juce::Justification::centredRight);
    }

    // Tone curve (dim)
    juce::Path toneLine;
    for (int i = 0; i < points; ++i)
        i == 0 ? toneLine.startNewSubPath (xAt (i), yOfTone (tone[0])) : toneLine.lineTo (xAt (i), yOfTone (tone[(size_t) i]));
    g.setColour (cream.withAlpha (0.28f));
    g.strokePath (toneLine, juce::PathStrokeType (1.2f));

    // RT60 curve with glow and fill
    juce::Path curve;
    for (int i = 0; i < points; ++i)
        i == 0 ? curve.startNewSubPath (xAt (i), yOfRt (rt[0])) : curve.lineTo (xAt (i), yOfRt (rt[(size_t) i]));

    juce::Path fill (curve);
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.28f), 0.0f, plot.getY(), amber.withAlpha (0.0f), 0.0f, plot.getBottom(), false));
    g.fillPath (fill);

    g.setColour (amber.withAlpha (0.18f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (amber);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Readouts
    g.setFont (monoFont (13.0f));
    g.setColour (amber);
    g.drawText ("RT60 " + juce::String (shown.decaySec, 2) + "s   PRE " + juce::String (juce::roundToInt (shown.predelayMs)) + "ms   SIZE "
                    + juce::String (juce::roundToInt (shown.size)) + "%   MIX " + juce::String (juce::roundToInt (shown.mix)) + "%",
                readout, juce::Justification::centredLeft);
    g.setFont (engravedFont (10.0f, true));
    g.setColour (creamDim);
    g.drawText ("DECAY vs FREQUENCY  /  TONE", readout, juce::Justification::centredRight);

    // LED scanlines
    g.setColour (juce::Colours::black.withAlpha (0.14f));
    for (float y = r.getY() + 2.0f; y < r.getBottom() - 2.0f; y += 3.0f)
        g.drawHorizontalLine ((int) y, r.getX() + 2.0f, r.getRight() - 2.0f);
}
