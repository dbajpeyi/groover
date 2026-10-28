#include "Groove.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

namespace groover
{

namespace
{
    // Short lane names keep the pattern tables below readable.
    constexpr Part K = Kick, S = Snare, R = Rim, H = ClosedHat, P = PedalHat, O = OpenHat,
                   T1 = HighTom, T2 = MidTom, T3 = FloorTom, Y = Ride, B = Perc;

    using Lanes = std::initializer_list<std::pair<Part, const char*>>;

    struct Meta
    {
        const char* category;
        const char* name;
        const char* origin;
        const char* timeSignature;
        int stepsPerQuarter;
        int stepsPerBar;
        int bars;
        int tempo;
        float swing;
        const char* notes;
    };

    Groove make (const Meta& m, Lanes lanes)
    {
        Groove g;
        g.category = m.category;
        g.name = m.name;
        g.origin = m.origin;
        g.timeSignature = m.timeSignature;
        g.notes = m.notes;
        g.stepsPerQuarter = m.stepsPerQuarter;
        g.stepsPerBar = m.stepsPerBar;
        g.bars = m.bars;
        g.tempo = m.tempo;
        g.swing = m.swing;

        // The spacing of the first lane marks the beats, so odd meters keep
        // their grouping (e.g. "Xx Xx Xxx" is 2+2+3).
        if (lanes.size() > 0)
        {
            int index = 0;
            bool groupStart = true;
            for (const char* c = lanes.begin()->second; *c != 0; ++c)
            {
                if (*c == ' ' || *c == '|')
                {
                    groupStart = true;
                    continue;
                }
                if (groupStart && index < g.stepsPerBar)
                    g.beatStarts.push_back (index);
                groupStart = false;
                ++index;
            }
        }

        if (g.beatStarts.size() <= 1)
        {
            g.beatStarts.clear();
            for (int s = 0; s < g.stepsPerBar; s += g.stepsPerQuarter)
                g.beatStarts.push_back (s);
        }

        std::vector<float> steps;

        for (const auto& [part, text] : lanes)
        {
            if (! parseLane (text, steps))
            {
                g.error += std::string (kPartInfo[(size_t) part].name) + " lane has an unknown symbol. ";
                continue;
            }

            if ((int) steps.size() != g.totalSteps())
            {
                g.error += std::string (kPartInfo[(size_t) part].name) + " lane has " + std::to_string (steps.size())
                         + " steps, expected " + std::to_string (g.totalSteps()) + ". ";
                continue;
            }

            for (int i = 0; i < (int) steps.size(); ++i)
                if (steps[(size_t) i] > 0.0f)
                    g.hits.push_back ({ i, (int) part, steps[(size_t) i] });

            g.usesPart[(size_t) part] = true;
        }

        std::stable_sort (g.hits.begin(), g.hits.end(), [] (const Hit& a, const Hit& b) { return a.step < b.step; });
        return g;
    }

