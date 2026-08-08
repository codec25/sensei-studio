#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"
#include "sensei/core/bass/BassGenerator.hpp"
#include "sensei/core/drums/DrumPatterns.hpp"
#include "sensei/core/sensei/LessonFlow.hpp"

using namespace sensei::core;

TEST_CASE("Starter drum patterns stay inside 4 bars", "[drums]")
{
    const auto rock = makeBasicRockPattern();
    REQUIRE(rock.stepCount == 64);
    for (const auto& hit : rock.hits)
        REQUIRE(hit.step >= 0);
    REQUIRE(rock.hits.size() > 10);
}

TEST_CASE("Root bass follows chord roots", "[bass]")
{
    const auto chords = generateProgression(0, ScaleMode::Major, "I-V-vi-IV", 16.0);
    const auto bass = generateRootBass(chords.harmony);
    REQUIRE(bass.size() == 4);
    for (std::size_t i = 0; i < bass.size(); ++i)
    {
        REQUIRE((bass[i].pitch % 12) == (chords.harmony.chords[i].pitches[0] % 12));
        REQUIRE(bass[i].endBeat() <= Catch::Approx(16.0));
    }
}

TEST_CASE("Guided apply drums and bass with compound undo", "[lesson][undo]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    REQUIRE(doc.applyStarterDrums("basic-rock"));
    REQUIRE(doc.project().totalDrumHitCount() > 0);
    REQUIRE(doc.applyRootBass());
    REQUIRE(doc.project().findTrackByRole(TrackRole::Bass)->clips.front().notes.size() == 4);
    REQUIRE(isCompleteLoop(doc.project()));

    REQUIRE(doc.undo()); // bass
    REQUIRE(doc.project().findTrackByRole(TrackRole::Bass)->clips.front().notes.empty());
    REQUIRE(doc.undo()); // drums
    REQUIRE(doc.project().totalDrumHitCount() == 0);
}

TEST_CASE("Learning events include first complete loop", "[lesson]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "vi-IV-I-V"));
    REQUIRE(doc.applyStarterDrums("light-pop"));
    REQUIRE(doc.applyRootBass());
    REQUIRE(doc.lesson().celebratedCompleteLoop);
    REQUIRE_FALSE(doc.lesson().events.empty());
    bool found = false;
    for (const auto& e : doc.lesson().events)
        if (e.kind == LearningEventKind::FirstCompleteLoop)
            found = true;
    REQUIRE(found);

    doc.handleChoice(UserChoice::LikeIt);
    REQUIRE(doc.lesson().quiet);
}

TEST_CASE("Snapshot includes multi-track notes and drums", "[snapshot]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-IV-V-I"));
    REQUIRE(doc.applyStarterDrums("four-on-floor"));
    REQUIRE(doc.applyRootBass());
    const auto guard = doc.snapshots().beginRead();
    REQUIRE(guard.get().noteCount >= 16); // 12 chord + 4 bass
    REQUIRE(guard.get().drumHitCount > 0);
}
