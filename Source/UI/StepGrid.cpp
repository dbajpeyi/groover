#include "StepGrid.h"
#include "GrooverLookAndFeel.h"
#include "../PluginProcessor.h"

using namespace groover;

StepGrid::StepGrid (GrooverProcessor& p) : processor (p)
{
    setRepaintsOnMouseActivity (false);
}

juce::Rectangle<float> StepGrid::gridArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft ((float) labelWidth).withTrimmedTop ((float) headerHeight);
}

float StepGrid::rowHeight() const
{
    return gridArea().getHeight() / (float) kNumParts;
}

int StepGrid::rowAt (juce::Point<int> pos) const
{
    if (pos.x >= labelWidth || pos.y < headerHeight)
        return -1;
    const int row = (int) ((float) (pos.y - headerHeight) / rowHeight());
    return row >= 0 && row < kNumParts ? row : -1;
}

void StepGrid::refresh()
{
    const auto& groove = processor.getGroove();
    const auto params = processor.getGenParams();
    const bool playing = processor.transportPlaying.load();
    const double pos = playing ? processor.displayPosition.load() : 0.0;
    const double barLength = groove.barLengthQuarters();

    bar = (int64_t) std::floor (pos / barLength);
    playStep = playing ? juce::jlimit (0, groove.stepsPerBar - 1,
                                       (int) std::floor ((pos - (double) bar * barLength) * groove.stepsPerQuarter))
                       : -1;

    generateBar (groove, bar, params, events);

    // Cheap fingerprint of everything drawn, so an idle editor costs nothing.
    size_t h = std::hash<int>() (processor.getGrooveIndex());
    auto combine = [&h] (size_t v) { h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
    combine ((size_t) bar);
    combine ((size_t) (playStep + 1));
    combine ((size_t) hoverRow + 7);
    for (int p = 0; p < kNumParts; ++p)
        combine ((size_t) processor.getNote (p) * 2 + (processor.isPartMuted (p) ? 1 : 0));
    for (const auto& e : events)
        combine ((size_t) (e.step * 131 + e.part * 7 + e.velocity));

    if (h != signature)
    {
        signature = h;
        repaint();
    }
}

void StepGrid::paint (juce::Graphics& g)
{
    const auto& groove = processor.getGroove();
    const int steps = groove.stepsPerBar;
    const auto area = gridArea();
    const float rowH = rowHeight();
    const float colW = area.getWidth() / (float) steps;

    // Beat shading and numbers, following the groove's grouping (2+2+3 etc.).
    g.setFont (GrooverLookAndFeel::font (10.0f, true));
    const auto& beats = groove.beatStarts;
    for (size_t beat = 0; beat < beats.size(); ++beat)
    {
        const int s = beats[beat];
        const int next = beat + 1 < beats.size() ? beats[beat + 1] : steps;
        const auto column = juce::Rectangle<float> (area.getX() + (float) s * colW, area.getY(),
                                                    colW * (float) (next - s), area.getHeight());
        if (beat % 2 == 0)
        {
            g.setColour (Palette::panel);
            g.fillRect (column);
        }

        g.setColour (Palette::dim);
        g.drawText (juce::String (beat + 1), column.withY (0.0f).withHeight ((float) headerHeight - 4.0f).toNearestInt(),
                    juce::Justification::bottomLeft, false);
    }

    // Bar counter.
    const bool fill = isFillBar (bar, processor.getGenParams().fillEvery);
    juce::String barText = "BAR " + juce::String (bar + 1);
    if (groove.bars > 1)
        barText << "  (" << (int) ((bar % groove.bars + groove.bars) % groove.bars) + 1 << "/" << groove.bars << ")";
    g.setColour (fill ? Palette::accent : Palette::dim);
    g.drawText (fill ? barText + "  FILL" : barText, getLocalBounds().withHeight (headerHeight - 4),
                juce::Justification::bottomRight, false);

    // Playhead.
    if (playStep >= 0)
    {
        g.setColour (Palette::text.withAlpha (0.07f));
        g.fillRect (area.getX() + (float) playStep * colW, area.getY(), colW, area.getHeight());
    }

    // Rows.
    for (int p = 0; p < kNumParts; ++p)
    {
        const float y = area.getY() + (float) p * rowH;
        const bool muted = processor.isPartMuted (p);
        const bool used = groove.usesPart[(size_t) p];
        const auto label = juce::Rectangle<float> (0.0f, y, (float) labelWidth - 10.0f, rowH);

        g.setColour (Palette::line.withAlpha (0.6f));
        g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());

        if (p == hoverRow)
        {
            g.setColour (Palette::panel);
            g.fillRoundedRectangle (label.reduced (0.0f, 1.0f), 4.0f);
        }

        const auto colour = Palette::forPart (p);
        g.setColour (muted ? Palette::faint : colour.withAlpha (used ? 1.0f : 0.35f));
        g.fillEllipse (label.getX() + 6.0f, y + rowH * 0.5f - 2.5f, 5.0f, 5.0f);

        g.setFont (GrooverLookAndFeel::font (11.0f, true));
        g.setColour (muted ? Palette::faint : (used ? Palette::text : Palette::dim));
        g.drawText (kPartInfo[(size_t) p].shortName, label.withTrimmedLeft (18.0f), juce::Justification::centredLeft, false);

        g.setFont (GrooverLookAndFeel::font (10.0f));
        g.setColour (muted ? Palette::faint : Palette::dim);
        g.drawText (muted ? juce::String ("MUTE") : juce::String (noteName (processor.getNote (p))),
                    label, juce::Justification::centredRight, false);

        // Empty step markers.
        g.setColour (Palette::faint.withAlpha (0.5f));
        for (int s = 0; s < steps; ++s)
        {
            const float cx = area.getX() + ((float) s + 0.5f) * colW;
            g.fillEllipse (cx - 1.0f, y + rowH * 0.5f - 1.0f, 2.0f, 2.0f);
        }
    }

    // Hits.
    const float maxRadius = juce::jmin (colW, rowH) * 0.36f;
    for (const auto& e : events)
    {
        const float v = (float) e.velocity / 127.0f;
        const float r = juce::jmap (v, 0.25f, 1.0f) * maxRadius;
        const float cx = area.getX() + ((float) e.step + 0.5f) * colW;
        const float cy = area.getY() + ((float) e.part + 0.5f) * rowH;
        const bool muted = processor.isPartMuted (e.part);
        const bool lit = e.step == playStep;

        auto colour = muted ? Palette::faint : Palette::forPart (e.part);
        g.setColour (colour.withAlpha (lit ? 1.0f : 0.35f + 0.6f * v));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

        if (lit && ! muted)
        {
            g.setColour (colour.withAlpha (0.25f));
            g.drawEllipse (cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f, 1.0f);
        }
    }

    g.setColour (Palette::line.withAlpha (0.6f));
    g.drawHorizontalLine (juce::roundToInt (area.getBottom()) - 1, area.getX(), area.getRight());
}

void StepGrid::mouseUp (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    if (row < 0 || e.mouseWasDraggedSinceMouseDown())
        return;

    if (auto* param = processor.apvts.getParameter (ParamIDs::mute (row)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (processor.isPartMuted (row) ? 0.0f : 1.0f);
        param->endChangeGesture();
    }
}

void StepGrid::mouseMove (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    if (row != hoverRow)
    {
        hoverRow = row;
        setMouseCursor (row >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        refresh();
    }
}

void StepGrid::mouseExit (const juce::MouseEvent&)
{
    hoverRow = -1;
    refresh();
}