    std::vector<Groove> buildLibrary()
    {
        std::vector<Groove> lib;
        auto add = [&lib] (const Meta& m, Lanes lanes) { lib.push_back (make (m, lanes)); };

        //==============================================================================
        // Rock
        add ({ "Rock", "Straight Eighths", "USA / UK", "4/4", 4, 16, 1, 110, 50, "Kick on 1 and 3, snare on 2 and 4: the backbone of rock." },
             { { H, "X.x. X.x. X.x. X.x." },
               { S, ".... X... .... X..." },
               { K, "x... .... x.x. ...." } });

        add ({ "Rock", "Sixteenth Hats", "USA / UK", "4/4", 4, 16, 1, 100, 50, "Driving 16th hi-hats with an anticipated kick." },
             { { H, "Xsxs Xsxs Xsxs Xsxs" },
               { S, ".... X... .... X..." },
               { K, "x... ..x. x... ...." } });

        add ({ "Rock", "Half-Time", "USA / UK", "4/4", 4, 16, 1, 140, 50, "Snare on 3 halves the felt pulse." },
             { { H, "X.x. x.x. X.x. x.x." },
               { S, ".... .... X... ...." },
               { K, "x... ..x. .... ..x." } });

        add ({ "Rock", "Four on the Floor", "USA / UK", "4/4", 4, 16, 1, 120, 50, "Kick on every beat with an open hat lift into the next bar." },
             { { H, "x.x. x.x. x.x. x..." },
               { O, ".... .... .... ..x." },
               { S, ".... X... .... X..." },
               { K, "x... x... x... x..." } });

        add ({ "Rock", "Ride Groove", "USA / UK", "4/4", 4, 16, 1, 120, 50, "Bigger, washier time on the ride with the hat foot on 2 and 4." },
             { { Y, "X.x. X.x. X.x. X.x." },
               { P, ".... x... .... x..." },
               { S, ".... X... .... X..." },
               { K, "x... ...x x.x. ...." } });

        add ({ "Rock", "Rock Shuffle", "USA", "4/4", 3, 12, 1, 120, 50, "Triplet shuffle on the hats, backbeat on 2 and 4." },
             { { H, "X.x X.x X.x X.x" },
               { S, "... X.. ... X.." },
               { K, "x.. ... x.x ..." } });

        add ({ "Rock", "12/8 Ballad", "USA / UK", "12/8", 3, 12, 1, 66, 50, "Slow triplet ballad feel, written as four beats of three." },
             { { H, "Xss xss Xss xss" },
               { S, "... X.. ... X.." },
               { K, "x.. ..x x.. ..." } });

        add ({ "Rock", "Tribal Toms", "USA", "4/4", 4, 16, 1, 130, 50, "Floor-tom ostinato over a four-on-the-floor kick." },
             { { T3, "X..x X... X..x X..." },
               { T2, ".... ..x. .... ..x." },
               { K,  "x... x... x... x..." } });

        add ({ "Rock", "Punk Skank", "UK / USA", "4/4", 2, 8, 1, 180, 50, "Kick and snare trade eighth notes under constant cymbals." },
             { { H, "Xx Xx Xx Xx" },
               { K, "x. x. x. x." },
               { S, ".X .X .X .X" } });

        add ({ "Rock", "Metal Gallop", "UK / USA", "4/4", 4, 16, 1, 160, 50, "Eighth-two-sixteenths kick gallop." },
             { { H, "x... x... x... x..." },
               { S, ".... X... .... X..." },
               { K, "x.xx x.xx x.xx x.xx" } });

        add ({ "Rock", "Double Kick", "Worldwide", "4/4", 4, 16, 1, 150, 50, "Continuous 16th double kick under a ride and backbeat." },
             { { Y, "X... x... X... x..." },
               { S, ".... X... .... X..." },
               { K, "xxxx xxxx xxxx xxxx" } });

        add ({ "Rock", "Traditional Blast", "Worldwide", "4/4", 4, 16, 1, 180, 50, "Kick and cymbal together, snare in between." },
             { { Y, "x.x. x.x. x.x. x.x." },
               { K, "x.x. x.x. x.x. x.x." },
               { S, ".x.x .x.x .x.x .x.x" } });

        //==============================================================================
        // Pop & Soul
        add ({ "Pop & Soul", "Motown", "Detroit, USA", "4/4", 4, 16, 1, 120, 50, "Snare on all four beats, leaning on 2 and 4." },
             { { S, "x... X... x... X..." },
               { H, "x.x. x.x. x.x. x.x." },
               { K, "x... ..x. x... ..x." } });

        add ({ "Pop & Soul", "Disco", "New York / Philadelphia", "4/4", 4, 16, 1, 118, 50, "Four on the floor with open hats on every offbeat." },
             { { K, "x... x... x... x..." },
               { S, ".... X... .... X..." },
               { H, "x..s x..s x..s x..s" },
               { O, "..x. ..x. ..x. ..x." } });

        add ({ "Pop & Soul", "Pop Ballad", "Worldwide", "4/4", 4, 16, 1, 72, 50, "Soft eighths with cross-stick backbeat." },
             { { H, "x.s. x.s. x.s. x.s." },
               { R, ".... x... .... x..." },
               { K, "x... ...x ..x. ...." } });

        add ({ "Pop & Soul", "Eighties Synth Pop", "UK / USA", "4/4", 4, 16, 1, 118, 50, "Machine-tight eighths with an open hat on the and of 3." },
             { { H, "x.x. x.x. x... x.x." },
               { O, ".... .... ..x. ...." },
               { S, ".... X... .... X..." },
               { K, "x... ..x. x... ...." } });

        add ({ "Pop & Soul", "Modern Pop", "Worldwide", "4/4", 4, 16, 1, 100, 50, "Syncopated kick under a steady backbeat." },
             { { H, "x.xs x.xs x.xs x.xs" },
               { S, ".... X... .... X..." },
               { K, "x... .x.. ..x. ...." } });

        //==============================================================================
        // Funk
        add ({ "Funk", "Ghost Note Funk", "USA", "4/4", 4, 16, 1, 96, 50, "Sixteenth ghost notes around a solid backbeat." },
             { { H, "x.x. x.x. x.x. x..." },
               { O, ".... .... .... ..x." },
               { S, ".g.. X..g .g.g X..g" },
               { K, "x.x. ...x ..x. ...." } });

        add ({ "Funk", "Drummer's Break", "Augusta, USA", "4/4", 4, 16, 1, 100, 50, "Sixteenth hats and a skipping kick, in the late-60s James Brown style." },
             { { H, "xxxx xxxx xxxx xx.x" },
               { O, ".... .... .... ..x." },
               { S, ".... X..g .g.g X..g" },
               { K, "x.x. .... ..x. .x.." } });

        add ({ "Funk", "Linear Funk", "USA", "4/4", 4, 16, 1, 98, 50, "No two voices ever land together." },
             { { K, "x..x ..x. ..x. .x.." },
               { S, ".... X..g .... X..g" },
               { H, ".xx. .x.. xx.x ..x." } });

        add ({ "Funk", "Half-Time Shuffle", "USA", "4/4", 3, 12, 1, 104, 50, "Triplet hats with ghosted middle partials, backbeat on 3." },
             { { H, "x.x x.x x.x x.x" },
               { S, ".g. .g. X.. .g." },
               { K, "x.. ..x ... ..x" } });

        add ({ "Funk", "Second Line", "New Orleans, USA", "4/4", 4, 16, 1, 110, 58, "Street-parade snare chatter over a rolling bass drum." },
             { { S, "x.gg x.gx .gx. X.gg" },
               { K, "x... ..x. ...x ..x." } });

        add ({ "Funk", "Go-Go", "Washington DC, USA", "4/4", 4, 16, 1, 96, 60, "Swung pocket with a cowbell part on top." },
             { { H, "x.x. x.xx x.x. x.x." },
               { S, ".... X..g ..g. X..." },
               { K, "x... ..x. ..x. ...." },
               { B, "x..x ..x. ..x. x..." } });

        //==============================================================================
        // Jazz
        add ({ "Jazz", "Medium Swing", "USA", "4/4", 3, 12, 1, 140, 50, "Spang-a-lang ride, hat foot on 2 and 4, feathered kick." },
             { { Y, "x.. X.s x.. X.s" },
               { P, "... x.. ... x.." },
               { K, "g.. g.. g.. g.." },
               { S, "... ... ..g ..." } });

        add ({ "Jazz", "Up-Tempo Bebop", "USA", "4/4", 3, 12, 1, 220, 50, "Light ride time with sparse snare comping and a kick bomb." },
             { { Y, "x.. X.s x.. X.s" },
               { P, "... x.. ... x.." },
               { S, "... ..g ... .s." },
               { K, "... ... ... ..x" } });

        add ({ "Jazz", "Two Feel", "USA", "4/4", 3, 12, 1, 120, 50, "Kick on 1 and 3 with a cross-stick on 4." },
             { { Y, "x.. X.s x.. X.s" },
               { P, "... x.. ... x.." },
               { K, "s.. ... s.. ..." },
               { R, "... ... ... x.." } });

        add ({ "Jazz", "Ballad", "USA", "4/4", 3, 12, 1, 60, 50, "Quiet ride and cross-stick for slow tunes." },
             { { Y, "s.. x.g s.. x.g" },
               { P, "... s.. ... s.." },
               { K, "g.. ... g.. ..." },
               { R, "... ... ... s.." } });

        add ({ "Jazz", "Jazz Waltz", "USA", "3/4", 3, 9, 1, 150, 50, "Swung three with the ride skip on beat 2." },
             { { Y, "x.. X.s x.." },
               { P, "... x.. x.." },
               { K, "s.. ... ..." },
               { S, "... ... ..g" } });

        add ({ "Jazz", "Five-Four Swing", "USA", "5/4", 3, 15, 1, 170, 50, "3 + 2 phrasing in the spirit of Take Five." },
             { { Y, "x.. X.s x.. X.. X.s" },
               { P, "... x.. ... x.. ..." },
               { S, "... ... ..g x.. x.." },
               { K, "s.. ... s.. ... ..." } });

        add ({ "Jazz", "Straight Eighths", "Europe / USA", "4/4", 4, 16, 1, 110, 50, "Even-eighth modern jazz ride with cross-stick on 4." },
             { { Y, "x.x. x.xx x.x. x.x." },
               { P, ".... x... .... x..." },
               { R, ".... .... .... x..." },
               { K, "x... .... ..x. ...." } });

        //==============================================================================
        // Blues & Shuffle
        add ({ "Blues & Shuffle", "Chicago Shuffle", "Chicago, USA", "4/4", 3, 12, 1, 120, 50, "Shuffle hats, backbeat, kick on 1 and 3." },
             { { H, "x.x x.x x.x x.x" },
               { S, "... X.. ... X.." },
               { K, "x.. ... x.. ..." } });

        add ({ "Blues & Shuffle", "Texas Double Shuffle", "Texas, USA", "4/4", 3, 12, 1, 130, 50, "Both hands shuffle; kick on every beat." },
             { { Y, "x.x x.x x.x x.x" },
               { S, "s.s X.s s.s X.s" },
               { K, "x.. x.. x.. x.." } });

        add ({ "Blues & Shuffle", "Slow Blues", "USA", "12/8", 3, 12, 1, 60, 50, "Triplets on the ride, heavy 2 and 4." },
             { { Y, "Xss Xss Xss Xss" },
               { S, "... X.. ... X.." },
               { K, "x.. ... x.x ..." },
               { P, "... x.. ... x.." } });

        add ({ "Blues & Shuffle", "Bo Diddley Beat", "Chicago, USA", "4/4", 4, 16, 1, 115, 50, "3-2 clave on the toms with maracas on top." },
             { { T3, "x..x ..x. ..x. x..." },
               { K,  "x... .... x... ...." },
               { H,  "xsxs xsxs xsxs xsxs" } });

        //==============================================================================
        // Hip-Hop & R&B
        add ({ "Hip-Hop & R&B", "Boom Bap", "New York, USA", "4/4", 4, 16, 1, 90, 58, "Swung eighth hats, hard snare, kick on 1, 2a and 3&." },
             { { K, "x... ...x ..x. ...." },
               { S, ".... X... .... X..." },
               { H, "x.x. x.x. x.x. x.x." } });

        add ({ "Hip-Hop & R&B", "Dusty Boom Bap", "New York, USA", "4/4", 4, 16, 1, 88, 62, "Lazier swing and a busier kick." },
             { { K, "x..x .... ..x. .x.." },
               { S, ".... X... .... X..g" },
               { H, "x.xx x.x. x.xx x.x." } });

        add ({ "Hip-Hop & R&B", "Trap", "Atlanta, USA", "4/4", 8, 32, 1, 140, 50, "Half-time snare with 16th hats and a 32nd-note roll." },
             { { H, "x.x.x.x. x.x.x.x. x.x.x.x. x.x.ssxx" },
               { S, "........ ........ X....... ........" },
               { K, "x....... ..x..... ....x... ......x." } });

        add ({ "Hip-Hop & R&B", "New Jack Swing", "USA", "4/4", 4, 16, 1, 104, 62, "Swung 16ths and a bouncing kick." },
             { { K, "x..x ..x. x... ..x." },
               { S, ".... X..g .... X..." },
               { H, "xsxs xsxs xsxs xsxs" } });

        add ({ "Hip-Hop & R&B", "Neo-Soul Pocket", "USA", "4/4", 4, 16, 1, 84, 58, "Behind-the-beat feel; add humanize for extra lean." },
             { { K, "x... ..x. .x.. ...." },
               { S, ".g.. X..g .... X.g." },
               { H, "x.x. x.xx x.x. x.x." } });

        add ({ "Hip-Hop & R&B", "Slow Jam 12/8", "USA", "12/8", 3, 12, 1, 66, 50, "R&B ballad triplets." },
             { { H, "xss xss xss xss" },
               { K, "x.. ..x x.. .x." },
               { S, "... X.. ... X.." } });

        add ({ "Hip-Hop & R&B", "G-Funk", "Los Angeles, USA", "4/4", 4, 16, 1, 92, 56, "Laid-back West Coast bounce." },
             { { K, "x... ...x x.x. ...." },
               { S, ".... X... .... X..." },
               { H, "xsxs xsxs xsxs xsxs" } });

        //==============================================================================
        // Electronic
        add ({ "Electronic", "House", "Chicago, USA", "4/4", 4, 16, 1, 124, 50, "Four on the floor, offbeat open hats." },
             { { K, "x... x... x... x..." },
               { S, ".... X... .... X..." },
               { O, "..x. ..x. ..x. ..x." },
               { H, ".s.s .s.s .s.s .s.s" } });

        add ({ "Electronic", "Deep House", "USA / UK", "4/4", 4, 16, 1, 120, 56, "Swung shuffle with a rim on 2 and 4." },
             { { K, "x... x... x... x..." },
               { R, ".... x... .... x..." },
               { O, "..x. ..x. ..x. ..x." },
               { H, "s..s s..s s..s s..s" } });

        add ({ "Electronic", "Techno", "Detroit / Berlin", "4/4", 4, 16, 1, 130, 50, "Relentless kick, 16th hats, offbeat ride." },
             { { K, "x... x... x... x..." },
               { S, ".... x... .... x..." },
               { H, "xsxs xsxs xsxs xsxs" },
               { Y, "..x. ..x. ..x. ..x." } });

        add ({ "Electronic", "Classic Break", "UK", "4/4", 4, 16, 2, 136, 50, "Two-bar break in the style of the classic funk breaks." },
             { { Y, "x.x. x.x. x.x. x.x. x.x. x.x. x.x. x.x." },
               { K, "x.x. .... ..xx .... x.x. .... ..x. .x.." },
               { S, ".... X..g .g.. X..g .g.. X..g .g.. ..X." } });

        add ({ "Electronic", "Drum & Bass", "UK", "4/4", 4, 16, 1, 172, 50, "Two-step: kick on 1 and 3&, snare on 2 and 4." },
             { { K, "x... .... ..x. ...." },
               { S, ".... X... .g.. X..." },
               { H, "x.x. x.x. x.x. x.x." } });

        add ({ "Electronic", "Jungle", "UK", "4/4", 4, 16, 2, 165, 50, "Chopped two-bar break with ghosted snares." },
             { { Y, "x.x. x.x. x.x. x.x. x.x. x.x. x.x. x.x." },
               { K, "x... .... ..x. .... ..x. .... .x.. ...." },
               { S, ".... X..g .g.. X..g .g.. X..g .g.X ..X." } });

        add ({ "Electronic", "Dubstep", "London, UK", "4/4", 4, 16, 1, 140, 50, "Half-time snare on 3." },
             { { K, "x... .... .... ..x." },
               { S, ".... .... X... ...." },
               { H, "x.x. x.x. x.x. x.x." } });

        add ({ "Electronic", "UK Garage 2-Step", "London, UK", "4/4", 4, 16, 1, 132, 62, "Skippy swung kick with no kick on 2 and 4." },
             { { K, "x... .... ..x. .x.." },
               { S, ".... X... .... X..." },
               { H, "..x. ..xx ..x. ..x." } });

        add ({ "Electronic", "Electro", "New York, USA", "4/4", 4, 16, 1, 120, 50, "808-style syncopated kick with cowbell stabs." },
             { { K, "x... ..x. ..x. ...." },
               { S, ".... X... .... X..." },
               { H, "xxxx xxxx xxxx xxxx" },
               { B, "..x. .... ..x. ...." } });

        add ({ "Electronic", "Footwork", "Chicago, USA", "4/4", 4, 16, 1, 160, 50, "3-3-2 kick figures at 160." },
             { { K, "x..x ..x. x..x ..x." },
               { S, ".... .... X... ...." },
               { H, "..x. ..x. ..x. ..x." } });

        //==============================================================================
        // Afro-Cuban
        add ({ "Afro-Cuban", "Songo", "Havana, Cuba", "4/4", 4, 16, 1, 100, 50, "Rumba clave on the rim, bell quarters, tumbao kick." },
             { { R,  "x..x ...x ..x. x..." },
               { B,  "x... x... x... x..." },
               { K,  ".... ..x. .... x..." },
               { S,  "..g. .g.. ..gX .g.." },
               { T3, ".... ...x .... ..x." } });

        add ({ "Afro-Cuban", "Mambo (Cascara)", "Havana, Cuba", "4/4", 4, 16, 1, 100, 50, "3-2 cascara on the shell or ride with son clave on the rim." },
             { { Y,  "x.xx .x.x x.x. xx.x" },
               { R,  "x..x ..x. ..x. x..." },
               { K,  ".... ..x. .... x..." },
               { T3, ".... .... .... x.x." } });

        add ({ "Afro-Cuban", "Cha-Cha-Cha", "Havana, Cuba", "4/4", 4, 16, 1, 116, 50, "Bell quarters, guiro long-short-short, conga cha-cha on 4." },
             { { B,  "X... x... X... x..." },
               { H,  "x... x.x. x... x.x." },
               { R,  ".... x... .... ...." },
               { T3, ".... .... .... x.x." },
               { K,  "x... .... x... ...." } });

        add ({ "Afro-Cuban", "Bolero", "Cuba", "4/4", 4, 16, 1, 76, 50, "Soft maracas, cross-stick and bongo-like accents." },
             { { H,  "s.s. s.s. s.s. s.s." },
               { R,  ".... x..x .... x..." },
               { K,  "x... .... x... ...." },
               { T3, ".... .... .... ..x." } });

        add ({ "Afro-Cuban", "6/8 Bembe", "Cuba / West Africa", "12/8", 3, 12, 1, 110, 50, "Standard 7-stroke bell over a four-beat foot pulse." },
             { { B,  "x.x .xx .x. x.x" },
               { K,  "x.. ... x.. ..." },
               { P,  "... x.. ... x.." },
               { T2, "..x ... ..x ..." },
               { T3, "... ..x ... ..x" } });

        //==============================================================================
        // Brazilian
        add ({ "Brazilian", "Samba", "Rio de Janeiro, Brazil", "4/4", 4, 16, 1, 100, 50, "Surdo-style kick, teleco-teco cross-stick, 16th hats." },
             { { K, "x..x x..x x..x x..x" },
               { P, "..x. ..x. ..x. ..x." },
               { H, "xssX xssX xssX xssX" },
               { R, "x.xx .x.x .xx. x.x." } });

        add ({ "Brazilian", "Bossa Nova", "Rio de Janeiro, Brazil", "4/4", 2, 8, 2, 130, 50, "Two-bar bossa clave on the cross-stick, dotted kick." },
             { { R, "x. .x .. x. .. x. .x .." },
               { K, "x. .x x. .x x. .x x. .x" },
               { H, "xs xs xs xs xs xs xs xs" } });

        add ({ "Brazilian", "Baiao", "Northeast Brazil", "4/4", 4, 16, 1, 100, 50, "Zabumba dotted bass with triangle 16ths." },
             { { K, "x..x .... x..x ...." },
               { R, "..x. ..x. ..x. ..x." },
               { H, "sxXx sxXx sxXx sxXx" } });

        add ({ "Brazilian", "Samba-Reggae", "Salvador, Brazil", "4/4", 4, 16, 1, 100, 50, "Bloco-style snare figure with surdo answers." },
             { { S,  "X..x ..x. ..X. .x.." },
               { K,  "x... .... x... ..x." },
               { T3, ".... x... .... x..." },
               { H,  "x.x. x.x. x.x. x.x." } });

        //==============================================================================
        // Latin America
        add ({ "Latin America", "Cumbia", "Colombia", "4/4", 4, 16, 1, 95, 50, "Guiro scrape pattern with llamador on the offbeats." },
             { { H,  "x.xx x.xx x.xx x.xx" },
               { K,  "x... .... x... ...." },
               { R,  "..x. ..x. ..x. ..x." },
               { T3, "x... ...x x... ...x" } });

        add ({ "Latin America", "Merengue", "Dominican Republic", "4/4", 4, 16, 1, 130, 50, "Constant guira 16ths over a pulsing kick." },
             { { H,  "Xxxx Xxxx Xxxx Xxxx" },
               { K,  "x... x... x... x..." },
               { T3, ".... ..x. .... x.x." } });

        add ({ "Latin America", "Bachata", "Dominican Republic", "4/4", 4, 16, 1, 128, 50, "Guira scrape and bongo martillo." },
             { { H,  "x.xx x.xx x.xx x.xx" },
               { T1, "x.s. x.s. x.s. x.s." },
               { K,  "x... .... x... ...." },
               { T3, ".... .... .... x.x." } });

        add ({ "Latin America", "Reggaeton (Dembow)", "Puerto Rico / Panama", "4/4", 4, 16, 1, 95, 50, "Four-on-the-floor kick with the 3-3-2 dembow snare." },
             { { K, "x... x... x... x..." },
               { S, "...x ..x. ...x ..x." },
               { H, "x.x. x.x. x.x. x.x." } });

        add ({ "Latin America", "Habanera / Tango", "Cuba / Argentina", "4/4", 4, 16, 1, 80, 50, "Dotted habanera bass figure." },
             { { K, "x..x x.x. x..x x.x." },
               { R, ".... x... .... x..." },
               { H, "s.s. s.s. s.s. s.s." } });

        //==============================================================================
        // Caribbean
        add ({ "Caribbean", "One Drop", "Jamaica", "4/4", 4, 16, 1, 75, 62, "Kick and rim together on 3 only." },
             { { K, ".... .... x... ...." },
               { R, ".... .... x... ...." },
               { H, "x.x. x.xx x.x. x.x." } });

        add ({ "Caribbean", "Rockers", "Jamaica", "4/4", 4, 16, 1, 80, 58, "Kick on every beat, rim on 2 and 4." },
             { { K, "x... x... x... x..." },
               { R, ".... x... .... x..." },
               { H, "s.x. s.x. s.x. s.x." } });

        add ({ "Caribbean", "Steppers", "Jamaica / UK", "4/4", 4, 16, 1, 78, 50, "Four on the floor with the rim on 3." },
             { { K, "x... x... x... x..." },
               { R, ".... .... x... ...." },
               { H, "xsxs xsxs xsxs xsxs" } });

        add ({ "Caribbean", "Ska", "Jamaica", "4/4", 4, 16, 1, 140, 50, "Backbeat snare with open hats on the upbeats." },
             { { K, "x... .... x... ...." },
               { S, ".... X... .... X..." },
               { O, "..x. ..x. ..x. ..x." },
               { H, "x... x... x... x..." } });

        add ({ "Caribbean", "Calypso", "Trinidad & Tobago", "4/4", 4, 16, 1, 110, 50, "Habanera-like kick and offbeat cowbell." },
             { { K, "x..x x... x..x x..." },
               { S, ".... x... .... x..." },
               { B, "..x. ..x. ..x. ..x." } });

        add ({ "Caribbean", "Soca", "Trinidad & Tobago", "4/4", 4, 16, 1, 150, 50, "Driving four-on-the-floor with offbeat open hats." },
             { { K, "x... x... x... x..." },
               { S, ".... X..x .... X..." },
               { O, "..x. ..x. ..x. ..x." },
               { H, "x..x x..x x..x x..x" } });

        add ({ "Caribbean", "Dancehall", "Jamaica", "4/4", 4, 16, 1, 96, 50, "3-3-2 tresillo kick against a straight backbeat." },
             { { K, "x..x ..x. x..x ..x." },
               { S, ".... X... .... X..." },
               { H, "x.x. x.x. x.x. x.x." } });

        //==============================================================================
        // African
        add ({ "African", "Afrobeat", "Lagos, Nigeria", "4/4", 4, 16, 1, 110, 50, "Tony Allen-style interlocking kick, ghosted snare and clave." },
             { { K, "x..x ..x. x... .x.." },
               { S, "..g. X..g .g.. X.g." },
               { H, "x.xs x.xs x.xs x.x." },
               { O, ".... .... .... ...x" },
               { B, "x..x ..x. ..x. x..." } });

        add ({ "African", "Highlife", "Ghana", "4/4", 4, 16, 1, 120, 50, "Bell timeline over a lilting kick." },
             { { B, "x.x. xx.x .x.x x.x." },
               { K, "x... ..x. x... ..x." },
               { R, ".... x... .... x..." },
               { H, "..x. ..x. ..x. ..x." } });

        add ({ "African", "Agbekor 12/8", "Ewe, Ghana / Togo", "12/8", 3, 12, 1, 120, 50, "Standard bell with supporting drums." },
             { { B,  "x.x .xx .x. x.x" },
               { K,  "x.. ... x.. ..." },
               { T2, "..x ..x ..x ..x" },
               { T3, "... x.. ... x.." },
               { H,  "x.x x.x x.x x.x" } });

        add ({ "African", "Afrobeats", "Nigeria / Ghana", "4/4", 4, 16, 1, 104, 54, "3-3-2 rim over a shaker groove." },
             { { K, "x..x ..x. x... ...." },
               { R, "...x ..x. ...x ..x." },
               { H, "x.xx x.xx x.xx x.xx" } });

        add ({ "African", "Amapiano", "South Africa", "4/4", 4, 16, 1, 112, 56, "Steady kick, shaker 16ths and a lazy rim." },
             { { K, "x... x... x... x..." },
               { R, ".... x..x .... x..." },
               { H, "sxsx sxsx sxsx sxsx" } });

        add ({ "African", "Soukous", "DR Congo", "4/4", 4, 16, 1, 140, 50, "Fast kick pulse with the snare on every offbeat." },
             { { K, "x... x... x... x..." },
               { S, "..x. ..x. ..x. ..x." },
               { H, "xsxs xsxs xsxs xsxs" } });

        add ({ "African", "Gnawa", "Morocco", "12/8", 3, 12, 1, 100, 50, "Krakeb long-short triplets." },
             { { H,  "X.x X.x X.x X.x" },
               { K,  "x.. ... x.. ..." },
               { T3, "... ..x ... ..x" } });

        add ({ "African", "Afro House", "South Africa", "4/4", 4, 16, 1, 122, 50, "Four on the floor with tom percussion and clave." },
             { { K,  "x... x... x... x..." },
               { T2, "...x ..x. ...x .x.." },
               { O,  "..x. ..x. ..x. ..x." },
               { B,  "x..x ..x. ..x. x..." } });

        //==============================================================================
        // Middle Eastern
        add ({ "Middle Eastern", "Maqsum", "Egypt", "4/4", 4, 16, 1, 100, 50, "Dum tek - tek dum - tek -. Dum on the kick, tek on the snare." },
             { { K, "x... .... x... ...." },
               { S, "..x. ..x. .... x..." },
               { H, ".s.s .s.s .s.s .s.s" } });

        add ({ "Middle Eastern", "Baladi", "Egypt", "4/4", 4, 16, 1, 100, 50, "Dum dum - tek dum - tek -." },
             { { K, "x.x. .... x... ...." },
               { S, ".... ..x. .... x..." },
               { H, ".s.s .s.s .s.s .s.s" } });

        add ({ "Middle Eastern", "Saidi", "Upper Egypt", "4/4", 4, 16, 1, 104, 50, "Dum tek - dum dum - tek -." },
             { { K, "x... ..x. x... ...." },
               { S, "..x. .... .... x..." },
               { H, ".s.s .s.s .s.s .s.s" } });

        add ({ "Middle Eastern", "Malfuf", "Egypt / Levant", "2/4", 4, 8, 1, 120, 50, "Fast 3-3-2 rhythm: dum - - tek - - tek -." },
             { { K, "x... ...." },
               { S, "...x ..x." },
               { H, ".s.. .s.s" } });

        add ({ "Middle Eastern", "Ayoub", "Egypt / Sudan", "2/4", 4, 8, 1, 110, 50, "Trance rhythm: dum - ka dum tek - ka -." },
             { { K, "x..x ...." },
               { S, ".... x..." },
               { H, "..s. ..s." } });

        add ({ "Middle Eastern", "Karsilama 9/8", "Turkey", "9/8", 2, 9, 1, 130, 50, "2+2+2+3 with the long group at the end." },
             { { K, "x. .. x. ..." },
               { S, ".. x. .. x.x" },
               { H, "Xx Xx Xx Xxx" } });

        //==============================================================================
        // Balkan
        add ({ "Balkan", "Rachenitsa 7/8", "Bulgaria", "7/8", 2, 7, 1, 160, 50, "Quick-quick-slow: 2+2+3." },
             { { K, "x. .. x.." },
               { S, ".. x. ..x" },
               { H, "Xx Xx Xxx" } });

        add ({ "Balkan", "Lesnoto 7/8", "North Macedonia", "7/8", 2, 7, 1, 120, 50, "Slow-quick-quick: 3+2+2." },
             { { K, "x.. .. .." },
               { S, "... x. x." },
               { H, "Xxx Xx Xx" } });

        add ({ "Balkan", "Kopanitsa 11/8", "Bulgaria", "11/8", 2, 11, 1, 150, 50, "2+2+3+2+2 with the long beat in the middle." },
             { { K, "x. .. x.. .. .." },
               { S, ".. x. ... x. x." },
               { H, "Xx Xx Xxx Xx Xx" } });

        add ({ "Balkan", "Daichovo 9/8", "Bulgaria", "9/8", 2, 9, 1, 150, 50, "2+2+2+3 dance rhythm." },
             { { K, "x. .. .. x.." },
               { S, ".. x. x. ..x" },
               { H, "Xx Xx Xx Xxx" } });

        add ({ "Balkan", "Paidushko 5/8", "Bulgaria / North Macedonia", "5/8", 2, 5, 1, 130, 50, "Limping 2+3." },
             { { K, "x. ..." },
               { S, ".. x.." },
               { H, "Xx Xxx" } });

        //==============================================================================
        // South Asian
        add ({ "South Asian", "Bhangra Chaal", "Punjab", "4/4", 3, 12, 1, 100, 50, "Dhol chaal: bass and treble sides in a skipping triplet figure." },
             { { K, "x.. ..x x.. ..x" },
               { R, "..x x.. ..x x.." },
               { H, "x.x x.x x.x x.x" } });

        add ({ "South Asian", "Kaharwa (8)", "North India", "4/4", 2, 8, 1, 120, 50, "Dha Ge Na Ti Na Ke Dhi Na. Bayan on the kick, dayan on the rim." },
             { { K, "xx .. .. x." },
               { R, "x. xx x. xx" },
               { S, ".. .. .g .." } });

        add ({ "South Asian", "Dadra (6)", "North India", "6/8", 2, 6, 1, 90, 50, "Dha Dhi Na Dha Ti Na." },
             { { K, "xx. x.." },
               { R, "Xxx sxx" } });

        add ({ "South Asian", "Rupak (7)", "North India", "7/8", 2, 7, 1, 100, 50, "Tin Tin Na | Dhi Na | Dhi Na, starting on the open (khali) beat." },
             { { K, "... x. x." },
               { R, "sss Xx Xx" } });

        add ({ "South Asian", "Teental (16)", "North India", "4/4", 2, 8, 2, 100, 50, "Sixteen-beat cycle with the khali (open) section on 9-12." },
             { { K, "xx xx xx xx x. .. .x xx" },
               { R, "Xx xx Xx xx sx xx Xx xx" } });

        //==============================================================================
        // Country & Folk
        add ({ "Country & Folk", "Train Beat", "USA", "4/4", 4, 16, 1, 120, 50, "Sixteenth snare chug with accents on 2 and 4." },
             { { S, "sgsg Xgsg sgsg Xgsg" },
               { K, "x... .... x... ...." } });

        add ({ "Country & Folk", "Two-Beat", "USA", "4/4", 4, 16, 1, 160, 50, "Boom-chick: kick on 1 and 3, cross-stick on 2 and 4." },
             { { K, "x... .... x... ...." },
               { R, ".... x... .... x..." },
               { P, ".... x... .... x..." } });

        add ({ "Country & Folk", "Western Swing", "Texas, USA", "4/4", 3, 12, 1, 140, 50, "Brushy snare shuffle." },
             { { S, "x.x X.x x.x X.x" },
               { K, "x.. ... x.. ..." } });

        add ({ "Country & Folk", "Polka", "Central Europe", "2/4", 4, 8, 1, 120, 50, "Oom-pah: kick on the beat, snare off it." },
             { { K, "x... x..." },
               { S, "..x. ..x." } });

        add ({ "Country & Folk", "Waltz", "Europe", "3/4", 2, 6, 1, 150, 50, "Oom-pah-pah." },
             { { K, "x. .. .." },
               { S, ".. x. x." },
               { H, "x. x. x." } });

        add ({ "Country & Folk", "Irish Jig", "Ireland", "6/8", 2, 6, 1, 165, 50, "Bodhran triplets in two." },
             { { T3, "Xxx Xxx" },
               { K,  "x.. x.." } });

        return lib;
    }
} // namespace

bool parseLane (const char* text, std::vector<float>& out)
{
    out.clear();

    for (const char* c = text; *c != 0; ++c)
    {
        switch (*c)
        {
            case ' ': case '|':  break;
            case '.': case '-':  out.push_back (0.0f);  break;
            case 'g':            out.push_back (0.30f); break;
            case 's':            out.push_back (0.55f); break;
            case 'x':            out.push_back (0.79f); break;
            case 'X':            out.push_back (1.00f); break;
            default:             return false;
        }
    }

    return true;
}

int Groove::numerator() const
{
    // Whole quarters give n/4; otherwise count eighths. A 12/8 groove written
    // as four triplet beats is therefore exported as 4/4, matching its grid.
    const int eighths = stepsPerBar * 2 / stepsPerQuarter;
    return eighths % 2 == 0 ? eighths / 2 : eighths;
}

int Groove::denominator() const
{
    const int eighths = stepsPerBar * 2 / stepsPerQuarter;
    return eighths % 2 == 0 ? 4 : 8;
}

const std::vector<Groove>& GrooveLibrary::all()
{
    static const std::vector<Groove> library = buildLibrary();
    return library;
}

const std::vector<std::string>& GrooveLibrary::categories()
{
    static const std::vector<std::string> cats = []
    {
        std::vector<std::string> result;
        for (const auto& g : all())
            if (std::find (result.begin(), result.end(), g.category) == result.end())
                result.push_back (g.category);
        return result;
    }();
    return cats;
}

int GrooveLibrary::indexOf (const std::string& name)
{
    // Prefer the unique "Category / Name" key; fall back to a bare name.
    const auto& lib = all();
    for (size_t i = 0; i < lib.size(); ++i)
        if (lib[i].key() == name)
            return (int) i;
    for (size_t i = 0; i < lib.size(); ++i)
        if (lib[i].name == name)
            return (int) i;
    return -1;
}

} // namespace groover
