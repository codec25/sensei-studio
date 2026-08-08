#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"

using namespace sensei::core;

TEST_CASE("Add and delete notes", "[notes]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();

    auto add = std::make_unique<AddNoteCommand>(track->id, clip->id, 64, 1.0, 0.5, 0.7f);
    auto* raw = add.get();
    REQUIRE(doc.execute(std::move(add)));
    const Id id = raw->createdNoteId();
    REQUIRE(doc.project().totalNoteCount() == 1);
    REQUIRE(doc.project().findNote(track->id, clip->id, id) != nullptr);

    REQUIRE(doc.execute(std::make_unique<DeleteNoteCommand>(track->id, clip->id, id)));
    REQUIRE(doc.project().totalNoteCount() == 0);
}

TEST_CASE("Move and resize notes", "[notes]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();

    auto add = std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f);
    auto* raw = add.get();
    REQUIRE(doc.execute(std::move(add)));
    const Id id = raw->createdNoteId();

    REQUIRE(doc.execute(std::make_unique<MoveNoteCommand>(track->id, clip->id, id, 2.1, 67)));
    auto* note = doc.project().findNote(track->id, clip->id, id);
    REQUIRE(note != nullptr);
    REQUIRE(note->startBeat == Catch::Approx(2.0));
    REQUIRE(note->pitch == 67);

    REQUIRE(doc.execute(std::make_unique<ResizeNoteCommand>(track->id, clip->id, id, 2.1)));
    note = doc.project().findNote(track->id, clip->id, id);
    REQUIRE(note->lengthBeats == Catch::Approx(2.0));
}
