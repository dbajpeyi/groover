#pragma once

#include "Core/GrooveEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>

class GrooverProcessor;

namespace MidiExport
{
    // Renders the current groove and settings to a .mid file in the temp
    // folder, ready to be dragged into the host. Returns the file, or an
    // invalid File on failure.
    juce::File writeCurrentGroove (const GrooverProcessor& processor);

    juce::MidiMessageSequence render (const groover::Groove& groove, const groover::GenParams& params,
                                      const std::function<int (int part)>& noteForPart,
                                      const std::function<bool (int part)>& isMuted,
                                      int channel, double speed, int bars, int ticksPerQuarter);
}
