#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"

using namespace sensei::core;

TEST_CASE("Undo redo add note", "[undo]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();

    auto add = std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f);
    auto* raw = add.get();
    REQUIRE(doc.execute(std::move(add)));
    const Id id = raw->createdNoteId();
    REQUIRE(doc.project().totalNoteCount() == 1);

    REQUIRE(doc.undo());
    REQUIRE(doc.project().totalNoteCount() == 0);

    REQUIRE(doc.redo());
    REQUIRE(doc.project().findNote(track->id, clip->id, id) != nullptr);
}

TEST_CASE("Undo move and resize", "[undo]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();

    auto add = std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f);
    auto* raw = add.get();
    REQUIRE(doc.execute(std::move(add)));
    const Id id = raw->createdNoteId();

    REQUIRE(doc.execute(std::make_unique<MoveNoteCommand>(track->id, clip->id, id, 4.0, 62)));
    REQUIRE(doc.execute(std::make_unique<ResizeNoteCommand>(track->id, clip->id, id, 2.0)));

    REQUIRE(doc.undo()); // resize
    REQUIRE(doc.project().findNote(track->id, clip->id, id)->lengthBeats == Catch::Approx(1.0));
    REQUIRE(doc.undo()); // move
    auto* note = doc.project().findNote(track->id, clip->id, id);
    REQUIRE(note->startBeat == Catch::Approx(0.0));
    REQUIRE(note->pitch == 60);
}

TEST_CASE("New command clears redo stack", "[undo]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();

    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f)));
    REQUIRE(doc.undo());
    REQUIRE(doc.history().canRedo());

    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(track->id, clip->id, 64, 1.0, 1.0, 0.8f)));
    REQUIRE_FALSE(doc.history().canRedo());
    REQUIRE(doc.project().totalNoteCount() == 1);
}
