#pragma once

#include "PluginProcessor.h"
#include "UI/Controls.h"
#include "UI/GrooverLookAndFeel.h"
#include "UI/KitPanel.h"
#include "UI/StepGrid.h"

class GrooverEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GrooverEditor (GrooverProcessor&);
    ~GrooverEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void syncGrooveSelection();
    void fillGrooveBox (const juce::String& category);
    void showKit (bool shouldShowKit);

    GrooverProcessor& processor;
    GrooverLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    juce::ComboBox categoryBox, grooveBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TextButton grooveTab { "GROOVE" }, kitTab { "KIT" };
    DragMidiButton dragButton;

    StepGrid grid;
    KitPanel kit;

    Knob swing, complexity, variation, humanize, velocity;
    Segmented fill, speed;
    DiceButton dice;

    juce::Rectangle<int> statusArea() const;

    int shownGroove = -1;
    uint32_t lastNotesSent = 0;
    int activityFrames = 0;   // frames left to keep the MIDI OUT light lit
    int shownStatus = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooverEditor)
};
