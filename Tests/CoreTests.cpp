// Standalone tests for the groove library and engine (no JUCE needed).
//   groover_tests            run all checks
//   groover_tests --list     list every groove
//   groover_tests --print N  print groove N (index or "Category / Name") as a grid

#include "../Source/Core/GrooveEngine.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

using namespace groover;

namespace
{
    int failures = 0;

    void check (bool condition, const std::string& what)
    {
        if (! condition)
        {
            ++failures;
            std::printf ("FAIL: %s\n", what.c_str());
        }
    }

    void printGroove (const Groove& g, const GenParams& params, int64_t bar)
    {
        BarEvents events;
        generateBar (g, bar, params, events);

        std::printf ("%s  [%s, %s, %d bpm]  bar %lld\n", g.key().c_str(), g.origin.c_str(),
                     g.timeSignature.c_str(), g.tempo, (long long) bar);

        for (int p = 0; p < kNumParts; ++p)
        {
            std::string row (g.stepsPerBar, '.');
            for (auto s : g.beatStarts)
                row[(size_t) s] = ',';

            bool any = false;
            for (const auto& e : events)
                if (e.part == p)
                {
                    row[(size_t) e.step] = e.velocity >= 115 ? 'X' : e.velocity >= 90 ? 'x' : e.velocity >= 55 ? 's' : 'g';
                    any = true;
                }

            if (any)
                std::printf ("  %-6s %s\n", kPartInfo[(size_t) p].shortName, row.c_str());
        }
    }

    void testLibrary()
    {
        const auto& lib = GrooveLibrary::all();
        check (lib.size() >= 90, "library has at least 90 grooves (has " + std::to_string (lib.size()) + ")");
        check (GrooveLibrary::categories().size() >= 12, "library has at least 12 categories");

        std::set<std::string> keys;
        for (const auto& g : lib)
        {
            check (g.error.empty(), g.key() + ": " + g.error);
            check (! g.hits.empty(), g.key() + ": has hits");
            check (g.stepsPerBar > 0 && g.stepsPerBar <= kMaxStepsPerBar, g.key() + ": step count in range");
            check (g.tempo >= 40 && g.tempo <= 260, g.key() + ": tempo in range");
            check (g.swing >= 50.0f && g.swing <= 75.0f, g.key() + ": swing in range");
            check (keys.insert (g.key()).second, g.key() + ": unique key");
            check (! g.beatStarts.empty() && g.beatStarts.front() == 0, g.key() + ": beats start on step 0");
            for (size_t i = 1; i < g.beatStarts.size(); ++i)
                check (g.beatStarts[i] > g.beatStarts[i - 1] && g.beatStarts[i] < g.stepsPerBar, g.key() + ": beat groups ascend");
            check (GrooveLibrary::indexOf (g.key()) >= 0, g.key() + ": findable by key");

            // The written time signature must agree with the grid length; 12/8
            // grooves are written as four beats of triplets.
            const auto slash = g.timeSignature.find ('/');
            const double written = std::atof (g.timeSignature.c_str()) * 4.0 / std::atof (g.timeSignature.c_str() + slash + 1);
            const bool tripletTwelveEight = g.timeSignature == "12/8" && g.stepsPerQuarter == 3 && g.stepsPerBar == 12;
            check (tripletTwelveEight || std::abs (written - g.barLengthQuarters()) < 1e-9,
                   g.key() + ": time signature " + g.timeSignature + " matches bar length");
            check (std::abs (g.numerator() * 4.0 / g.denominator() - g.barLengthQuarters()) < 1e-9,
                   g.key() + ": grid meter matches bar length");
        }
    }

    void testDeterminism()
    {
        const auto& g = GrooveLibrary::all()[0];
        GenParams params;
        params.complexity = 0.8f;
        params.variation = 0.6f;
        params.humanize = 0.5f;
        params.fillEvery = 4;
        params.seed = 1234;

        BarEvents a, b;
        for (int64_t bar = -2; bar < 12; ++bar)
        {
            generateBar (g, bar, params, a);
            generateBar (g, bar, params, b);
            bool same = a.count == b.count;
            for (int i = 0; same && i < a.count; ++i)
                same = a.events[(size_t) i].position == b.events[(size_t) i].position
                    && a.events[(size_t) i].velocity == b.events[(size_t) i].velocity
                    && a.events[(size_t) i].part == b.events[(size_t) i].part;
            check (same, "bar " + std::to_string (bar) + " is deterministic");
        }
    }

    void testAsWritten()
    {
        // At the default settings every groove plays exactly what it says.
        GenParams params;
        for (const auto& g : GrooveLibrary::all())
        {
            for (int bar = 0; bar < g.bars; ++bar)
            {
                BarEvents events;
                generateBar (g, bar, params, events);

                int expected = 0;
                for (const auto& h : g.hits)
                    expected += (h.step / g.stepsPerBar == bar) ? 1 : 0;

                check (events.count == expected, g.key() + ": bar " + std::to_string (bar) + " plays as written");

                for (const auto& e : events)
                    check (e.velocity >= 1 && e.velocity <= 127, g.key() + ": velocity in range");
            }
        }
    }

