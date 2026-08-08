#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"

using namespace sensei::core;

TEST_CASE("Starter project has multi-track roles", "[project][milestone-c]")
{
    const auto project = Project::createStarter();
    REQUIRE(project.tracks().size() == 4);
    REQUIRE(project.findTrackByRole(TrackRole::Chords) != nullptr);
    REQUIRE(project.findTrackByRole(TrackRole::Bass) != nullptr);
    REQUIRE(project.findTrackByRole(TrackRole::Drums) != nullptr);
    REQUIRE(project.findTrackByRole(TrackRole::Melody) != nullptr);
    REQUIRE(project.findTrackByRole(TrackRole::Drums)->type == TrackType::Drums);
    REQUIRE(project.primaryMidiTrack()->role == TrackRole::Chords);
}
