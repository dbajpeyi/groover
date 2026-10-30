#pragma once

#include "Core/GrooveEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr auto groove     = "groove";
    inline constexpr auto swing      = "swing";
    inline constexpr auto complexity = "complexity";
    inline constexpr auto variation  = "variation";
    inline constexpr auto humanize   = "humanize";
    inline constexpr auto velocity   = "velocity";
    inline constexpr auto fill       = "fill";
    inline constexpr auto speed      = "speed";

    inline juce::String mute (int part) { return "mute" + juce::String (part); }
}

class GrooverProcessor : public juce::AudioProcessor
{
public:
    GrooverProcessor();
    ~GrooverProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override
    {
       #if JucePlugin_IsMidiEffect
        return true;
       #else
        return false;
       #endif
    }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    int getGrooveIndex() const;
    const groover::Groove& getGroove() const;
    groover::GenParams getGenParams() const;
    double getSpeedFactor() const;
    bool isPartMuted (int part) const;

    // Called from the editor: selects a groove and loads its suggested swing.
    void selectGroove (int index);

    // Kit mapping. Not automatable, but saved with the project.
    int getNote (int part) const          { return notes[(size_t) part].load(); }
    void setNote (int part, int note)     { notes[(size_t) part] = juce::jlimit (0, 127, note); }
    void applyKitPreset (groover::KitPreset preset);

    std::atomic<int> midiChannel { 10 };
    std::atomic<bool> midiThru { true };
    std::atomic<uint32_t> seed { 1 };

    // Part waiting for MIDI learn, or -1. The next incoming note-on is assigned to it.
    std::atomic<int> learnPart { -1 };

    // Transport snapshot for the editor, in groove quarters (speed applied).
    std::atomic<double> displayPosition { 0.0 };
    std::atomic<bool> transportPlaying { false };
    std::atomic<bool> hostHasTransport { false };  // false if the host gives no tempo/position at all
    std::atomic<uint32_t> notesSent { 0 };          // counts generated note-ons, for the activity light

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct ActiveNote
    {
        int note;
        int channel;
        int64_t offSample;
    };

    struct Scheduled
    {
        int sample;
        int note;
        int velocity;
    };

    void flushNoteOffs (juce::MidiBuffer& out, int64_t upToSample);
    void releaseAll (juce::MidiBuffer& out, int samplePosition);

    std::array<std::atomic<int>, groover::kNumParts> notes;

    juce::AudioParameterChoice* grooveParam = nullptr;
    std::atomic<float>* swingParam = nullptr;
    std::atomic<float>* complexityParam = nullptr;
    std::atomic<float>* variationParam = nullptr;
    std::atomic<float>* humanizeParam = nullptr;
    std::atomic<float>* velocityParam = nullptr;
    std::atomic<float>* fillParam = nullptr;
    std::atomic<float>* speedParam = nullptr;
    std::array<std::atomic<float>*, groover::kNumParts> muteParams {};

    double sampleRate = 44100.0;
    int64_t sampleCounter = 0;
    bool wasPlaying = false;

    groover::BarEvents barEvents;
    std::vector<Scheduled> scheduled;
    std::vector<ActiveNote> active;
    juce::MidiBuffer outBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooverProcessor)
};
