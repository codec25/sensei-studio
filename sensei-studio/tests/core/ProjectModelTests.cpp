#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/AudioClip.hpp"
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

TEST_CASE("Audio trim is non destructive and tempo aware", "[audio][trim]")
{
    AudioClip clip;
    clip.startBeat = 4.0;
    clip.lengthBeats = 8.0;
    clip.sourceOffsetSeconds = 1.0;
    clip.sourceLengthSeconds = 4.0;

    trimAudioClipStart(clip, 2.0, 120.0); // one second
    REQUIRE(clip.startBeat == Catch::Approx(6.0));
    REQUIRE(clip.lengthBeats == Catch::Approx(6.0));
    REQUIRE(clip.sourceOffsetSeconds == Catch::Approx(2.0));
    REQUIRE(clip.sourceLengthSeconds == Catch::Approx(3.0));

    trimAudioClipEnd(clip, 2.0, 120.0); // another second
    REQUIRE(clip.lengthBeats == Catch::Approx(4.0));
    REQUIRE(clip.sourceLengthSeconds == Catch::Approx(2.0));
}

TEST_CASE("Audio fades stay inside the audible source window", "[audio][fade]")
{
    AudioClip clip;
    clip.sourceLengthSeconds = 1.0;
    clip.fadeIn.lengthSeconds = 0.8;
    clip.fadeOut.lengthSeconds = 0.8;
    sanitizeAudioClip(clip);

    REQUIRE(clip.fadeIn.lengthSeconds == Catch::Approx(0.5));
    REQUIRE(clip.fadeOut.lengthSeconds == Catch::Approx(0.5));
    REQUIRE(fadeGain(0.0, AudioFadeCurve::Linear) == Catch::Approx(0.0));
    REQUIRE(fadeGain(1.0, AudioFadeCurve::Linear) == Catch::Approx(1.0));
}

TEST_CASE("Equal power crossfade keeps centre energy musical", "[audio][crossfade]")
{
    const auto gains = crossfadeGains(0.5, AudioFadeCurve::EqualPower);
    REQUIRE(gains.outgoing == Catch::Approx(0.70710678).margin(0.00001));
    REQUIRE(gains.incoming == Catch::Approx(0.70710678).margin(0.00001));
}

TEST_CASE("Track mix state clamps pan gain and reverb send", "[audio][mix]")
{
    TrackMixState mix;
    mix.gainDb = 40.0;
    mix.pan = -2.0;
    mix.reverbSend01 = 1.5;
    mix.sanitize();

    REQUIRE(mix.gainDb == Catch::Approx(24.0));
    REQUIRE(mix.pan == Catch::Approx(-1.0));
    REQUIRE(mix.reverbSend01 == Catch::Approx(1.0));
}
