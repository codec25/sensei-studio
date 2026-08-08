#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"
#include "sensei/core/Grid.hpp"
#include "sensei/core/Project.hpp"

using namespace sensei::core;

TEST_CASE("Starter project defaults", "[project]")
{
    const auto project = Project::createStarter();
    REQUIRE(project.id() != kInvalidId);
    REQUIRE(project.tracks().size() == 4);
    REQUIRE(project.primaryMidiTrack() != nullptr);
    REQUIRE(project.primaryClip() != nullptr);
    REQUIRE(project.primaryClip()->notes.empty());
    REQUIRE(project.loop().lengthBeats == Catch::Approx(16.0));
    REQUIRE(project.loop().enabled);
    REQUIRE(project.totalNoteCount() == 0);
}

TEST_CASE("Stable unique IDs", "[project]")
{
    auto project = Project::createStarter();
    const auto a = project.generateId();
    const auto b = project.generateId();
    const auto c = project.generateId();
    REQUIRE(a != b);
    REQUIRE(b != c);
    REQUIRE(a != project.id());
    REQUIRE(project.primaryMidiTrack()->id != project.primaryClip()->id);
}

TEST_CASE("Grid snapping", "[grid]")
{
    REQUIRE(snapBeat(0.12) == Catch::Approx(0.0));
    REQUIRE(snapBeat(0.13) == Catch::Approx(0.25));
    REQUIRE(snapLength(0.1) == Catch::Approx(0.25));
    REQUIRE(snapLength(1.2) == Catch::Approx(1.25));
}

TEST_CASE("Document snapshot contains notes", "[snapshot]")
{
    Document doc;
    auto* track = doc.project().primaryMidiTrack();
    auto* clip = doc.project().primaryClip();
    REQUIRE(track != nullptr);
    REQUIRE(clip != nullptr);

    auto add = std::make_unique<AddNoteCommand>(track->id, clip->id, 60, 0.0, 1.0, 0.8f);
    REQUIRE(doc.execute(std::move(add)));

    const auto guard = doc.snapshots().beginRead();
    const auto& snap = guard.get();
    REQUIRE(snap.noteCount == 1);
    REQUIRE(snap.notes[0].pitch == 60);
    REQUIRE(snap.loopLengthBeats == Catch::Approx(16.0));
    REQUIRE(snap.generation >= 1);
}
