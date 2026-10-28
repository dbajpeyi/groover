#include "MidiExport.h"
#include "PluginProcessor.h"

using namespace groover;

namespace MidiExport
{

juce::MidiMessageSequence render (const Groove& groove, const GenParams& params,
                                  const std::function<int (int)>& noteForPart,
                                  const std::function<bool (int)>& isMuted,
                                  int channel, double speed, int bars, int ticksPerQuarter)
{
    juce::MidiMessageSequence seq;
    BarEvents events;

    const double barLength = groove.barLengthQuarters();
    const double noteLength = 0.125 * ticksPerQuarter / speed;

    for (int bar = 0; bar < bars; ++bar)
    {
        generateBar (groove, bar, params, events);

        for (const auto& e : events)
        {
            if (isMuted (e.part))
                continue;

            const double tick = juce::jmax (0.0, (bar * barLength + e.position) / speed * ticksPerQuarter);
            const int note = noteForPart (e.part);

            seq.addEvent (juce::MidiMessage::noteOn (channel, note, (juce::uint8) e.velocity), tick);
            seq.addEvent (juce::MidiMessage::noteOff (channel, note), tick + noteLength);
        }
    }

    seq.sort();
    seq.updateMatchedPairs();
    return seq;
}

juce::File writeCurrentGroove (const GrooverProcessor& processor)
{
    constexpr int ticksPerQuarter = 960;

    const auto& groove = processor.getGroove();
    const auto params = processor.getGenParams();
    const double speed = processor.getSpeedFactor();

    // Long enough to include at least one fill.
    const int bars = juce::jmax (8, params.fillEvery);

    auto seq = render (groove, params,
                       [&] (int part) { return processor.getNote (part); },
                       [&] (int part) { return processor.isPartMuted (part); },
                       juce::jlimit (1, 16, processor.midiChannel.load()), speed, bars, ticksPerQuarter);

    const juce::String name = juce::String ("Groover ") + groove.name;

    juce::MidiMessageSequence meta;
    meta.addEvent (juce::MidiMessage::textMetaEvent (3, name), 0.0);
    if (juce::approximatelyEqual (speed, 1.0))
        meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (groove.numerator(), groove.denominator()), 0.0);
    meta.addSequence (seq, 0.0);
    meta.addEvent (juce::MidiMessage::endOfTrack(), bars * groove.barLengthQuarters() / speed * ticksPerQuarter);
    meta.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);
    file.addTrack (meta);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Groover");
    if (! dir.createDirectory())
        return {};

    auto target = dir.getChildFile (juce::File::createLegalFileName (name) + ".mid");
    target.deleteFile();

    juce::FileOutputStream out (target);
    if (! out.openedOk() || ! file.writeTo (out))
        return {};

    return target;
}

} // namespace MidiExport
