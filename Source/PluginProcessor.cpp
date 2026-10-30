#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace groover;

namespace
{
    constexpr int fillChoices[] = { 0, 2, 4, 8, 16 };
    constexpr double speedChoices[] = { 0.5, 1.0, 2.0 };

    // Drum hits are one-shots; a short gate keeps samplers in gate mode happy.
    constexpr double noteLengthQuarters = 0.125;

    juce::StringArray grooveChoices()
    {
        juce::StringArray names;
        for (const auto& g : GrooveLibrary::all())
            names.add (g.key());
        return names;
    }

    std::unique_ptr<juce::AudioParameterFloat> percent (const juce::String& id, const juce::String& name,
                                                        float min, float max, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (min, max, 1.0f), def,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; })
                .withValueFromStringFunction ([] (const juce::String& t) { return t.retainCharacters ("0123456789.").getFloatValue(); }));
    }
}

GrooverProcessor::GrooverProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                      ),
      apvts (*this, nullptr, "Groover", createLayout())
{
    grooveParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ParamIDs::groove));
    swingParam = apvts.getRawParameterValue (ParamIDs::swing);
    complexityParam = apvts.getRawParameterValue (ParamIDs::complexity);
    variationParam = apvts.getRawParameterValue (ParamIDs::variation);
    humanizeParam = apvts.getRawParameterValue (ParamIDs::humanize);
    velocityParam = apvts.getRawParameterValue (ParamIDs::velocity);
    fillParam = apvts.getRawParameterValue (ParamIDs::fill);
    speedParam = apvts.getRawParameterValue (ParamIDs::speed);

    for (int p = 0; p < kNumParts; ++p)
        muteParams[(size_t) p] = apvts.getRawParameterValue (ParamIDs::mute (p));

    applyKitPreset (KitPreset::GeneralMidi);

    scheduled.reserve (kMaxEventsPerBar * 4);
    active.reserve (256);
}

juce::AudioProcessorValueTreeState::ParameterLayout GrooverProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::groove, 1 }, "Groove", grooveChoices(), 0));
    layout.add (percent (ParamIDs::swing, "Swing", 50.0f, 75.0f, 50.0f));
    layout.add (percent (ParamIDs::complexity, "Complexity", 0.0f, 100.0f, 50.0f));
    layout.add (percent (ParamIDs::variation, "Variation", 0.0f, 100.0f, 0.0f));
    layout.add (percent (ParamIDs::humanize, "Humanize", 0.0f, 100.0f, 10.0f));
    layout.add (percent (ParamIDs::velocity, "Velocity", 10.0f, 150.0f, 100.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::fill, 1 }, "Fill Every",
                                                              juce::StringArray { "Off", "2", "4", "8", "16" }, 3));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::speed, 1 }, "Speed",
                                                              juce::StringArray { "Half", "Normal", "Double" }, 1));

    for (int p = 0; p < kNumParts; ++p)
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::mute (p), 1 },
                                                                juce::String ("Mute ") + kPartInfo[(size_t) p].name, false));

    return layout;
}

//==============================================================================
int GrooverProcessor::getGrooveIndex() const
{
    return juce::jlimit (0, (int) GrooveLibrary::all().size() - 1, grooveParam->getIndex());
}

const Groove& GrooverProcessor::getGroove() const
{
    return GrooveLibrary::all()[(size_t) getGrooveIndex()];
}

GenParams GrooverProcessor::getGenParams() const
{
    GenParams p;
    p.swing = swingParam->load();
    p.complexity = complexityParam->load() / 100.0f;
    p.variation = variationParam->load() / 100.0f;
    p.humanize = humanizeParam->load() / 100.0f;
    p.velocity = velocityParam->load() / 100.0f;
    p.fillEvery = fillChoices[juce::jlimit (0, 4, (int) fillParam->load())];
    p.seed = seed.load();
    return p;
}

double GrooverProcessor::getSpeedFactor() const
{
    return speedChoices[juce::jlimit (0, 2, (int) speedParam->load())];
}

bool GrooverProcessor::isPartMuted (int part) const
{
    return muteParams[(size_t) part]->load() >= 0.5f;
}

void GrooverProcessor::selectGroove (int index)
{
    const auto& lib = GrooveLibrary::all();
    index = juce::jlimit (0, (int) lib.size() - 1, index);

    grooveParam->beginChangeGesture();
    *grooveParam = index;
    grooveParam->endChangeGesture();

    if (auto* swing = apvts.getParameter (ParamIDs::swing))
    {
        swing->beginChangeGesture();
        swing->setValueNotifyingHost (swing->convertTo0to1 (lib[(size_t) index].swing));
        swing->endChangeGesture();
    }
}

void GrooverProcessor::applyKitPreset (KitPreset preset)
{
    const auto map = makeNoteMap (preset);
    for (int p = 0; p < kNumParts; ++p)
        notes[(size_t) p] = map[(size_t) p];
}

//==============================================================================
void GrooverProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    outBuffer.ensureSize ((size_t) juce::jmax (4096, samplesPerBlock * 16));
}

void GrooverProcessor::flushNoteOffs (juce::MidiBuffer& out, int64_t upToSample)
{
    for (size_t i = 0; i < active.size();)
    {
        const auto& a = active[i];
        if (a.offSample <= upToSample)
        {
            out.addEvent (juce::MidiMessage::noteOff (a.channel, a.note), (int) juce::jmax ((int64_t) 0, a.offSample - sampleCounter));
            active[i] = active.back();
            active.pop_back();
        }
        else
        {
            ++i;
        }
    }
}

