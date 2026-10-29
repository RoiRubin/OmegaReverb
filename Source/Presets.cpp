#include "Presets.h"
#include "Parameters.h"

namespace
{
    const juce::Identifier presetProperty { "preset" };
    constexpr const char* presetExtension = ".omegapreset";

    omega::HallParams make (double mix, double predelayMs, double decaySec, double size, double earlyDiff, double lateDiff,
                            double modRateHz, double modDepth, double bassMult, double bassXover, double highMult,
                            double highXover, double lowCut, double highCut)
    {
        return { (float) mix, (float) predelayMs, (float) decaySec, (float) size, (float) earlyDiff, (float) lateDiff,
                 (float) modRateHz, (float) modDepth, (float) bassMult, (float) bassXover, (float) highMult,
                 (float) highXover, (float) lowCut, (float) highCut };
    }
}

// Inspired by the character of ValhallaVintageVerb's reverb modes; values are our own approximations
// for OmegaReverb's hall engine, to be refined against measured Valhalla impulse responses.
const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets {
        //                         mix  pre  decay size  eDif lDif rate  depth bassM bassX highM highX  lowC  highC
        { "Init",            omega::HallParams {} },
        { "Concert Hall",    make (30,  20,  2.8,  70,   80,  75,  0.70, 45,   1.20, 300,  0.45, 4000,  80,  8500) },
        { "Bright Hall",     make (30,  15,  2.4,  65,   85,  70,  0.80, 35,   1.00, 250,  0.75, 6000, 100, 15000) },
        { "Plate",           make (30,   5,  1.8,  45,  100,  85,  1.00, 25,   0.80, 400,  0.70, 6000, 150, 12000) },
        { "Room",            make (30,   4,  0.8,  25,   90,  70,  0.90, 25,   1.00, 300,  0.50, 4500, 100,  9000) },
        { "Chamber",         make (30,   8,  1.4,  38,   85,  75,  0.60, 30,   1.10, 300,  0.55, 4000,  90,  9000) },
        { "Random Space",    make (30,  25,  3.2,  75,   70,  70,  0.30, 75,   1.20, 300,  0.50, 4500,  70,  9500) },
        { "Chorus Space",    make (30,  20,  3.0,  70,   75,  70,  1.60, 80,   1.10, 300,  0.55, 4500,  80, 10000) },
        { "Ambience",        make (25,   2,  0.5,  18,  100,  60,  0.80, 20,   0.90, 300,  0.70, 5000, 120, 11000) },
        { "Sanctuary",       make (30,  30,  5.5,  85,   80,  80,  0.40, 50,   1.30, 250,  0.40, 3500,  60,  8000) },
        { "Dirty Hall",      make (30,  15,  2.6,  65,   60,  65,  0.90, 65,   1.30, 350,  0.35, 3000,  70,  6000) },
        { "Dirty Plate",     make (30,   5,  1.7,  45,   90,  75,  1.10, 55,   0.90, 400,  0.40, 3500, 140,  6500) },
        { "Smooth Plate",    make (30,   4,  2.0,  50,  100,  90,  0.50, 15,   0.80, 400,  0.65, 6000, 150, 11000) },
        { "Smooth Room",     make (30,   3,  0.9,  28,  100,  85,  0.50, 15,   1.00, 300,  0.55, 5000, 100,  9500) },
        { "Smooth Random",   make (30,  15,  3.0,  70,   95,  90,  0.35, 45,   1.10, 300,  0.50, 4500,  80, 10000) },
        { "Chaotic Hall",    make (30,  20,  3.5,  75,   80,  75,  0.25, 90,   1.20, 300,  0.45, 4000,  70,  9000) },
        { "Chaotic Chamber", make (30,  10,  1.8,  42,   85,  75,  0.30, 85,   1.10, 300,  0.50, 4000,  90,  9000) },
        { "Chaotic Neutral", make (30,  15,  2.8,  65,   85,  80,  0.20, 70,   1.00, 300,  0.60, 5000,  80, 11000) },
        { "Cathedral",       make (30,  40,  7.0, 100,   75,  80,  0.40, 45,   1.30, 250,  0.40, 3500,  50,  8000) },
        { "Palace",          make (30,  50, 12.0, 100,   80,  85,  0.30, 50,   1.40, 250,  0.35, 3000,  40,  7500) },
        { "Chamber1979",     make (30,  10,  1.6,  40,   70,  70,  0.60, 40,   1.20, 300,  0.40, 3000,  90,  7000) },
        { "Hall1984",        make (30,  20,  2.6,  68,   75,  72,  0.80, 50,   1.20, 300,  0.40, 3500,  80,  8000) },
    };

    return presets;
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    refreshUserList();
}

