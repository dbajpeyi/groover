#pragma once

#include "Groove.h"

#include <array>
#include <cstdint>

namespace groover
{

constexpr int kMaxStepsPerBar = 64;
constexpr int kMaxEventsPerBar = kNumParts * kMaxStepsPerBar;

struct GenParams
{
    float swing = 50.0f;       // 50 (straight) .. 75 (dotted); ignored on triplet grids
    float complexity = 0.5f;   // 0 = skeleton, 0.5 = as written, 1 = busy
    float variation = 0.0f;    // 0 = every cycle identical, 1 = constantly changing
    float humanize = 0.0f;     // 0..1, timing and velocity drift
    float velocity = 1.0f;     // overall velocity scale
    int fillEvery = 0;         // 0 = no fills, otherwise a fill in the last bar of every N
    uint32_t seed = 1;
};

struct Event
{
    double position;  // quarter notes from the start of the bar (may be slightly < 0 or > bar length)
    int part;
    int velocity;     // 1..127
    int step;         // grid step the event was derived from
};

struct BarEvents
{
    std::array<Event, kMaxEventsPerBar> events;
    int count = 0;

    const Event* begin() const { return events.data(); }
    const Event* end() const   { return events.data() + count; }
};

// Deterministically renders one bar of a groove. The same (groove, bar, params)
// always yields the same events, so the audio thread, the editor display and
// the MIDI export all agree. Never allocates, so it is safe on the audio thread.
void generateBar (const Groove& groove, int64_t bar, const GenParams& params, BarEvents& out);

// Maps a grid step to its position in quarter notes, applying swing on 8th or
// 16th grids (stepsPerQuarter 2, 4 or 8). Triplet and odd grids are untouched.
double stepToQuarters (int step, int stepsPerQuarter, float swingPercent);

bool isFillBar (int64_t bar, int fillEvery);

} // namespace groover
