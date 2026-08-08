#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"
#include "sensei/core/harmony/Progressions.hpp"

using namespace sensei::core;

TEST_CASE("Progression generation fits 4-bar loop", "[chords]")
{
    const auto material = generateProgression(0, ScaleMode::Major, "I-V-vi-IV", 16.0);
    REQUIRE(material.harmony.chords.size() == 4);
    REQUIRE(material.notes.size() == 12);
    for (const auto& chord : material.harmony.chords)
    {
        REQUIRE(chord.startBeat >= 0.0);
        REQUIRE(chord.endBeat() <= Catch::Approx(16.0));
    }
    REQUIRE(material.harmony.chords[0].roman == "I");
    REQUIRE(material.harmony.chords[0].chordName == "C");
}

TEST_CASE("Key transposition changes chord names", "[chords]")
{
    const auto inC = generateProgression(0, ScaleMode::Major, "I-IV-V-I", 16.0);
    const auto inG = generateProgression(7, ScaleMode::Major, "I-IV-V-I", 16.0);
    REQUIRE(inC.harmony.chords[0].chordName == "C");
    REQUIRE(inG.harmony.chords[0].chordName == "G");
    REQUIRE(inC.notes[0].pitch != inG.notes[0].pitch);
}

TEST_CASE("Apply progression via document compound command", "[chords][undo]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    REQUIRE(doc.project().findTrackByRole(TrackRole::Chords)->clips.front().notes.size() == 12);
    REQUIRE(doc.project().harmony().chords.size() == 4);
    REQUIRE(doc.undo());
    REQUIRE(doc.project().findTrackByRole(TrackRole::Chords)->clips.front().notes.empty());
    REQUIRE(doc.project().harmony().chords.empty());
}
