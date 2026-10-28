#include "KitPanel.h"
#include "GrooverLookAndFeel.h"
#include "../PluginProcessor.h"

using namespace groover;

namespace
{
    int parseNote (const juce::String& text)
    {
        const auto t = text.trim().toUpperCase();
        if (t.containsOnly ("0123456789"))
            return t.getIntValue();

        // Note names use Ableton's octave numbering, where C3 = 60.
        static const char* names[] = { "C#", "D#", "F#", "G#", "A#", "C", "D", "E", "F", "G", "A", "B" };
        static const int pitch[]   = {  1,    3,    6,    8,    10,   0,   2,   4,   5,   7,   9,   11 };

        for (int i = 0; i < 12; ++i)
            if (t.startsWith (names[i]))
            {
                const auto octave = t.substring ((int) strlen (names[i])).trim();
                if (octave.isEmpty() || ! octave.containsOnly ("-0123456789"))
                    return -1;
                return (octave.getIntValue() + 2) * 12 + pitch[i];
            }

        return -1;
    }
}

KitPanel::KitPanel (GrooverProcessor& p) : processor (p)
{
    for (int i = 0; i < kNumParts; ++i)
    {
        auto& row = rows[(size_t) i];

        row.note.setSliderStyle (juce::Slider::LinearBarVertical);
        row.note.setRange (0.0, 127.0, 1.0);
        row.note.setMouseDragSensitivity (300);
        row.note.setTextBoxIsEditable (true);
        row.note.setVelocityBasedMode (false);
        row.note.textFromValueFunction = [] (double v)
        {
            const int n = (int) v;
            return juce::String (noteName (n)) + "  " + juce::String (n);
        };
        row.note.valueFromTextFunction = [&row] (const juce::String& text)
        {
            const int n = parseNote (text);
            return n >= 0 && n <= 127 ? (double) n : row.note.getValue();
        };
        row.note.setValue (processor.getNote (i), juce::dontSendNotification);
        row.note.onValueChange = [this, i] { processor.setNote (i, (int) rows[(size_t) i].note.getValue()); };
        row.note.setTooltip ("Drag, scroll or double-click to type a note (e.g. 36 or C1)");
        addAndMakeVisible (row.note);

        row.learn.setTooltip ("Click, then play a note on your controller to assign it");
        row.learn.onClick = [this, i]
        {
            processor.learnPart = processor.learnPart.load() == i ? -1 : i;
            refresh();
        };
        addAndMakeVisible (row.learn);
    }

    gmButton.setTooltip ("Kick 36, Snare 38, Hats 42/44/46, Toms 50/47/43, Ride 51, Crash 49, Cowbell 56");
    gmButton.onClick = [this] { processor.applyKitPreset (KitPreset::GeneralMidi); refresh(); };
    chromaticButton.setTooltip ("Parts in order from C1 (36) upwards: handy for custom Drum Racks and samplers");
    chromaticButton.onClick = [this] { processor.applyKitPreset (KitPreset::ChromaticC1); refresh(); };
    addAndMakeVisible (gmButton);
    addAndMakeVisible (chromaticButton);

    for (int ch = 1; ch <= 16; ++ch)
        channelBox.addItem ("Channel " + juce::String (ch), ch);
    channelBox.setSelectedId (processor.midiChannel.load(), juce::dontSendNotification);
    channelBox.onChange = [this] { processor.midiChannel = channelBox.getSelectedId(); };
    channelBox.setTooltip ("MIDI channel for generated notes");
    addAndMakeVisible (channelBox);

    thruButton.setClickingTogglesState (true);
    thruButton.setToggleState (processor.midiThru.load(), juce::dontSendNotification);
    thruButton.onClick = [this] { processor.midiThru = thruButton.getToggleState(); };
    thruButton.setTooltip ("Pass incoming MIDI through to the drum instrument");
    addAndMakeVisible (thruButton);
}

void KitPanel::refresh()
{
    const int learning = processor.learnPart.load();

    for (int i = 0; i < kNumParts; ++i)
    {
        auto& row = rows[(size_t) i];
        row.learn.setToggleState (learning == i, juce::dontSendNotification);
        row.learn.setButtonText (learning == i ? "PLAY..." : "LEARN");

        if (! row.note.isMouseButtonDown() && (int) row.note.getValue() != processor.getNote (i))
            row.note.setValue (processor.getNote (i), juce::dontSendNotification);
    }

    if (channelBox.getSelectedId() != processor.midiChannel.load())
        channelBox.setSelectedId (processor.midiChannel.load(), juce::dontSendNotification);
    thruButton.setToggleState (processor.midiThru.load(), juce::dontSendNotification);
}

juce::Rectangle<int> KitPanel::rowArea (int part) const
{
    auto area = getLocalBounds().withTrimmedTop (48);
    const int columnWidth = area.getWidth() / 2;
    const int rowHeight = juce::jmin (40, area.getHeight() / 6);
    const int column = part / 6, row = part % 6;
    const juce::Rectangle<int> r (area.getX() + column * columnWidth, area.getY() + row * rowHeight, columnWidth, rowHeight);
    return column == 0 ? r.withTrimmedRight (12) : r.withTrimmedLeft (12);
}

void KitPanel::paint (juce::Graphics& g)
{
    g.setFont (GrooverLookAndFeel::font (10.0f, true));
    g.setColour (Palette::dim);
    g.drawText ("PRESET", 0, 0, 80, 14, juce::Justification::centredLeft, false);
    g.drawText ("OUTPUT", getWidth() - 280, 0, 80, 14, juce::Justification::centredLeft, false);

    for (int i = 0; i < kNumParts; ++i)
    {
        const auto row = rowArea (i);
        const auto r = row.reduced (0, 4);

        g.setColour (Palette::forPart (i));
        g.fillEllipse ((float) r.getX() + 2.0f, (float) r.getCentreY() - 2.5f, 5.0f, 5.0f);

        g.setColour (Palette::text);
        g.setFont (GrooverLookAndFeel::font (13.0f));
        g.drawText (kPartInfo[(size_t) i].name, r.withTrimmedLeft (16).withWidth (110), juce::Justification::centredLeft, false);

        g.setColour (Palette::line.withAlpha (0.6f));
        g.drawHorizontalLine (row.getBottom() - 1, (float) row.getX(), (float) row.getRight());
    }
}

void KitPanel::resized()
{
    auto top = getLocalBounds().removeFromTop (44).withTrimmedTop (16);
    gmButton.setBounds (top.removeFromLeft (110).withHeight (24));
    top.removeFromLeft (8);
    chromaticButton.setBounds (top.removeFromLeft (110).withHeight (24));

    auto right = top.removeFromRight (280);
    channelBox.setBounds (right.removeFromLeft (130).withHeight (24));
    right.removeFromLeft (8);
    thruButton.setBounds (right.removeFromLeft (100).withHeight (24));

    for (int i = 0; i < kNumParts; ++i)
    {
        auto r = rowArea (i).reduced (0, 7);
        auto& row = rows[(size_t) i];
        row.learn.setBounds (r.removeFromRight (70));
        r.removeFromRight (8);
        row.note.setBounds (r.removeFromRight (110));
    }
}