void GrooverProcessor::releaseAll (juce::MidiBuffer& out, int samplePosition)
{
    for (const auto& a : active)
        out.addEvent (juce::MidiMessage::noteOff (a.channel, a.note), samplePosition);
    active.clear();
}

void GrooverProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();

    const int numSamples = audio.getNumSamples();
    outBuffer.clear();

    // Incoming MIDI: MIDI learn first, then pass everything else through.
    const bool thru = midiThru.load();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int learning = learnPart.load();

        if (learning >= 0 && msg.isNoteOn())
        {
            setNote (learning, msg.getNoteNumber());
            learnPart = -1;
            continue;
        }

        if (thru)
            outBuffer.addEvent (msg, meta.samplePosition);
    }

    // Follow the host transport.
    juce::Optional<juce::AudioPlayHead::PositionInfo> position;
    if (auto* hostPlayHead = getPlayHead())
        position = hostPlayHead->getPosition();

    hostHasTransport = position.hasValue() && position->getPpqPosition().hasValue() && position->getBpm().hasValue();

    const bool playing = position.hasValue() && position->getIsPlaying()
                      && position->getPpqPosition().hasValue() && position->getBpm().hasValue()
                      && *position->getBpm() > 0.0;

    if (! playing)
    {
        if (wasPlaying)
            releaseAll (outBuffer, 0);

        flushNoteOffs (outBuffer, sampleCounter + numSamples - 1);
        wasPlaying = false;
        transportPlaying = false;
        sampleCounter += numSamples;
        midi.swapWith (outBuffer);
        return;
    }

    const double bpm = *position->getBpm();
    const double ppqStart = *position->getPpqPosition();
    const double samplesPerQuarter = sampleRate * 60.0 / bpm;
    const double speed = getSpeedFactor();

    // Work in "groove quarters" so half/double speed simply scales the clock.
    const double start = ppqStart * speed;
    const double end = (ppqStart + numSamples / samplesPerQuarter) * speed;

    const auto& groove = getGroove();
    const auto params = getGenParams();
    const double barLength = groove.barLengthQuarters();
    const int channel = juce::jlimit (1, 16, midiChannel.load());

    // Humanize and swing can push a note slightly over a barline, so look at
    // the neighbouring bars as well.
    const auto firstBar = (int64_t) std::floor (start / barLength) - 1;
    const auto lastBar = (int64_t) std::floor (end / barLength) + 1;

    scheduled.clear();
    for (auto bar = firstBar; bar <= lastBar; ++bar)
    {
        generateBar (groove, bar, params, barEvents);

        for (const auto& e : barEvents)
        {
            if (isPartMuted (e.part))
                continue;

            const double t = (double) bar * barLength + e.position;
            if (t < start || t >= end)
                continue;

            const int sample = juce::jlimit (0, numSamples - 1, (int) ((t / speed - ppqStart) * samplesPerQuarter));
            scheduled.push_back ({ sample, getNote (e.part), e.velocity });
        }
    }

    std::sort (scheduled.begin(), scheduled.end(), [] (const Scheduled& a, const Scheduled& b) { return a.sample < b.sample; });

    const auto noteLength = (int64_t) juce::jmax (1.0, samplesPerQuarter * noteLengthQuarters / speed);

    for (const auto& s : scheduled)
    {
        const int64_t when = sampleCounter + s.sample;
        flushNoteOffs (outBuffer, when);

        // Retriggering a ringing note: close it first.
        for (size_t i = 0; i < active.size(); ++i)
        {
            if (active[i].note == s.note && active[i].channel == channel)
            {
                outBuffer.addEvent (juce::MidiMessage::noteOff (channel, s.note), s.sample);
                active[i] = active.back();
                active.pop_back();
                break;
            }
        }

        outBuffer.addEvent (juce::MidiMessage::noteOn (channel, s.note, (juce::uint8) s.velocity), s.sample);
        active.push_back ({ s.note, channel, when + noteLength });
    }

    if (! scheduled.empty())
        notesSent += (uint32_t) scheduled.size();

    flushNoteOffs (outBuffer, sampleCounter + numSamples - 1);

    wasPlaying = true;
    transportPlaying = true;
    displayPosition = start;
    sampleCounter += numSamples;
    midi.swapWith (outBuffer);
}

//==============================================================================
void GrooverProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("grooveKey", juce::String (getGroove().key()), nullptr);

    juce::ValueTree kit ("Kit");
    for (int p = 0; p < kNumParts; ++p)
        kit.setProperty ("note" + juce::String (p), getNote (p), nullptr);
    kit.setProperty ("channel", midiChannel.load(), nullptr);
    kit.setProperty ("thru", midiThru.load(), nullptr);
    kit.setProperty ("seed", (juce::int64) seed.load(), nullptr);

    state.removeChild (state.getChildWithName ("Kit"), nullptr);
    state.appendChild (kit, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GrooverProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    const auto kit = state.getChildWithName ("Kit");
    const auto grooveKey = state.getProperty ("grooveKey").toString();

    apvts.replaceState (state);

    if (kit.isValid())
    {
        for (int p = 0; p < kNumParts; ++p)
            setNote (p, kit.getProperty ("note" + juce::String (p), kPartInfo[(size_t) p].gmNote));
        midiChannel = juce::jlimit (1, 16, (int) kit.getProperty ("channel", 10));
        midiThru = (bool) kit.getProperty ("thru", true);
        seed = (uint32_t) (juce::int64) kit.getProperty ("seed", 1);
    }

    // Grooves are stored by index; if the library order changed, find it by name.
    const int byKey = GrooveLibrary::indexOf (grooveKey.toStdString());
    if (byKey >= 0 && byKey != grooveParam->getIndex())
        *grooveParam = byKey;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrooverProcessor();
}
