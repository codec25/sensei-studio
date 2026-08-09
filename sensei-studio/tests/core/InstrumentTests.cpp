#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Document.hpp"
#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/commands/InstrumentCommands.hpp"
#include "sensei/core/commands/NoteCommands.hpp"
#include "sensei/engine/InstrumentRack.hpp"

using namespace sensei::core;
using namespace sensei::engine;

TEST_CASE("Starter tracks default to role instruments", "[instrument]")
{
    const auto project = Project::createStarter();
    REQUIRE(project.findTrackByRole(TrackRole::Chords)->instrumentId == InstrumentId::WarmKeys);
    REQUIRE(project.findTrackByRole(TrackRole::Bass)->instrumentId == InstrumentId::DeepBass);
    REQUIRE(project.findTrackByRole(TrackRole::Melody)->instrumentId == InstrumentId::BrightPluck);
    REQUIRE(project.findTrackByRole(TrackRole::Drums)->instrumentId == InstrumentId::StudioKitBasic);

    REQUIRE(std::string(instrumentInfo(InstrumentId::WarmKeys).stableId) == "chords.warm_keys");
    REQUIRE(std::string(instrumentInfo(InstrumentId::DeepBass).stableId) == "bass.deep_bass");
    REQUIRE(std::string(instrumentInfo(InstrumentId::BrightPluck).stableId) == "melody.bright_pluck");
    REQUIRE(std::string(instrumentInfo(InstrumentId::StudioKitBasic).stableId) == "drums.studio_kit_basic");
}

TEST_CASE("Changing instrument does not mutate MIDI", "[instrument]")
{
    Document doc;
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    auto* clip = &chords->clips.front();
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(chords->id, clip->id, 60, 0.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(chords->id, clip->id, 64, 1.0, 1.0, 0.8f)));
    const auto notesBefore = clip->notes;
    const auto noteCount = clip->notes.size();

    REQUIRE(doc.execute(std::make_unique<SetTrackInstrumentCommand>(chords->id, InstrumentId::BrightPluck)));
    REQUIRE(chords->instrumentId == InstrumentId::BrightPluck);
    REQUIRE(clip->notes.size() == noteCount);
    REQUIRE(clip->notes[0].pitch == notesBefore[0].pitch);
    REQUIRE(clip->notes[0].startBeat == Catch::Approx(notesBefore[0].startBeat));
    REQUIRE(clip->notes[1].pitch == notesBefore[1].pitch);
}

TEST_CASE("SetTrackInstrumentCommand undo redo", "[instrument][undo]")
{
    Document doc;
    auto* bass = doc.project().findTrackByRole(TrackRole::Bass);
    REQUIRE(bass != nullptr);
    REQUIRE(bass->instrumentId == InstrumentId::DeepBass);

    REQUIRE(doc.execute(std::make_unique<SetTrackInstrumentCommand>(bass->id, InstrumentId::WarmKeys)));
    REQUIRE(bass->instrumentId == InstrumentId::WarmKeys);
    REQUIRE(doc.undo());
    REQUIRE(bass->instrumentId == InstrumentId::DeepBass);
    REQUIRE(doc.redo());
    REQUIRE(bass->instrumentId == InstrumentId::WarmKeys);
}

TEST_CASE("Snapshot stamps track instrument IDs", "[instrument][snapshot]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    REQUIRE(doc.execute(std::make_unique<SetTrackInstrumentCommand>(chords->id, InstrumentId::BrightPluck)));

    const auto guard = doc.snapshots().beginRead();
    REQUIRE(guard.get().noteCount > 0);
    for (std::uint32_t i = 0; i < guard.get().noteCount; ++i)
    {
        REQUIRE(guard.get().notes[i].instrumentId == InstrumentId::BrightPluck);
        REQUIRE(guard.get().notes[i].program == SoundProgram::Melody);
    }
}

