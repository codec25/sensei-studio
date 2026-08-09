#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"
#include "sensei/core/commands/NoteCommands.hpp"
#include "sensei/core/commands/TrackMuteCommands.hpp"

using namespace sensei::core;

TEST_CASE("isTrackAudible respects mute and solo rules", "[mute][solo]")
{
    Track a;
    a.muted = false;
    a.solo = false;
    Track b = a;
    b.muted = true;
    Track c = a;
    c.solo = true;

    REQUIRE(isTrackAudible(a, false));
    REQUIRE_FALSE(isTrackAudible(b, false));
    REQUIRE(isTrackAudible(c, false));

    REQUIRE_FALSE(isTrackAudible(a, true));
    REQUIRE_FALSE(isTrackAudible(b, true));
    REQUIRE(isTrackAudible(c, true));

    c.muted = true;
    REQUIRE_FALSE(isTrackAudible(c, true));
}

TEST_CASE("Mute removes track notes from snapshot", "[mute][snapshot]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);

    {
        const auto guard = doc.snapshots().beginRead();
        REQUIRE(guard.get().noteCount > 0);
    }

    REQUIRE(doc.execute(std::make_unique<SetTrackMuteCommand>(chords->id, true)));
    REQUIRE(chords->muted);

    {
        const auto guard = doc.snapshots().beginRead();
        for (std::uint32_t i = 0; i < guard.get().noteCount; ++i)
            REQUIRE(guard.get().notes[i].instrumentId != InstrumentId::WarmKeys);
    }
}

TEST_CASE("Solo gates other tracks in snapshot", "[solo][snapshot]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    REQUIRE(doc.applyRootBass());

    auto* bass = doc.project().findTrackByRole(TrackRole::Bass);
    REQUIRE(bass != nullptr);
    REQUIRE(doc.execute(std::make_unique<SetTrackSoloCommand>(bass->id, true)));
    REQUIRE(bass->solo);

    const auto guard = doc.snapshots().beginRead();
    REQUIRE(guard.get().noteCount > 0);
    for (std::uint32_t i = 0; i < guard.get().noteCount; ++i)
        REQUIRE(guard.get().notes[i].instrumentId == InstrumentId::DeepBass);
}

TEST_CASE("Mute and solo undo redo", "[mute][solo][undo]")
{
    Document doc;
    auto* melody = doc.project().findTrackByRole(TrackRole::Melody);
    REQUIRE(melody != nullptr);

    REQUIRE(doc.execute(std::make_unique<SetTrackMuteCommand>(melody->id, true)));
    REQUIRE(melody->muted);
    REQUIRE(doc.undo());
    REQUIRE_FALSE(melody->muted);
    REQUIRE(doc.redo());
    REQUIRE(melody->muted);

    REQUIRE(doc.execute(std::make_unique<SetTrackSoloCommand>(melody->id, true)));
    REQUIRE(melody->solo);
    REQUIRE(doc.undo());
    REQUIRE_FALSE(melody->solo);
}

TEST_CASE("Mute does not mutate MIDI notes", "[mute]")
{
    Document doc;
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    auto* clip = &chords->clips.front();
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(chords->id, clip->id, 60, 0.0, 1.0, 0.8f)));
    const auto before = clip->notes.size();
    const int pitch = clip->notes.front().pitch;

    REQUIRE(doc.execute(std::make_unique<SetTrackMuteCommand>(chords->id, true)));
    REQUIRE(clip->notes.size() == before);
    REQUIRE(clip->notes.front().pitch == pitch);
}
