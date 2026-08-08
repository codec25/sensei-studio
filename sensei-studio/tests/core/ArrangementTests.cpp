#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "sensei/core/Document.hpp"
#include "sensei/core/Section.hpp"
#include "sensei/core/arrangement/SongShape.hpp"
#include "sensei/core/commands/ArrangementCommands.hpp"

using namespace sensei::core;

namespace {

void buildCompleteLoop(Document& doc)
{
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    REQUIRE(doc.applyStarterDrums("basic-rock"));
    REQUIRE(doc.applyRootBass());
    REQUIRE(isCompleteLoop(doc.project()));
}

} // namespace

TEST_CASE("Sections reject overlaps", "[arrangement][sections]")
{
    Document doc;
    REQUIRE(doc.execute(std::make_unique<CreateSectionCommand>(
        "Intro", SectionLabel::Intro, 0.0, 16.0)));
    REQUIRE_FALSE(doc.execute(std::make_unique<CreateSectionCommand>(
        "Overlap", SectionLabel::Custom, 8.0, 16.0)));
    REQUIRE(doc.project().sections().size() == 1);

    // Touching endpoints are allowed.
    REQUIRE(doc.execute(std::make_unique<CreateSectionCommand>(
        "Main", SectionLabel::Chorus, 16.0, 16.0)));
    REQUIRE(doc.project().sections().size() == 2);

    const Id mainId = doc.project().sections().back().id;
    REQUIRE_FALSE(doc.execute(std::make_unique<ResizeSectionCommand>(mainId, 8.0, 16.0)));
    REQUIRE(doc.project().findSection(mainId)->startBeat == Catch::Approx(16.0));
}

TEST_CASE("Song length derives from section ends", "[arrangement]")
{
    Document doc;
    REQUIRE(doc.project().songLengthBeats() == Catch::Approx(kDefaultLoopBeats));
    buildCompleteLoop(doc);
    REQUIRE(doc.applySongShape());

    const SongShapePlan plan;
    REQUIRE(doc.project().sections().size() == 4);
    REQUIRE(doc.project().songLengthBeats() == Catch::Approx(plan.songLengthBeats()));
    REQUIRE(hasFullSongStructure(doc.project()));
}

TEST_CASE("ApplySongShape repeats 4-bar seed without stretching", "[arrangement]")
{
    Document doc;
    buildCompleteLoop(doc);
    REQUIRE(doc.applySongShape());

    const auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    // Intro seed + Main(2) + Variation(2) + Outro(1) = 6 clips of 4 bars.
    REQUIRE(chords->clips.size() == 6);
    for (const auto& clip : chords->clips)
        REQUIRE(clip.lengthBeats == Catch::Approx(kDefaultLoopBeats));

    const auto* drums = doc.project().findTrackByRole(TrackRole::Drums);
    REQUIRE(drums != nullptr);
    REQUIRE(drums->drumClips.size() == 6);
    for (const auto& clip : drums->drumClips)
        REQUIRE(clip.lengthBeats == Catch::Approx(kDefaultLoopBeats));
}

TEST_CASE("Duplicate move delete clips with undo", "[arrangement][undo]")
{
    Document doc;
    buildCompleteLoop(doc);
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    const Id seedId = chords->clips.front().id;
    const std::size_t before = chords->clips.size();

    REQUIRE(doc.execute(std::make_unique<DuplicateClipCommand>(chords->id, seedId, 16.0)));
    REQUIRE(chords->clips.size() == before + 1);
    Id copyId = kInvalidId;
    for (const auto& c : chords->clips)
        if (std::abs(c.startBeat - 16.0) < 1.0e-6)
            copyId = c.id;
    REQUIRE(copyId != kInvalidId);

    REQUIRE(doc.execute(std::make_unique<MoveClipCommand>(chords->id, copyId, 20.0)));
    REQUIRE(doc.project().findClip(chords->id, copyId)->startBeat == Catch::Approx(20.0));

    REQUIRE(doc.execute(std::make_unique<DeleteClipCommand>(chords->id, copyId)));
    REQUIRE(doc.project().findClip(chords->id, copyId) == nullptr);

    REQUIRE(doc.undo()); // delete
    REQUIRE(doc.project().findClip(chords->id, copyId) != nullptr);
    REQUIRE(doc.undo()); // move
    REQUIRE(doc.project().findClip(chords->id, copyId)->startBeat == Catch::Approx(16.0));
    REQUIRE(doc.undo()); // duplicate
    REQUIRE(doc.project().findClip(chords->id, copyId) == nullptr);
}

TEST_CASE("Non-destructive clip resize hides then reveals notes", "[arrangement][snapshot]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-IV-V-I"));
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    auto* clip = &chords->clips.front();
    const auto originalNoteCount = clip->notes.size();
    REQUIRE(originalNoteCount >= 4);

    {
        const auto guard = doc.snapshots().beginRead();
        REQUIRE(guard.get().noteCount == originalNoteCount);
    }

    REQUIRE(doc.execute(std::make_unique<ResizeClipCommand>(chords->id, clip->id, 4.0)));
    REQUIRE(clip->notes.size() == originalNoteCount); // data preserved
    {
        const auto guard = doc.snapshots().beginRead();
        REQUIRE(guard.get().noteCount < originalNoteCount);
        REQUIRE(guard.get().noteCount > 0);
    }

    REQUIRE(doc.execute(std::make_unique<ResizeClipCommand>(chords->id, clip->id, 16.0)));
    {
        const auto guard = doc.snapshots().beginRead();
        REQUIRE(guard.get().noteCount == originalNoteCount);
    }
}

