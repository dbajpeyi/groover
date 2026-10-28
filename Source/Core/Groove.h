#pragma once

#include "DrumKit.h"

#include <string>
#include <vector>

namespace groover
{

struct Hit
{
    int step;        // absolute step within the whole groove (all bars)
    int part;
    float velocity;  // 0..1
};

// One groove from the library. Steps are a fixed grid of `stepsPerQuarter`
// subdivisions per quarter note, so a bar lasts stepsPerBar / stepsPerQuarter
// quarter notes. Triplet feels use 3 (or 6) steps per quarter, odd meters
// such as 7/8 use 2 steps per quarter with an odd step count.
struct Groove
{
    std::string category;
    std::string name;
    std::string origin;
    std::string timeSignature;   // as musicians write it, e.g. "12/8" or "7/8"
    std::string notes;
    int stepsPerQuarter = 4;
    int stepsPerBar = 16;
    int bars = 1;
    int tempo = 120;      // suggested tempo, quarter-note BPM
    float swing = 50.0f;  // suggested swing, 50 (straight) .. 75 (dotted)

    std::vector<Hit> hits;  // sorted by step
    std::vector<int> beatStarts;  // steps within a bar where a counted beat starts, e.g. 0, 2, 4 for 7/8 as 2+2+3
    std::array<bool, kNumParts> usesPart {};
    std::string error;      // empty when the definition parsed cleanly

    double barLengthQuarters() const   { return (double) stepsPerBar / (double) stepsPerQuarter; }
    int totalSteps() const             { return stepsPerBar * bars; }
    bool isBeatStart (int step) const
    {
        for (auto b : beatStarts)
            if (b == step)
                return true;
        return false;
    }

    std::string key() const            { return category + " / " + name; }

    // The meter of the grid itself, used for MIDI files.
    int numerator() const;
    int denominator() const;
};

class GrooveLibrary
{
public:
    static const std::vector<Groove>& all();
    static const std::vector<std::string>& categories();

    // Index into all() for a key() (or, failing that, a bare name); -1 if not found.
    static int indexOf (const std::string& name);
};

// Parses one lane of a groove definition. Spaces and '|' are ignored, so lanes
// can be grouped by beat. Velocity symbols:
//   .  rest      g  ghost      s  soft      x  normal      X  accent
// Returns false (and leaves `out` partially filled) on an unknown symbol.
bool parseLane (const char* text, std::vector<float>& out);

} // namespace groover
