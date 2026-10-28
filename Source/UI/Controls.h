#pragma once

#include "GrooverLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// A rotary knob bound to a parameter, with its name and value underneath.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& name)
        : title (name)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setMouseDragSensitivity (180);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);

        if (auto* param = state.getParameter (paramID))
            slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }

    void setTooltip (const juce::String& text) { slider.setTooltip (text); }

    void resized() override
    {
        auto r = getLocalBounds();
        slider.setBounds (r.removeFromTop (r.getHeight() - 32).withSizeKeepingCentre (54, 54));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().removeFromBottom (32);
        g.setColour (Palette::dim);
        g.setFont (GrooverLookAndFeel::font (10.0f, true));
        g.drawText (title.toUpperCase(), r.removeFromTop (14), juce::Justification::centred, false);
        g.setColour (Palette::text);
        g.setFont (GrooverLookAndFeel::font (12.0f));
        g.drawText (slider.getTextFromValue (slider.getValue()), r, juce::Justification::centred, false);
    }

private:
    juce::String title;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// A row of mutually exclusive options bound to a choice parameter.
class Segmented : public juce::Component, public juce::SettableTooltipClient
{
public:
    Segmented (juce::RangedAudioParameter& param, const juce::String& name, juce::StringArray optionLabels)
        : title (name), labels (std::move (optionLabels)),
          attachment (param, [this] (float v) { selected = juce::roundToInt (v); repaint(); })
    {
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour (Palette::dim);
        g.setFont (GrooverLookAndFeel::font (10.0f, true));
        g.drawText (title.toUpperCase(), r.removeFromTop (16), juce::Justification::centredLeft, false);

        const auto box = r.removeFromTop (28).toFloat().reduced (0.5f);
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (Palette::line);
        g.drawRoundedRectangle (box, 5.0f, 1.0f);

        const float w = box.getWidth() / (float) labels.size();
        g.setFont (GrooverLookAndFeel::font (11.0f, true));

        for (int i = 0; i < labels.size(); ++i)
        {
            const auto cell = juce::Rectangle<float> (box.getX() + w * (float) i, box.getY(), w, box.getHeight());
            if (i == selected)
            {
                g.setColour (Palette::accent.withAlpha (0.16f));
                g.fillRoundedRectangle (cell.reduced (2.0f), 4.0f);
                g.setColour (Palette::accent);
            }
            else
            {
                g.setColour (Palette::dim);
            }
            g.drawText (labels[i], cell, juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < 16)
            return;
        const int index = juce::jlimit (0, labels.size() - 1, e.x * labels.size() / juce::jmax (1, getWidth()));
        attachment.setValueAsCompleteGesture ((float) index);
    }

private:
    juce::String title;
    juce::StringArray labels;
    int selected = 0;
    juce::ParameterAttachment attachment;
};

// Rolls a new random seed: different embellishments, variations, fills and humanize.
class DiceButton : public juce::Button
{
public:
    DiceButton() : juce::Button ("New Seed") {}

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? Palette::raised : Palette::panel);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (over ? Palette::faint : Palette::line);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        const auto die = r.withSizeKeepingCentre (16.0f, 16.0f);
        g.setColour (over ? Palette::text : Palette::dim);
        g.drawRoundedRectangle (die, 3.0f, 1.4f);
        const float pip = 2.6f;
        for (auto p : { die.getCentre(), die.getTopLeft() + juce::Point<float> (4.5f, 4.5f), die.getBottomRight() - juce::Point<float> (4.5f, 4.5f) })
            g.fillEllipse (p.x - pip * 0.5f, p.y - pip * 0.5f, pip, pip);
    }
};

// Drag this into the host to drop the current groove as a MIDI clip.
class DragMidiButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<juce::File()> createFile;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool hot = isMouseOverOrDragging();
        g.setColour (hot ? Palette::accent.withAlpha (0.16f) : Palette::panel);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (hot ? Palette::accent.withAlpha (0.7f) : Palette::line);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        auto content = getLocalBounds().reduced (10, 0);
        auto icon = content.removeFromLeft (12).toFloat().withSizeKeepingCentre (10.0f, 12.0f);
        juce::Path arrow;
        arrow.startNewSubPath (icon.getCentreX(), icon.getY());
        arrow.lineTo (icon.getCentreX(), icon.getBottom() - 1.0f);
        arrow.startNewSubPath (icon.getX(), icon.getBottom() - 5.0f);
        arrow.lineTo (icon.getCentreX(), icon.getBottom() - 1.0f);
        arrow.lineTo (icon.getRight(), icon.getBottom() - 5.0f);
        g.setColour (hot ? Palette::accent : Palette::dim);
        g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont (GrooverLookAndFeel::font (11.0f, true));
        g.drawText ("DRAG MIDI", content.withTrimmedLeft (6), juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { setMouseCursor (juce::MouseCursor::DraggingHandCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
    void mouseUp (const juce::MouseEvent&) override    { dragging = false; repaint(); }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 4 || createFile == nullptr)
            return;

        dragging = true;
        const auto file = createFile();
        if (file.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
    }

private:
    bool dragging = false;
};
