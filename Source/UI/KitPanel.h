#pragma once

#include "../Core/DrumKit.h"

#include <juce_gui_basics/juce_gui_basics.h>

class GrooverProcessor;

// Assigns each of the twelve parts to an outgoing MIDI note, so grooves can
// drive any kit layout. Notes can be dragged, typed (36 or C1) or learned.
class KitPanel : public juce::Component
{
public:
    explicit KitPanel (GrooverProcessor&);

    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row
    {
        juce::Slider note;
        juce::TextButton learn { "LEARN" };
    };

    GrooverProcessor& processor;
    std::array<Row, groover::kNumParts> rows;

    juce::TextButton gmButton { "GENERAL MIDI" }, chromaticButton { "CHROMATIC C1" };
    juce::ComboBox channelBox;
    juce::TextButton thruButton { "MIDI THRU" };

    juce::Rectangle<int> rowArea (int part) const;
};
