#include "ui/PresetBar.h"
#include "ui/OmegaLookAndFeel.h"

PresetBar::PresetBar (PresetManager& manager)
    : presets (manager)
{
    for (auto* c : std::initializer_list<juce::Component*> { &prev, &list, &next, &save })
        addAndMakeVisible (c);
    addChildComponent (nameEditor);

    list.setJustificationType (juce::Justification::centred);
    list.onChange = [this]
    {
        if (const int id = list.getSelectedId(); id > 0)
            presets.load (id - 1);
        refresh();
    };

    prev.onClick = [this] { presets.loadNext (-1); refresh(); };
    next.onClick = [this] { presets.loadNext (1); refresh(); };
    save.onClick = [this] { startSaving(); };

    nameEditor.setFont (omegaui::monoFont (15.0f));
    nameEditor.setJustification (juce::Justification::centred);
    nameEditor.onReturnKey = [this] { finishSaving (true); };
    nameEditor.onEscapeKey = [this] { finishSaving (false); };
    nameEditor.onFocusLost = [this] { finishSaving (false); };

    refresh();
}

void PresetBar::resized()
{
    auto r = getLocalBounds();
    save.setBounds (r.removeFromRight (64));
    r.removeFromRight (8);
    prev.setBounds (r.removeFromLeft (30));
    next.setBounds (r.removeFromRight (30));
    r.reduce (4, 0);
    list.setBounds (r);
    nameEditor.setBounds (r);
}

void PresetBar::refresh()
{
    const auto names = presets.getPresetNames();
    if (names != shownNames)
    {
        shownNames = names;
        list.clear (juce::dontSendNotification);
        list.addSectionHeading ("FACTORY");
        for (int i = 0; i < names.size(); ++i)
        {
            if (i == presets.getNumFactory())
                list.addSectionHeading ("USER");
            list.addItem (names[i], i + 1);
        }
    }

    const int index = presets.getCurrentIndex();
    if (index >= 0 && ! presets.isModified())
    {
        if (list.getSelectedId() != index + 1)
            list.setSelectedId (index + 1, juce::dontSendNotification);
    }
    else
    {
        const auto text = presets.getCurrentName() + " *";
        if (list.getText() != text)
            list.setText (text, juce::dontSendNotification);
    }
}

void PresetBar::startSaving()
{
    nameEditor.setText (presets.getCurrentName(), juce::dontSendNotification);
    nameEditor.setVisible (true);
    nameEditor.selectAll();
    nameEditor.grabKeyboardFocus();
}

void PresetBar::finishSaving (bool commit)
{
    if (! nameEditor.isVisible())
        return;

    const auto name = nameEditor.getText().trim();
    nameEditor.setVisible (false);

    if (commit && name.isNotEmpty())
        presets.saveUser (name);

    refresh();
}
