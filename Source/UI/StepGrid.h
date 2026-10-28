#pragma once

#include "../Core/GrooveEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

class GrooverProcessor;

// Shows the bar that is playing right now (or the first bar when stopped),
// exactly as the engine renders it, with a playhead. Click a part name to mute it.
class StepGrid : public juce::Component
{
public:
    explicit StepGrid (GrooverProcessor&);

    // Polled by the editor's timer; repaints only when something changed.
    void refresh();

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> gridArea() const;
    float rowHeight() const;
    int rowAt (juce::Point<int>) const;

    GrooverProcessor& processor;
    groover::BarEvents events;
    int64_t bar = 0;
    int playStep = -1;
    int hoverRow = -1;
    size_t signature = 0;

    static constexpr int labelWidth = 104;
    static constexpr int headerHeight = 20;
};
