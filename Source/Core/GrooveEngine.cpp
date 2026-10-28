#include "GrooveEngine.h"

#include <algorithm>
#include <cmath>

namespace groover
{

namespace
{
    uint64_t mix (uint64_t x)
    {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    uint64_t hashOf (uint32_t seed, int64_t a, uint64_t salt)
    {
        return mix ((uint64_t) seed ^ mix ((uint64_t) a ^ mix (salt)));
    }

    struct Rng
    {
        explicit Rng (uint64_t s) : state (s) {}

        float next()
        {
            state = mix (state);
            return (float) (state >> 40) * (1.0f / 16777216.0f);
        }

        int below (int n)
        {
            return std::min (n - 1, (int) (next() * (float) n));
        }

        uint64_t state;
    };

    int64_t floorMod (int64_t a, int64_t n)
    {
        const auto r = a % n;
        return r < 0 ? r + n : r;
    }

    // How strongly a step sits on the pulse: downbeat > beat > 8th > the rest.
    float metricWeight (const Groove& g, int step)
    {
        const int spq = g.stepsPerQuarter;
        if (step == 0)                      return 1.0f;
        if (g.isBeatStart (step))           return 0.8f;
        if (spq == 2)                       return 0.55f;
        if (spq % 2 == 0 && step % (spq / 2) == 0) return 0.55f;
        if (spq == 3 && step % 3 == 2)      return 0.45f;  // the swung "let" of a triplet
        if (spq >= 8 && step % (spq / 4) == 0) return 0.35f;  // 16ths on a 32nd grid
        return spq >= 8 ? 0.2f : 0.3f;
    }

    float importance (const Groove& g, int part, int step, float velocity)
    {
        const bool backbone = (part == Kick || part == Snare) && velocity >= 0.5f;
        return 0.55f * velocity + 0.45f * metricWeight (g, step) + (backbone ? 0.15f : 0.0f);
    }

    using Grid = std::array<std::array<float, kMaxStepsPerBar>, kNumParts>;

    void thin (Grid& grid, const Groove& g, int steps, float threshold)
    {
        int bestPart = -1, bestStep = 0;
        float best = -1.0f, bestVel = 0.0f;
        bool anyLeft = false;

        for (int p = 0; p < kNumParts; ++p)
        {
            for (int s = 0; s < steps; ++s)
            {
                auto& v = grid[(size_t) p][(size_t) s];
                if (v <= 0.0f)
                    continue;

                const auto score = importance (g, p, s, v);
                if (score > best) { best = score; bestPart = p; bestStep = s; bestVel = v; }

                if (score < threshold)
                    v = 0.0f;
                else
                    anyLeft = true;
            }
        }

        if (! anyLeft && bestPart >= 0)
            grid[(size_t) bestPart][(size_t) bestStep] = bestVel;
    }

    int countHits (const Grid& grid, int part, int steps)
    {
        int n = 0;
        for (int s = 0; s < steps; ++s)
            n += grid[(size_t) part][(size_t) s] > 0.0f ? 1 : 0;
        return n;
    }

    void embellish (Grid& grid, const Groove& g, int steps, float amount, Rng& rng)
    {
        auto at = [&grid] (int part, int step) -> float& { return grid[(size_t) part][(size_t) step]; };

        // Ghost notes on the snare (or the rim, for grooves without a snare).
        const int ghostPart = g.usesPart[Snare] ? Snare : (g.usesPart[Rim] ? Rim : -1);
        if (ghostPart >= 0)
        {
            for (int s = 0; s < steps; ++s)
            {
                const bool nearAccent = (s > 0 && at (ghostPart, s - 1) >= 0.79f)
                                     || (s + 1 < steps && at (ghostPart, s + 1) >= 0.79f);
                const bool free = at (ghostPart, s) <= 0.0f && at (Kick, s) <= 0.0f;
                if (free && ! nearAccent && metricWeight (g, s) < 0.8f && rng.next() < amount * 0.3f)
                    at (ghostPart, s) = 0.22f + 0.12f * rng.next();
            }
        }

        // Extra kicks off the beat.
        if (g.usesPart[Kick])
            for (int s = 0; s < steps; ++s)
                if (at (Kick, s) <= 0.0f && at (Snare, s) <= 0.0f && metricWeight (g, s) <= 0.55f
                    && rng.next() < amount * 0.12f)
                    at (Kick, s) = 0.55f + 0.15f * rng.next();

        // Fill gaps in the timekeeping voice.
        const int timePart = countHits (grid, ClosedHat, steps) >= 3 ? ClosedHat
                           : (countHits (grid, Ride, steps) >= 3 ? Ride : -1);
        if (timePart >= 0)
        {
            const int stride = g.stepsPerQuarter >= 8 ? 2 : 1;
            for (int s = 0; s < steps; s += stride)
                if (at (timePart, s) <= 0.0f && at (OpenHat, s) <= 0.0f && rng.next() < amount * 0.2f)
                    at (timePart, s) = 0.35f + 0.15f * rng.next();
        }
    }

    void vary (Grid& grid, const Groove& g, int steps, float amount, Rng& rng)
    {

        // Drop some of the less important notes.
        for (int p = 0; p < kNumParts; ++p)
            for (int s = 0; s < steps; ++s)
            {
                auto& v = grid[(size_t) p][(size_t) s];
                if (v > 0.0f && importance (g, p, s, v) < 0.7f && rng.next() < amount * 0.2f)
                    v = 0.0f;
            }

        // Add a few new ones.
        if (rng.next() < amount)
        {
            const int adds = 1 + rng.below (2 + (int) (amount * 2.0f));
            for (int i = 0; i < adds; ++i)
            {
                const int s = rng.below (steps);
                const int choice = rng.below (3);
                const int part = choice == 0 ? Snare : (choice == 1 ? Kick : ClosedHat);
                if (! g.usesPart[(size_t) part] || metricWeight (g, s) >= 0.8f)
                    continue;

                auto& v = grid[(size_t) part][(size_t) s];
                if (v <= 0.0f)
                    v = part == Snare ? 0.28f : (part == Kick ? 0.62f : 0.45f);
            }
        }

        // Occasionally open a hat on an offbeat.
        if (g.usesPart[ClosedHat] && rng.next() < amount * 0.3f)
        {
            const int start = rng.below (steps);
            for (int i = 0; i < steps; ++i)
            {
                const int s = (start + i) % steps;
                auto& closed = grid[ClosedHat][(size_t) s];
                if (closed > 0.0f && metricWeight (g, s) <= 0.55f)
                {
                    grid[OpenHat][(size_t) s] = std::max (closed, 0.6f);
                    closed = 0.0f;
                    break;
                }
            }
        }
    }

    void writeFill (Grid& grid, const Groove& g, int steps, float complexity, Rng& rng)
    {
        const int spq = g.stepsPerQuarter;

        int length = spq;
        if (steps / spq >= 4 && rng.next() < 0.3f + 0.4f * complexity)
            length = 2 * spq;
        length = std::clamp (length, 1, std::max (1, steps / 2));

        const int start = steps - length;

        for (int p = 0; p < kNumParts; ++p)
        {
            if (p == PedalHat)
                continue;

            for (int s = start; s < steps; ++s)
                if (! (p == Kick && g.isBeatStart (s)))
                    grid[(size_t) p][(size_t) s] = 0.0f;
        }

        int stride = (spq % 2 == 0 && complexity < 0.35f) ? 2 : 1;
        if (spq >= 8)
            stride *= 2;

        const int count = (length + stride - 1) / stride;
        const int style = rng.below (4);

        for (int i = 0; i < count; ++i)
        {
            const int s = start + i * stride;
            const float progress = count > 1 ? (float) i / (float) (count - 1) : 1.0f;
            float vel = 0.55f + 0.4f * progress;
            if (metricWeight (g, s) >= 0.8f)
                vel += 0.1f;
            vel = std::min (vel, 1.0f);

            int voice = Snare;
            switch (style)
            {
                case 0:  // snare, then down the toms
                {
                    static constexpr int voices[] = { Snare, HighTom, MidTom, FloorTom };
                    voice = voices[std::min (3, i * 4 / count)];
                    break;
                }
                case 1:  // toms only, high to low
                {
                    static constexpr int voices[] = { HighTom, MidTom, FloorTom };
                    voice = voices[std::min (2, i * 3 / count)];
                    break;
                }
                case 2:  // snare crescendo, floor tom on the last note
                    voice = (i == count - 1) ? FloorTom : Snare;
                    break;
                default: // hand-hand-foot
                {
                    const bool firstHalf = i < count / 2;
                    const int phase = i % 3;
                    voice = phase == 0 ? (firstHalf ? Snare : HighTom)
                          : phase == 1 ? (firstHalf ? HighTom : FloorTom)
                                       : Kick;
                    break;
                }
            }

            grid[(size_t) voice][(size_t) s] = vel;
        }
    }
} // namespace

bool isFillBar (int64_t bar, int fillEvery)
{
    return fillEvery > 0 && floorMod (bar + 1, fillEvery) == 0;
}

double stepToQuarters (int step, int spq, float swingPercent)
{
    if (! (spq == 2 || spq == 4 || spq == 8) || swingPercent <= 50.0f)
        return (double) step / (double) spq;

    // Swing pairs 8ths on an 8th grid, otherwise 16ths. `unit` is the number of
    // steps in the first half of each pair.
    const int unit = spq == 2 ? 1 : spq / 4;
    const int pair = unit * 2;
    const double ratio = std::clamp ((double) swingPercent, 50.0, 75.0) / 100.0;

    const int base = (step / pair) * pair;
    const int p = step % pair;
    const double split = pair * ratio;

    const double offset = p < unit ? p * split / unit
                                   : split + (p - unit) * (pair - split) / unit;

    return (base + offset) / (double) spq;
}

void generateBar (const Groove& g, int64_t bar, const GenParams& params, BarEvents& out)
{
    out.count = 0;

    const int steps = g.stepsPerBar;
    if (steps <= 0 || steps > kMaxStepsPerBar || g.bars <= 0)
        return;

    const int spq = g.stepsPerQuarter;
    const int64_t patternBar = floorMod (bar, g.bars);

    Grid grid {};
    for (const auto& h : g.hits)
    {
        const int local = h.step - (int) patternBar * steps;
        if (local >= 0 && local < steps)
            grid[(size_t) h.part][(size_t) local] = h.velocity;
    }

    const float complexity = std::clamp (params.complexity, 0.0f, 1.0f);
    const float variation = std::clamp (params.variation, 0.0f, 1.0f);

    // With no variation the embellishments repeat every pattern cycle; with
    // variation some bars draw a fresh set.
    Rng varRng (hashOf (params.seed, bar, 0x5A1));
    const int64_t embellishKey = varRng.next() < variation ? bar : patternBar;

    if (complexity < 0.5f)
        thin (grid, g, steps, (0.5f - complexity) * 2.0f * 0.75f);
    else if (complexity > 0.5f)
    {
        Rng addRng (hashOf (params.seed, embellishKey, 0xADD));
        embellish (grid, g, steps, (complexity - 0.5f) * 2.0f, addRng);
    }

    if (variation > 0.0f)
        vary (grid, g, steps, variation, varRng);

    if (isFillBar (bar, params.fillEvery))
    {
        Rng fillRng (hashOf (params.seed, bar, 0xF11));
        writeFill (grid, g, steps, complexity, fillRng);
    }

    if (bar > 0 && isFillBar (bar - 1, params.fillEvery))
    {
        grid[Crash][0] = 1.0f;
        grid[Kick][0] = std::max (grid[Kick][0], 0.9f);
        grid[ClosedHat][0] = 0.0f;
    }

    const float humanize = std::clamp (params.humanize, 0.0f, 1.0f);

    for (int s = 0; s < steps; ++s)
    {
        const double basePos = stepToQuarters (s, spq, params.swing);

        for (int p = 0; p < kNumParts; ++p)
        {
            const float v = grid[(size_t) p][(size_t) s];
            if (v <= 0.0f)
                continue;

            double timing = 0.0;
            float velJitter = 0.0f;

            if (humanize > 0.0f)
            {
                Rng h (hashOf (params.seed, bar, (uint64_t) (s * kNumParts + p) + 0x4E00));
                timing = (double) ((h.next() + h.next() - 1.0f) * humanize * 0.025f);
                if (s == 0)
                    timing = std::abs (timing);  // never rush the downbeat across the barline
                velJitter = (h.next() * 2.0f - 1.0f) * humanize * 0.12f;
            }

            const int vel = std::clamp ((int) std::lround ((v + velJitter) * params.velocity * 127.0f), 1, 127);
            out.events[(size_t) out.count++] = { basePos + timing, p, vel, s };
        }
    }
}

} // namespace groover