TEST_CASE("Overlapping clips both schedule", "[arrangement][snapshot]")
{
    Document doc;
    REQUIRE(doc.applyProgression(0, ScaleMode::Major, "I-V-vi-IV"));
    auto* chords = doc.project().findTrackByRole(TrackRole::Chords);
    REQUIRE(chords != nullptr);
    const Id seed = chords->clips.front().id;
    const auto baseCount = doc.snapshots().beginRead().get().noteCount;

    REQUIRE(doc.execute(std::make_unique<DuplicateClipCommand>(chords->id, seed, 0.0)));
    // Two clips at same start — both emit.
    const auto guard = doc.snapshots().beginRead();
    REQUIRE(guard.get().noteCount == baseCount * 2);
}

TEST_CASE("Arrangement snapshot maps drum clips onto timeline", "[arrangement][snapshot]")
{
    Document doc;
    buildCompleteLoop(doc);
    REQUIRE(doc.applySongShape());

    const auto guard = doc.snapshots().beginRead();
    const auto& snap = guard.get();
    REQUIRE(snap.songLengthBeats == Catch::Approx(SongShapePlan {}.songLengthBeats()));
    REQUIRE(snap.noteCount > 12);
    REQUIRE(snap.drumHitCount > 10);

    bool foundLateHit = false;
    for (std::uint32_t i = 0; i < snap.drumHitCount; ++i)
        if (snap.drumHits[i].beat >= 16.0)
            foundLateHit = true;
    REQUIRE(foundLateHit);
}

TEST_CASE("Song shape compound undo restores loop project", "[arrangement][undo]")
{
    Document doc;
    buildCompleteLoop(doc);
    const auto notesBefore = doc.project().totalNoteCount();
    const auto drumsBefore = doc.project().totalDrumHitCount();

    REQUIRE(doc.applySongShape());
    REQUIRE(doc.project().sections().size() == 4);
    REQUIRE(doc.project().totalNoteCount() > notesBefore);

    REQUIRE(doc.undo());
    REQUIRE(doc.project().sections().empty());
    REQUIRE(doc.project().totalNoteCount() == notesBefore);
    REQUIRE(doc.project().totalDrumHitCount() == drumsBefore);
}

TEST_CASE("Variation helper thins hats and emits events", "[arrangement][sensei]")
{
    Document doc;
    buildCompleteLoop(doc);
    REQUIRE(doc.applySongShape());
    REQUIRE(doc.applyVariationThinDrums());

    bool foundVariation = false;
    bool foundContrast = false;
    bool foundSong = false;
    for (const auto& e : doc.lesson().events)
    {
        if (e.kind == LearningEventKind::FirstVariationCreated)
            foundVariation = true;
        if (e.kind == LearningEventKind::ContrastIntroduced)
            foundContrast = true;
        if (e.kind == LearningEventKind::FirstFullSongStructureCreated)
            foundSong = true;
    }
    REQUIRE(foundVariation);
    REQUIRE(foundContrast);
    REQUIRE(foundSong);

    auto* drums = doc.project().findTrackByRole(TrackRole::Drums);
    REQUIRE(drums != nullptr);
    bool anyThinned = false;
    for (const auto& clip : drums->drumClips)
    {
        if (clip.startBeat >= 48.0)
        {
            for (const auto& hit : clip.pattern.hits)
                REQUIRE(hit.lane != DrumLane::ClosedHat);
            anyThinned = true;
        }
    }
    REQUIRE(anyThinned);
}

TEST_CASE("Intro contrast removes drums and bass from intro", "[arrangement]")
{
    Document doc;
    buildCompleteLoop(doc);
    REQUIRE(doc.applySongShape());
    REQUIRE(doc.applyIntroContrast());

    auto* drums = doc.project().findTrackByRole(TrackRole::Drums);
    auto* bass = doc.project().findTrackByRole(TrackRole::Bass);
    REQUIRE(drums != nullptr);
    REQUIRE(bass != nullptr);
    for (const auto& clip : drums->drumClips)
        REQUIRE(clip.startBeat >= 16.0 - 1.0e-6);
    for (const auto& clip : bass->clips)
        REQUIRE(clip.startBeat >= 16.0 - 1.0e-6);
}

TEST_CASE("thinDrumPattern helper removes one lane", "[arrangement][helpers]")
{
    const auto rock = makeBasicRockPattern();
    const auto thinned = thinDrumPattern(rock, DrumLane::ClosedHat);
    REQUIRE(thinned.hits.size() < rock.hits.size());
    for (const auto& hit : thinned.hits)
        REQUIRE(hit.lane != DrumLane::ClosedHat);
}

TEST_CASE("Whole-song transport stops at song length", "[arrangement][transport]")
{
    Document doc;
    buildCompleteLoop(doc);
    REQUIRE(doc.applySongShape());
    REQUIRE_FALSE(doc.project().loop().enabled);

    auto& transport = doc.transport();
    REQUIRE_FALSE(transport.loopEnabled());
    transport.setSongLengthBeats(doc.project().songLengthBeats());
    transport.play();
    // 94 BPM × 90s ≈ 141 beats > 96-beat song.
    transport.advance(90.0);
    REQUIRE_FALSE(transport.isPlaying());
    REQUIRE(transport.positionBeats() == Catch::Approx(doc.project().songLengthBeats()));
}