TEST_CASE("Drum snapshot carries kit identity and pad mapping", "[instrument][drums]")
{
    Document doc;
    REQUIRE(doc.applyStarterDrums("basic-rock"));
    const auto guard = doc.snapshots().beginRead();
    REQUIRE(guard.get().drumHitCount > 0);
    bool kick = false, snare = false, hat = false;
    for (std::uint32_t i = 0; i < guard.get().drumHitCount; ++i)
    {
        REQUIRE(guard.get().drumHits[i].instrumentId == InstrumentId::StudioKitBasic);
        if (guard.get().drumHits[i].program == DrumProgram::Kick) kick = true;
        if (guard.get().drumHits[i].program == DrumProgram::Snare) snare = true;
        if (guard.get().drumHits[i].program == DrumProgram::ClosedHat) hat = true;
    }
    REQUIRE(kick);
    REQUIRE(snare);
    REQUIRE(hat);
}

TEST_CASE("Rack routes instrument IDs to distinct engines", "[instrument][engine]")
{
    InstrumentRack rack;
    rack.prepare(44100.0, 64);
    REQUIRE(rack.pitched(InstrumentId::WarmKeys)->id() == InstrumentId::WarmKeys);
    REQUIRE(rack.pitched(InstrumentId::DeepBass)->id() == InstrumentId::DeepBass);
    REQUIRE(rack.pitched(InstrumentId::BrightPluck)->id() == InstrumentId::BrightPluck);
    REQUIRE(rack.pitched(InstrumentId::StudioKitBasic) == nullptr);
    REQUIRE(rack.drums().id() == InstrumentId::StudioKitBasic);

    // Audition path: notes go to selected instrument engines without throwing / allocating.
    rack.noteOn(InstrumentId::WarmKeys, 60, 0.8f);
    rack.noteOn(InstrumentId::DeepBass, 36, 0.8f);
    rack.noteOn(InstrumentId::BrightPluck, 72, 0.8f);
    rack.triggerDrum(InstrumentId::StudioKitBasic, DrumProgram::Kick, 0.9f);

    std::array<float, 64> left {};
    std::array<float, 64> right {};
    rack.process(left.data(), right.data(), 64);

    float energy = 0.0f;
    for (float s : left)
        energy += s * s;
    REQUIRE(energy > 0.0f);

    rack.allNotesOff();
    // After allNotesOff, a short process should decay toward silence (no stuck full-amp notes).
    left.fill(0.0f);
    right.fill(0.0f);
    for (int n = 0; n < 20; ++n)
        rack.process(left.data(), right.data(), 64);
    float after = 0.0f;
    for (float s : left)
        after += std::fabs(s);
    REQUIRE(after < 0.05f);
}

TEST_CASE("Reject drum kit on MIDI track and pitched on drums", "[instrument]")
{
    Document doc;
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    auto* drums = doc.project().findTrackByRole(TrackRole::Drums);
    REQUIRE_FALSE(doc.execute(std::make_unique<SetTrackInstrumentCommand>(
        chords->id, InstrumentId::StudioKitBasic)));
    REQUIRE_FALSE(doc.execute(std::make_unique<SetTrackInstrumentCommand>(
        drums->id, InstrumentId::WarmKeys)));
    REQUIRE(chords->instrumentId == InstrumentId::WarmKeys);
    REQUIRE(drums->instrumentId == InstrumentId::StudioKitBasic);
}

TEST_CASE("Sensei surfaces instrument identity tip", "[instrument][sensei]")
{
    Document doc;
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    doc.setSelectedTrackId(chords->id);
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(
        chords->id, chords->clips.front().id, 60, 0.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(
        chords->id, chords->clips.front().id, 64, 0.5, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(
        chords->id, chords->clips.front().id, 67, 1.0, 1.0, 0.8f)));
    REQUIRE(doc.execute(std::make_unique<AddNoteCommand>(
        chords->id, chords->clips.front().id, 60, 2.0, 1.0, 0.8f)));

    const auto obs = doc.analyze();
    REQUIRE(obs.kind == ObservationKind::InstrumentIdentity);
    REQUIRE(obs.title.find("Warm Keys") != std::string::npos);
}
