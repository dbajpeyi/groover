#pragma once

#include <array>
#include <string>

namespace groover
{

// The twelve voices every groove is written for. Each one is mapped to an
// outgoing MIDI note, so the same groove can drive any drum kit layout.
enum Part : int
{
    Kick = 0,
    Snare,
    Rim,        // side stick / cross-stick, also used for clave figures
    ClosedHat,
    PedalHat,
    OpenHat,
    HighTom,
    MidTom,
    FloorTom,   // also carries surdo / conga / dhol bass voices
    Ride,
    Crash,
    Perc,       // cowbell by default; remap to ride bell, clave, tambourine...
    kNumParts
};

struct PartInfo
{
    const char* name;
    const char* shortName;
    int gmNote;
};

inline constexpr std::array<PartInfo, kNumParts> kPartInfo { {
    { "Kick",       "KICK",  36 },
    { "Snare",      "SNR",   38 },
    { "Side Stick", "RIM",   37 },
    { "Closed Hat", "HH",    42 },
    { "Pedal Hat",  "PHH",   44 },
    { "Open Hat",   "OHH",   46 },
    { "High Tom",   "TOM1",  50 },
    { "Mid Tom",    "TOM2",  47 },
    { "Floor Tom",  "TOM3",  43 },
    { "Ride",       "RIDE",  51 },
    { "Crash",      "CRASH", 49 },
    { "Perc",       "PERC",  56 },
} };

using NoteMap = std::array<int, kNumParts>;

enum class KitPreset
{
    GeneralMidi,
    ChromaticC1,   // 36, 37, 38 ... in part order: suits custom Drum Racks / samplers
};

inline NoteMap makeNoteMap (KitPreset preset)
{
    NoteMap map {};
    for (int i = 0; i < kNumParts; ++i)
        map[(size_t) i] = preset == KitPreset::GeneralMidi ? kPartInfo[(size_t) i].gmNote : 36 + i;
    return map;
}

// Note names use Ableton's convention, where middle C (60) is C3.
inline std::string noteName (int note)
{
    static constexpr const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (note < 0 || note > 127)
        return "-";
    return std::string (names[note % 12]) + std::to_string (note / 12 - 2);
}

} // namespace groover