juce::File PresetManager::userFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("OmegaReverb").getChildFile ("Presets");
}

void PresetManager::refreshUserList()
{
    userFiles = userFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + presetExtension);
    userFiles.sort();
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : factoryPresets())
        names.add (preset.name);
    for (const auto& file : userFiles)
        names.add (file.getFileNameWithoutExtension());
    return names;
}

int PresetManager::getNumFactory() const
{
    return (int) factoryPresets().size();
}

juce::String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty (presetProperty, factoryPresets().front().name).toString();
}

int PresetManager::getCurrentIndex() const
{
    return getPresetNames().indexOf (getCurrentName());
}

void PresetManager::setCurrentName (const juce::String& name)
{
    apvts.state.setProperty (presetProperty, name, nullptr);
}

omega::HallParams PresetManager::presetParams (int index) const
{
    if (index < getNumFactory())
        return factoryPresets()[(size_t) index].params;

    omega::HallParams p;
    if (auto xml = juce::XmlDocument::parse (userFiles[index - getNumFactory()]))
        for (const auto& spec : paramSpecs())
            p.*(spec.field) = (float) xml->getDoubleAttribute (spec.id, p.*(spec.field));
    return p;
}

omega::HallParams PresetManager::currentParams() const
{
    omega::HallParams p;
    for (const auto& spec : paramSpecs())
        if (auto* value = apvts.getRawParameterValue (spec.id))
            p.*(spec.field) = value->load();
    return p;
}

bool PresetManager::isModified() const
{
    const int index = getCurrentIndex();
    if (index < 0)
        return false;

    if (cachedName != getCurrentName())
    {
        cachedParams = presetParams (index);
        cachedName = getCurrentName();
    }

    const auto now = currentParams();
    for (const auto& spec : paramSpecs())
    {
        const float a = now.*(spec.field), b = cachedParams.*(spec.field);
        if (std::abs (a - b) > 1e-3f * std::max (1.0f, std::abs (b)))
            return true;
    }
    return false;
}

void PresetManager::load (int index)
{
    const auto names = getPresetNames();
    if (! juce::isPositiveAndBelow (index, names.size()))
        return;

    const auto p = presetParams (index);
    for (const auto& spec : paramSpecs())
    {
        if (auto* param = apvts.getParameter (spec.id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (p.*(spec.field)));
            param->endChangeGesture();
        }
    }

    setCurrentName (names[index]);
}

void PresetManager::loadNext (int delta)
{
    const int count = getPresetNames().size();
    const int index = getCurrentIndex();
    load (index < 0 ? 0 : (index + delta + count) % count);
}

bool PresetManager::saveUser (const juce::String& requestedName)
{
    auto name = juce::File::createLegalFileName (requestedName.trim());
    if (name.isEmpty())
        return false;

    for (const auto& preset : factoryPresets())
        if (name == preset.name)
            name << " (User)";

    const auto folder = userFolder();
    if (! folder.createDirectory())
        return false;

    juce::XmlElement xml ("OmegaPreset");
    const auto p = currentParams();
    for (const auto& spec : paramSpecs())
        xml.setAttribute (spec.id, (double) (p.*(spec.field)));

    if (! xml.writeTo (folder.getChildFile (name + presetExtension)))
        return false;

    refreshUserList();
    cachedName.clear();
    setCurrentName (name);
    return true;
}
