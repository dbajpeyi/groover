#include "PluginEditor.h"
#include "MidiExport.h"

using namespace groover;

namespace
{
    constexpr int editorWidth = 880;
    constexpr int editorHeight = 524;
    constexpr int margin = 20;
    constexpr int headerHeight = 60;
    constexpr int footerHeight = 112;

    const juce::String dot = juce::String::fromUTF8 ("  \xc2\xb7  ");
}

GrooverEditor::GrooverEditor (GrooverProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      grid (p),
      kit (p),
      swing (p.apvts, ParamIDs::swing, "Swing"),
      complexity (p.apvts, ParamIDs::complexity, "Complexity"),
      variation (p.apvts, ParamIDs::variation, "Variation"),
      humanize (p.apvts, ParamIDs::humanize, "Humanize"),
      velocity (p.apvts, ParamIDs::velocity, "Velocity"),
      fill (*p.apvts.getParameter (ParamIDs::fill), "Fill every", { "OFF", "2", "4", "8", "16" }),
      speed (*p.apvts.getParameter (ParamIDs::speed), "Speed", { "1/2", "1x", "2x" })
{
    setLookAndFeel (&lookAndFeel);

    const auto& categories = GrooveLibrary::categories();
    for (int i = 0; i < (int) categories.size(); ++i)
        categoryBox.addItem (categories[(size_t) i], i + 1);
    categoryBox.onChange = [this]
    {
        const auto category = categoryBox.getText();
        const auto& lib = GrooveLibrary::all();
        if (processor.getGroove().category == category.toStdString())
            return;
        for (int i = 0; i < (int) lib.size(); ++i)
            if (lib[(size_t) i].category == category.toStdString())
                return processor.selectGroove (i);
    };
    categoryBox.setTooltip ("Style family");
    addAndMakeVisible (categoryBox);

    grooveBox.onChange = [this]
    {
        if (const int id = grooveBox.getSelectedId(); id > 0 && id - 1 != processor.getGrooveIndex())
            processor.selectGroove (id - 1);
    };
    addAndMakeVisible (grooveBox);

    const int count = (int) GrooveLibrary::all().size();
    prevButton.onClick = [this, count] { processor.selectGroove ((processor.getGrooveIndex() + count - 1) % count); };
    nextButton.onClick = [this, count] { processor.selectGroove ((processor.getGrooveIndex() + 1) % count); };
    prevButton.setTooltip ("Previous groove");
    nextButton.setTooltip ("Next groove");
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    for (auto* tab : { &grooveTab, &kitTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (1);
        addAndMakeVisible (tab);
    }
    // Radio buttons also fire onClick when switched off, so act only on the one turned on.
    grooveTab.onClick = [this] { if (grooveTab.getToggleState()) showKit (false); };
    kitTab.onClick = [this] { if (kitTab.getToggleState()) showKit (true); };
    kitTab.setTooltip ("Map each part to the notes your drum instrument expects");

    dragButton.createFile = [this] { return MidiExport::writeCurrentGroove (processor); };
    dragButton.setTooltip ("Drag into a MIDI track to drop the groove as a clip");
    addAndMakeVisible (dragButton);

    addAndMakeVisible (grid);
    addChildComponent (kit);

    swing.setTooltip ("Delays every second 8th or 16th. 50% is straight, 66% is a triplet shuffle. Selecting a groove loads its usual swing.");
    complexity.setTooltip ("Below 50% strips the groove back to its skeleton; above 50% adds ghost notes, pickups and fills in the gaps.");
    variation.setTooltip ("How much each bar differs from the last.");
    humanize.setTooltip ("Small random shifts in timing and velocity.");
    velocity.setTooltip ("Overall velocity.");
    fill.setTooltip ("Play a fill in the last bar of every N bars, with a crash on the next downbeat.");
    speed.setTooltip ("Play the groove at half or double the host tempo.");
    dice.setTooltip ("New random seed: a fresh take on embellishments, variations, fills and humanize.");
    dice.onClick = [this] { processor.seed = (uint32_t) juce::Random::getSystemRandom().nextInt (1 << 30) + 1; };

    for (juce::Component* c : std::initializer_list<juce::Component*> { &swing, &complexity, &variation, &humanize, &velocity, &fill, &speed, &dice })
        addAndMakeVisible (c);

    grooveTab.setToggleState (true, juce::dontSendNotification);
    syncGrooveSelection();

    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

GrooverEditor::~GrooverEditor()
{
    processor.learnPart = -1;
    setLookAndFeel (nullptr);
}

void GrooverEditor::showKit (bool shouldShowKit)
{
    kit.setVisible (shouldShowKit);
    grid.setVisible (! shouldShowKit);
    if (! shouldShowKit)
        processor.learnPart = -1;
    kit.refresh();
}

void GrooverEditor::fillGrooveBox (const juce::String& category)
{
    grooveBox.clear (juce::dontSendNotification);
    const auto& lib = GrooveLibrary::all();
    for (int i = 0; i < (int) lib.size(); ++i)
        if (lib[(size_t) i].category == category.toStdString())
            grooveBox.addItem (lib[(size_t) i].name, i + 1);
}

void GrooverEditor::syncGrooveSelection()
{
    const int index = processor.getGrooveIndex();
    if (index == shownGroove)
        return;

    shownGroove = index;
    const auto& groove = processor.getGroove();
    const auto& categories = GrooveLibrary::categories();

    for (int i = 0; i < (int) categories.size(); ++i)
        if (categories[(size_t) i] == groove.category)
            categoryBox.setSelectedId (i + 1, juce::dontSendNotification);

    fillGrooveBox (groove.category);
    grooveBox.setSelectedId (index + 1, juce::dontSendNotification);
    repaint();
}

void GrooverEditor::timerCallback()
{
    syncGrooveSelection();

    if (grid.isVisible())
        grid.refresh();
    if (kit.isVisible())
        kit.refresh();
}

void GrooverEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);

    // Wordmark.
    auto header = getLocalBounds().removeFromTop (headerHeight).reduced (margin, 0);
    g.setColour (Palette::accent);
    g.fillEllipse ((float) header.getX(), (float) header.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour (Palette::text);
    g.setFont (GrooverLookAndFeel::font (17.0f, true));
    g.drawText ("GROOVER", header.withTrimmedLeft (16), juce::Justification::centredLeft, false);

    // Groove details.
    const auto& groove = processor.getGroove();
    auto info = getLocalBounds().withTrimmedTop (headerHeight).removeFromTop (22).reduced (margin, 0);

    juce::String details;
    details << groove.origin << dot << groove.timeSignature << dot << groove.tempo << " BPM";

    g.setFont (GrooverLookAndFeel::font (12.0f, true));
    g.setColour (Palette::text);
    const int detailsWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), details) + 4;
    g.drawText (details, info.removeFromLeft (detailsWidth), juce::Justification::centredLeft, false);

    g.setFont (GrooverLookAndFeel::font (12.0f));
    g.setColour (Palette::dim);
    g.drawText (dot.trimStart() + groove.notes, info, juce::Justification::centredLeft, true);

    // Footer divider.
    g.setColour (Palette::line);
    g.drawHorizontalLine (getHeight() - footerHeight, (float) margin, (float) (getWidth() - margin));
}

void GrooverEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop (headerHeight).reduced (margin, 16);
    header.removeFromLeft (120);
    categoryBox.setBounds (header.removeFromLeft (150));
    header.removeFromLeft (8);
    grooveBox.setBounds (header.removeFromLeft (230));
    header.removeFromLeft (6);
    prevButton.setBounds (header.removeFromLeft (28));
    header.removeFromLeft (4);
    nextButton.setBounds (header.removeFromLeft (28));

    dragButton.setBounds (header.removeFromRight (110));
    header.removeFromRight (16);
    kitTab.setBounds (header.removeFromRight (64));
    header.removeFromRight (4);
    grooveTab.setBounds (header.removeFromRight (76));

    bounds.removeFromTop (30);

    auto footer = bounds.removeFromBottom (footerHeight).reduced (margin, 14);
    for (auto* knob : { &swing, &complexity, &variation, &humanize, &velocity })
        knob->setBounds (footer.removeFromLeft (84));

    footer.removeFromLeft (20);
    auto controls = footer.withSizeKeepingCentre (footer.getWidth(), 46);
    fill.setBounds (controls.removeFromLeft (180));
    controls.removeFromLeft (16);
    speed.setBounds (controls.removeFromLeft (120));
    dice.setBounds (controls.removeFromRight (40).removeFromBottom (28));

    auto main = bounds.reduced (margin, 4);
    grid.setBounds (main);
    kit.setBounds (main);
}

juce::AudioProcessorEditor* GrooverProcessor::createEditor()
{
    return new GrooverEditor (*this);
}
