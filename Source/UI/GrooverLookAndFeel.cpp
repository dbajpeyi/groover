#include "GrooverLookAndFeel.h"
#include "../Core/DrumKit.h"

juce::Colour Palette::forPart (int part)
{
    switch (part)
    {
        case groover::ClosedHat: case groover::PedalHat: case groover::OpenHat:
        case groover::Ride: case groover::Crash:
            return cymbal;
        case groover::HighTom: case groover::MidTom: case groover::FloorTom: case groover::Perc:
            return tom;
        default:
            return accent;
    }
}

GrooverLookAndFeel::GrooverLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour (juce::ComboBox::backgroundColourId, Palette::panel);
    setColour (juce::ComboBox::textColourId, Palette::text);
    setColour (juce::ComboBox::outlineColourId, Palette::line);
    setColour (juce::ComboBox::arrowColourId, Palette::dim);
    setColour (juce::PopupMenu::backgroundColourId, Palette::raised);
    setColour (juce::PopupMenu::textColourId, Palette::text);
    setColour (juce::PopupMenu::headerTextColourId, Palette::dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::accent.withAlpha (0.2f));
    setColour (juce::PopupMenu::highlightedTextColourId, Palette::text);
    setColour (juce::Label::textColourId, Palette::text);
    setColour (juce::TextButton::buttonColourId, Palette::panel);
    setColour (juce::TextButton::buttonOnColourId, Palette::accent);
    setColour (juce::TextButton::textColourOffId, Palette::dim);
    setColour (juce::TextButton::textColourOnId, Palette::text);
    setColour (juce::Slider::textBoxTextColourId, Palette::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, Palette::raised);
    setColour (juce::TextEditor::textColourId, Palette::text);
    setColour (juce::TextEditor::highlightColourId, Palette::accent.withAlpha (0.3f));
    setColour (juce::TextEditor::focusedOutlineColourId, Palette::accent);
    setColour (juce::CaretComponent::caretColourId, Palette::accent);
    setColour (juce::TooltipWindow::backgroundColourId, Palette::raised);
    setColour (juce::TooltipWindow::textColourId, Palette::text);
    setColour (juce::TooltipWindow::outlineColourId, Palette::line);
}

juce::Font GrooverLookAndFeel::font (float height, bool bold)
{
    auto options = juce::FontOptions (height).withKerningFactor (bold ? 0.06f : 0.0f);
    return juce::Font (bold ? options.withStyle ("Bold") : options);
}

void GrooverLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float stroke = 2.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius - stroke, radius - stroke, 0.0f, startAngle, endAngle, true);
    g.setColour (Palette::line);
    g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius - stroke, radius - stroke, 0.0f, startAngle, angle, true);
    g.setColour (slider.isEnabled() ? Palette::accent : Palette::faint);
    g.strokePath (value, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (Palette::raised);
    g.fillEllipse (bounds.withSizeKeepingCentre (radius * 1.25f, radius * 1.25f));

    const auto tip = centre.getPointOnCircumference (radius * 0.48f, angle);
    const auto inner = centre.getPointOnCircumference (radius * 0.18f, angle);
    g.setColour (Palette::text);
    g.drawLine ({ inner, tip }, 2.0f);
}

void GrooverLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float,
                                           float, float, juce::Slider::SliderStyle, juce::Slider& slider)
{
    // Used for the note boxes on the kit page: a flat field, value drawn by the text box.
    const auto r = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (0.5f);
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (slider.isMouseOverOrDragging() ? Palette::faint : Palette::line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

juce::Label* GrooverLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (font (12.0f));
    label->setColour (juce::Label::textColourId, Palette::text);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return label;
}

void GrooverLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    // Like the default, but never outlined: the surrounding control draws its own frame.
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        const auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (getLabelFont (label));
        g.drawFittedText (label.getText(), getLabelBorderSize (label).subtractedFrom (label.getLocalBounds()),
                          label.getJustificationType(), 1, label.getMinimumHorizontalScale());
    }
}

void GrooverLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.isMouseOver (true) ? Palette::faint : Palette::line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.0f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (Palette::dim);
    g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void GrooverLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 0, box.getWidth() - 30, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

void GrooverLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (Palette::raised);
    g.setColour (Palette::line);
    g.drawRect (0, 0, width, height);
}

void GrooverLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                               bool isMouseOverButton, bool isButtonDown)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();

    g.setColour (on ? Palette::accent.withAlpha (0.16f) : (isButtonDown ? Palette::raised : Palette::panel));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? Palette::accent.withAlpha (0.7f) : (isMouseOverButton ? Palette::faint : Palette::line));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void GrooverLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isMouseOverButton, bool)
{
    const bool on = button.getToggleState();
    g.setColour (on ? Palette::accent : (isMouseOverButton ? Palette::text : Palette::dim));
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 0), juce::Justification::centred, 1);
}
