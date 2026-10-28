#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Palette
{
    inline const juce::Colour background { 0xff141518 };
    inline const juce::Colour panel      { 0xff1c1d21 };
    inline const juce::Colour raised     { 0xff24262b };
    inline const juce::Colour line       { 0xff2c2e34 };
    inline const juce::Colour text       { 0xffe6e6e6 };
    inline const juce::Colour dim        { 0xff7d8088 };
    inline const juce::Colour faint      { 0xff464950 };
    inline const juce::Colour accent     { 0xffff7a45 };  // drums
    inline const juce::Colour cymbal     { 0xffe8d5b0 };  // hats and cymbals
    inline const juce::Colour tom        { 0xff5fc7b8 };  // toms and percussion

    juce::Colour forPart (int part);
}

class GrooverLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrooverLookAndFeel();

    static juce::Font font (float height, bool bold = false);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (13.0f); }
    juce::Font getPopupMenuFont() override               { return font (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (11.0f, true); }
    juce::Font getLabelFont (juce::Label&) override      { return font (12.0f); }

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isMouseOverButton, bool isButtonDown) override;
};
