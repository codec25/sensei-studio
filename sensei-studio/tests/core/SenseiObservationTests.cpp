#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"
#include "sensei/core/sensei/ProjectAnalyzer.hpp"

using namespace sensei::core;

TEST_CASE("No notes observation", "[sensei]")
{
    Document doc;
    const auto obs = doc.analyze();
    REQUIRE(obs.kind == ObservationKind::NoNotes);
}

TEST_CASE("First idea observation", "[sensei]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 64, 1.0, 1.0, 0.8f)));
    REQUIRE(doc.analyze().kind == ObservationKind::FirstIdea);
}

TEST_CASE("Chord observation", "[sensei]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();
    // More than 3 notes so FirstIdea does not win; overlapping C-E-G chord.
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 2.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 64, 0.0, 2.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 67, 0.0, 2.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 4.0, 1.0, 0.8f)));
    REQUIRE(doc.analyze().kind == ObservationKind::ChordDetected);
}

TEST_CASE("Notes outside loop observation", "[sensei]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 62, 1.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 64, 2.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 65, 18.0, 1.0, 0.8f)));
    REQUIRE(doc.analyze().kind == ObservationKind::NotesOutsideLoop);
}

TEST_CASE("Low pitch variety observation", "[sensei]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();
    for (int i = 0; i < 6; ++i)
    {
        const int pitch = (i % 2 == 0) ? 60 : 62;
        REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, pitch, i * 1.0, 0.5, 0.8f)));
    }
    REQUIRE(doc.analyze().kind == ObservationKind::LowPitchVariety);
}