    void testComplexity()
    {
        for (const auto& g : GrooveLibrary::all())
        {
            GenParams params;
            BarEvents base, sparse, busy;
            generateBar (g, 0, params, base);
            params.complexity = 0.0f;
            generateBar (g, 0, params, sparse);
            params.complexity = 1.0f;
            generateBar (g, 0, params, busy);

            check (sparse.count >= 1, g.key() + ": skeleton keeps at least one note");
            check (sparse.count <= base.count, g.key() + ": low complexity never adds notes");
            check (busy.count >= base.count, g.key() + ": high complexity never removes notes");
        }
    }

    void testFills()
    {
        GenParams params;
        params.fillEvery = 4;

        for (const auto& g : GrooveLibrary::all())
        {
            BarEvents fill, after, plain;
            generateBar (g, 3, params, fill);
            generateBar (g, 4, params, after);
            generateBar (g, 2, params, plain);

            bool hasFillVoice = false;
            for (const auto& e : fill)
                hasFillVoice |= (e.part == Snare || e.part == HighTom || e.part == MidTom || e.part == FloorTom)
                             && e.step >= g.stepsPerBar / 2;
            check (hasFillVoice, g.key() + ": bar 4 of 4 has a fill");

            bool crash = false;
            for (const auto& e : after)
                crash |= e.part == Crash && e.step == 0;
            check (crash, g.key() + ": crash after the fill");

            bool plainCrash = false;
            for (const auto& e : plain)
                plainCrash |= e.part == Crash;
            check (! plainCrash, g.key() + ": no crash in an ordinary bar");
        }

        check (! isFillBar (0, 0) && ! isFillBar (3, 0), "fills off when fillEvery is 0");
        check (isFillBar (7, 8) && ! isFillBar (8, 8) && isFillBar (-1, 8), "fill bar arithmetic");
    }

    void testSwing()
    {
        auto near = [] (double a, double b) { return std::abs (a - b) < 1e-9; };

        check (near (stepToQuarters (1, 4, 50.0f), 0.25), "straight 16ths");
        check (near (stepToQuarters (1, 4, 75.0f), 0.375), "dotted 16th swing");
        check (near (stepToQuarters (2, 4, 66.0f), 0.5), "swing leaves 8ths alone on a 16th grid");
        check (near (stepToQuarters (1, 2, 75.0f), 0.75), "8th-note swing");
        check (near (stepToQuarters (2, 8, 75.0f), 0.375), "16th swing on a 32nd grid");
        check (near (stepToQuarters (1, 3, 75.0f), 1.0 / 3.0), "triplet grids are never swung");
    }

    void testHumanize()
    {
        GenParams params;
        params.humanize = 1.0f;

        for (const auto& g : GrooveLibrary::all())
        {
            BarEvents events;
            generateBar (g, 5, params, events);
            for (const auto& e : events)
            {
                const double grid = stepToQuarters (e.step, g.stepsPerQuarter, params.swing);
                check (std::abs (e.position - grid) <= 0.0251, g.key() + ": humanize stays within limits");
                check (e.step != 0 || e.position >= 0.0, g.key() + ": downbeat never moves early");
            }
        }
    }
} // namespace

int main (int argc, char** argv)
{
    const auto& lib = GrooveLibrary::all();

    if (argc >= 2 && std::string (argv[1]) == "--list")
    {
        for (size_t i = 0; i < lib.size(); ++i)
            std::printf ("%3zu  %-42s %-6s %s\n", i, lib[i].key().c_str(), lib[i].timeSignature.c_str(), lib[i].origin.c_str());
        return 0;
    }

    if (argc >= 3 && std::string (argv[1]) == "--print")
    {
        const std::string arg = argv[2];
        int index = std::all_of (arg.begin(), arg.end(), ::isdigit) ? std::atoi (arg.c_str()) : GrooveLibrary::indexOf (arg);
        if (arg == "all")
        {
            for (const auto& g : lib)
                for (int bar = 0; bar < g.bars; ++bar)
                    printGroove (g, {}, bar);
            return 0;
        }
        if (index < 0 || index >= (int) lib.size())
        {
            std::printf ("No such groove: %s\n", arg.c_str());
            return 1;
        }

        GenParams params;
        if (argc >= 4) params.complexity = (float) std::atof (argv[3]);
        if (argc >= 5) params.variation = (float) std::atof (argv[4]);
        if (argc >= 6) params.fillEvery = std::atoi (argv[5]);

        for (int bar = 0; bar < std::max (lib[(size_t) index].bars, params.fillEvery + 1); ++bar)
            printGroove (lib[(size_t) index], params, bar);
        return 0;
    }

    testLibrary();
    testDeterminism();
    testAsWritten();
    testComplexity();
    testFills();
    testSwing();
    testHumanize();

    if (failures == 0)
        std::printf ("All tests passed (%zu grooves in %zu categories).\n", lib.size(), GrooveLibrary::categories().size());
    else
        std::printf ("%d failure(s).\n", failures);

    return failures == 0 ? 0 : 1;
}
