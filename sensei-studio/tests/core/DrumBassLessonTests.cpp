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

TEST_CASE("Generated material alone does not emit UserModifiedGenerated", "[lesson][events]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    REQUIRE(doc.applyStarterDrums("basic-rock"));
    REQUIRE(doc.applyRootBass());

    REQUIRE(doc.project().findTrackByRole(TrackRole::Chords)->generatedOrigin);
    REQUIRE(doc.project().findTrackByRole(TrackRole::Drums)->generatedOrigin);
    REQUIRE(doc.project().findTrackByRole(TrackRole::Bass)->generatedOrigin);
    REQUIRE_FALSE(doc.lesson().userModifiedGeneratedEmitted);

    for (const auto& e : doc.lesson().events)
        REQUIRE(e.kind != LearningEventKind::UserModifiedGenerated);
}

TEST_CASE("Manual edit after generation emits UserModifiedGenerated once", "[lesson][events]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    REQUIRE(chords->generatedOrigin);
    REQUIRE_FALSE(chords->clips.front().notes.empty());

    const Id noteId = chords->clips.front().notes.front().id;
    REQUIRE(doc.execute(std::make_unique<MoveNoteCommand>(
        chords->id, chords->clips.front().id, noteId, 1.0, 62)));

    REQUIRE(doc.lesson().userModifiedGeneratedEmitted);
    int userModifiedCount = 0;
    for (const auto& e : doc.lesson().events)
        if (e.kind == LearningEventKind::UserModifiedGenerated)
            ++userModifiedCount;
    REQUIRE(userModifiedCount == 1);
    REQUIRE_FALSE(doc.project().findTrackByRole(TrackRole::Chords)->generatedOrigin);

    // Further edits must not spam the event.
    REQUIRE(doc.execute(std::make_unique<MoveNoteCommand>(
        chords->id, chords->clips.front().id, noteId, 2.0, 64)));
    REQUIRE(doc.execute(std::make_unique<ResizeNoteCommand>(
        chords->id, chords->clips.front().id, noteId, 2.0)));

    userModifiedCount = 0;
    for (const auto& e : doc.lesson().events)
        if (e.kind == LearningEventKind::UserModifiedGenerated)
            ++userModifiedCount;
    REQUIRE(userModifiedCount == 1);
}

TEST_CASE("Editing non-generated track does not count as user-modified-generated", "[lesson][events]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-IV-V-I"));
    auto* melody = doc.project().findTrackByRole(TrackRole::Melody);
    REQUIRE(melody != nullptr);
    REQUIRE_FALSE(melody->generatedOrigin);

    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(
        melody->id, melody->clips.front().id, 72, 0.0, 1.0, 0.8f)));

    REQUIRE_FALSE(doc.lesson().userModifiedGeneratedEmitted);
    for (const auto& e : doc.lesson().events)
        REQUIRE(e.kind != LearningEventKind::UserModifiedGenerated);
    REQUIRE(doc.project().findTrackByRole(TrackRole::Chords)->generatedOrigin);
}

TEST_CASE("Drum step maps from loop length and stepCount", "[drums][snapshot]")
{
    REQUIRE(drumBeatPerStep(16.0, 64) == Catch::Approx(0.25));
    REQUIRE(drumStepToBeat(0, 16.0, 64) == Catch::Approx(0.0));
    REQUIRE(drumStepToBeat(4, 16.0, 64) == Catch::Approx(1.0));
    REQUIRE(drumStepToBeat(16, 16.0, 64) == Catch::Approx(4.0));

    // Different valid resolution: 16 steps across 16 beats → 1 beat per step.
    REQUIRE(drumBeatPerStep(16.0, 16) == Catch::Approx(1.0));
    REQUIRE(drumStepToBeat(3, 16.0, 16) == Catch::Approx(3.0));

    // Different loop length with default 64 steps.
    REQUIRE(drumBeatPerStep(8.0, 64) == Catch::Approx(0.125));
    REQUIRE(drumStepToBeat(8, 8.0, 64) == Catch::Approx(1.0));

    // Invalid/zero stepCount falls back to default 64.
    REQUIRE(drumBeatPerStep(16.0, 0) == Catch::Approx(0.25));
    REQUIRE(drumStepToBeat(4, 16.0, 0) == Catch::Approx(1.0));

    Document doc;
    REQUIRE(doc.applyStarterDrums("four-on-floor"));
    {
        const auto guard = doc.snapshots().beginRead();
        const auto& snap = guard.get();
        REQUIRE(snap.drumHitCount > 0);
        // Default 64-step / 16-beat mapping: first kick at step 0 → beat 0.
        bool foundKickAtZero = false;
        for (std::size_t i = 0; i < snap.drumHitCount; ++i)
        {
            if (snap.drumHits[i].program == DrumProgram::Kick
                && snap.drumHits[i].beat == Catch::Approx(0.0))
                foundKickAtZero = true;
        }
        REQUIRE(foundKickAtZero);
    }

    // Re-map with a coarser pattern (16 steps) and verify snapshot beats.
    auto* drums = doc.project().findTrackByRole(TrackRole::Drums);
    REQUIRE(drums != nullptr);
    DrumPattern coarse;
    coarse.stepCount = 16;
    coarse.hits = { { 0, DrumLane::Kick, 0.8f }, { 4, DrumLane::Snare, 0.8f } };
    REQUIRE(doc.execute(std::make_unique<ReplaceDrumPatternCommand>(drums->id, std::move(coarse))));

    const auto guard = doc.snapshots().beginRead();
    const auto& snap = guard.get();
    REQUIRE(snap.drumHitCount == 2);
    REQUIRE(snap.drumHits[0].beat == Catch::Approx(0.0));
    REQUIRE(snap.drumHits[1].beat == Catch::Approx(4.0));
}
