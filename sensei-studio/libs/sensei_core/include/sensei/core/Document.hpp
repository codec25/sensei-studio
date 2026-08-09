#pragma once

#include "sensei/core/Project.hpp"
#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/core/arrangement/SongShape.hpp"
#include "sensei/core/bass/BassGenerator.hpp"
#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/commands/ArrangementCommands.hpp"
#include "sensei/core/commands/CommandHistory.hpp"
#include "sensei/core/commands/CompoundCommand.hpp"
#include "sensei/core/commands/InstrumentCommands.hpp"
#include "sensei/core/commands/NoteCommands.hpp"
#include "sensei/core/commands/TrackContentCommands.hpp"
#include "sensei/core/commands/TrackMuteCommands.hpp"
#include "sensei/core/drums/DrumPatterns.hpp"
#include "sensei/core/harmony/Progressions.hpp"
#include "sensei/core/sensei/LessonFlow.hpp"
#include "sensei/core/sensei/ProjectAnalyzer.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sensei::core {

class Document
{
public:
    Document()
        : project_(Project::createStarter())
    {
        transport_.setBpm(kDefaultBpm);
        if (auto* chords = project_.findTrackByRole(TrackRole::Chords))
        {
            selectedTrackId_ = chords->id;
            if (! chords->clips.empty())
                selectedClipId_ = chords->clips.front().id;
        }
        syncTransportFromProject();
        publishSnapshot();
    }

    [[nodiscard]] Project& project() noexcept { return project_; }
    [[nodiscard]] const Project& project() const noexcept { return project_; }
    [[nodiscard]] Transport& transport() noexcept { return transport_; }
    [[nodiscard]] const Transport& transport() const noexcept { return transport_; }
    [[nodiscard]] CommandHistory& history() noexcept { return history_; }
    [[nodiscard]] const CommandHistory& history() const noexcept { return history_; }
    [[nodiscard]] const SnapshotPublisher& snapshots() const noexcept { return snapshots_; }
    [[nodiscard]] SnapshotPublisher& snapshots() noexcept { return snapshots_; }
    [[nodiscard]] LessonState& lesson() noexcept { return lesson_; }
    [[nodiscard]] const LessonState& lesson() const noexcept { return lesson_; }

    [[nodiscard]] Id selectedNoteId() const noexcept { return selectedNoteId_; }
    void setSelectedNoteId(Id id) noexcept { selectedNoteId_ = id; }
    [[nodiscard]] Id selectedTrackId() const noexcept { return selectedTrackId_; }
    [[nodiscard]] Id selectedClipId() const noexcept { return selectedClipId_; }

    void setSelectedTrackId(Id id) noexcept
    {
        selectedTrackId_ = id;
        ensureSelectedClipForTrack();
    }

    void setSelectedClipId(Id trackId, Id clipId) noexcept
    {
        selectedTrackId_ = trackId;
        selectedClipId_ = clipId;
    }

    void syncTransportFromProject() noexcept
    {
        const auto& loop = project_.loop();
        transport_.setLoop(loop.startBeat, loop.lengthBeats, loop.enabled);
        transport_.setSongLengthBeats(project_.songLengthBeats());
    }

    void syncTransportLoopFromProject() noexcept { syncTransportFromProject(); }

    void setBpm(double bpm) noexcept
    {
        transport_.setBpm(bpm);
        publishSnapshot();
    }

    bool execute(std::unique_ptr<Command> command)
    {
        return executeInternal(std::move(command), false);
    }

    bool undo()
    {
        if (! history_.undo(project_))
            return false;
        ensureSelectedClipForTrack();
        syncTransportFromProject();
        publishSnapshot();
        return true;
    }

    bool redo()
    {
        if (! history_.redo(project_))
            return false;
        ensureSelectedClipForTrack();
        syncTransportFromProject();
        publishSnapshot();
        return true;
    }

    bool applyProgression(int rootPc, ScaleMode mode, const char* progressionId)
    {
        auto* track = project_.findTrackByRole(TrackRole::Chords);
        if (track == nullptr || track->clips.empty())
            return false;

        auto* clip = resolveMidiClip(*track);
        if (clip == nullptr)
            return false;

        const double span = clip->lengthBeats > 0.0 ? clip->lengthBeats : project_.loop().lengthBeats;
        auto material = generateProgression(rootPc, mode, progressionId, span);
        for (auto& n : material.notes)
            n.id = kInvalidId;

        auto compound = std::make_unique<CompoundCommand>("Add chord progression");
        compound->add(std::make_unique<SetHarmonyCommand>(material.harmony));
        compound->add(std::make_unique<ReplaceClipNotesCommand>(
            track->id, clip->id, std::move(material.notes)));

        if (! executeInternal(std::move(compound), true))
            return false;

        track->generatedOrigin = true;
        selectedTrackId_ = track->id;
        selectedClipId_ = clip->id;
        pushEvent(LearningEventKind::ChordProgressionCreated);
        lesson_.chordsAccepted = true;
        lesson_.step = LessonStep::OfferDrums;
        lesson_.quiet = false;
        return true;
    }

    bool applyStarterDrums(const char* patternName = "basic-rock")
    {
        auto* track = project_.findTrackByRole(TrackRole::Drums);
        if (track == nullptr || track->drumClips.empty())
            return false;

        auto* clip = resolveDrumClip(*track);
        if (clip == nullptr)
            return false;

        DrumPattern pattern = makeBasicRockPattern();
        if (std::string(patternName) == "light-pop")
            pattern = makeLightPopPattern();
        else if (std::string(patternName) == "four-on-floor")
            pattern = makeFourOnFloorPattern();
        pattern.id = kInvalidId;

        auto compound = std::make_unique<CompoundCommand>("Add starter drums");
        compound->add(std::make_unique<ReplaceDrumPatternCommand>(track->id, clip->id, std::move(pattern)));
        if (! executeInternal(std::move(compound), true))
            return false;

        track->generatedOrigin = true;
        selectedTrackId_ = track->id;
        selectedClipId_ = clip->id;
        pushEvent(LearningEventKind::DrumPatternCreated);
        lesson_.drumsAccepted = true;
        lesson_.step = LessonStep::OfferBass;
        return true;
    }

    bool applyRootBass()
    {
        auto* track = project_.findTrackByRole(TrackRole::Bass);
        if (track == nullptr || track->clips.empty())
            return false;
        if (project_.harmony().chords.empty())
            return false;

        auto* clip = resolveMidiClip(*track);
        if (clip == nullptr)
            return false;

        auto notes = generateRootBass(project_.harmony());
        for (auto& n : notes)
            n.id = kInvalidId;

        auto compound = std::make_unique<CompoundCommand>("Add root-note bass");
        compound->add(std::make_unique<ReplaceClipNotesCommand>(
            track->id, clip->id, std::move(notes)));
        if (! executeInternal(std::move(compound), true))
            return false;

        track->generatedOrigin = true;
        selectedTrackId_ = track->id;
        selectedClipId_ = clip->id;
        pushEvent(LearningEventKind::RootBassAdded);
        lesson_.bassAccepted = true;
        maybeCelebrateCompleteLoop();
        return true;
    }

    bool applySongShape()
    {
        if (! isCompleteLoop(project_))
            return false;

        auto compound = makeApplySongShapeCommand(project_);
        if (! executeInternal(std::move(compound), true))
            return false;

        project_.deriveAndSetSongLength(SongShapePlan {}.songLengthBeats());
        syncTransportFromProject();

        pushEvent(LearningEventKind::FirstArrangementCreated);
        pushEvent(LearningEventKind::IntroCreated);
        pushEvent(LearningEventKind::LoopDuplicated);
        pushEvent(LearningEventKind::FirstFullSongStructureCreated);

        lesson_.songShapeAccepted = true;
        lesson_.celebratedSong = true;
        lesson_.step = LessonStep::OfferVariation;
        lesson_.quiet = false;
        publishSnapshot();
        return true;
    }

    bool applyIntroContrast()
    {
        auto compound = makeIntroMuteDrumsAndBassCommand(project_);
        if (! executeInternal(std::move(compound), true))
            return false;
        pushEvent(LearningEventKind::ContrastIntroduced);
        return true;
    }

    bool applyVariationThinDrums()
    {
        auto compound = makeThinVariationDrumsCommand(project_);
        if (! executeInternal(std::move(compound), true))
            return false;
        pushEvent(LearningEventKind::FirstVariationCreated);
        pushEvent(LearningEventKind::ContrastIntroduced);
        lesson_.variationAccepted = true;
        lesson_.step = LessonStep::CelebrateSong;
        return true;
    }

    void handleChoice(UserChoice choice)
    {
        switch (choice)
        {
            case UserChoice::LikeIt:
                lesson_.quiet = true;
                lesson_.step = LessonStep::Quiet;
                lesson_.lastWhy = "Keeping your creative choice. Sensei will stay quiet.";
                break;
            case UserChoice::Later:
                lesson_.quiet = true;
                lesson_.lastWhy = "Okay — continue creating. Ask Sensei when you want the next step.";
                break;
            case UserChoice::Why:
                if (! lesson_.events.empty())
                    lesson_.lastWhy = lesson_.events.back().fact + " " + lesson_.events.back().advice;
                else
                    lesson_.lastWhy = "Sensei observes measurable facts in your project before offering advice.";
                break;
            case UserChoice::DoSomething:
                lesson_.quiet = false;
                if (lesson_.step == LessonStep::Quiet || lesson_.step == LessonStep::CelebrateLoop
                    || lesson_.step == LessonStep::CelebrateSong)
                {
                    if (! lesson_.chordsAccepted)
                        lesson_.step = LessonStep::ChooseKey;
                    else if (! lesson_.drumsAccepted)
                        lesson_.step = LessonStep::OfferDrums;
                    else if (! lesson_.bassAccepted)
                        lesson_.step = LessonStep::OfferBass;
                    else if (! lesson_.songShapeAccepted)
                        lesson_.step = LessonStep::OfferSongShape;
                    else if (! lesson_.variationAccepted)
                        lesson_.step = LessonStep::OfferVariation;
                    else
                        lesson_.step = LessonStep::CelebrateSong;
                }
                break;
        }
    }

    [[nodiscard]] Observation analyze() const
    {
        if (! lesson_.quiet && ! lesson_.events.empty())
            return observationFromLesson(lesson_, project_);
        if (hasFullSongStructure(project_))
            return observationFromLesson(lesson_, project_);
        if (isCompleteLoop(project_))
            return observationFromLesson(lesson_, project_);

        auto obs = ProjectAnalyzer::analyze(project_);
        if (const auto* track = project_.findTrack(selectedTrackId_))
        {
            const auto info = instrumentInfo(track->instrumentId);
            // Surface instrument identity when the loop already has established material.
            if (obs.kind == ObservationKind::LoopHasMaterial)
            {
                Observation tip;
                tip.kind = ObservationKind::InstrumentIdentity;
                tip.title = std::string(info.displayName);
                tip.fact = std::string(track->name) + " uses " + info.displayName + ".";
                tip.advice = info.shortFact;
                return tip;
            }
            if (obs.advice.empty())
                obs.advice = info.shortFact;
        }
        return obs;
    }

    void publishSnapshot()
    {
        auto& slot = snapshots_.beginWrite();
        slot = {};
        slot.generation = ++generation_;
        slot.bpm = transport_.bpm();
        slot.loopStartBeats = project_.loop().startBeat;
        slot.loopLengthBeats = project_.loop().lengthBeats;
        slot.loopEnabled = project_.loop().enabled;
        slot.songLengthBeats = project_.songLengthBeats();

        const bool anySolo = projectHasSolo(project_.tracks());

        for (const auto& track : project_.tracks())
        {
            if (! isTrackAudible(track, anySolo))
                continue;

            if (track.type == TrackType::Midi)
            {
                const auto instrumentId = isValidInstrumentId(track.instrumentId)
                                              ? track.instrumentId
                                              : defaultInstrumentForRole(track.role);
                const auto program = soundProgramForInstrument(instrumentId);
                for (const auto& clip : track.clips)
                {
                    for (const auto& note : clip.notes)
                    {
                        // Non-destructive clip resize: keep note data, emit only in-range content.
                        if (note.startBeat >= clip.lengthBeats - 1.0e-9)
                            continue;
                        if (slot.noteCount >= SequenceSnapshot::kMaxNotes)
                            break;

                        const double localEnd = std::min(note.endBeat(), clip.lengthBeats);
                        ScheduledNote scheduled;
                        scheduled.id = note.id;
                        scheduled.pitch = note.pitch;
                        scheduled.velocity = note.velocity;
                        scheduled.startBeat = clip.startBeat + note.startBeat;
                        scheduled.endBeat = clip.startBeat + localEnd;
                        scheduled.instrumentId = instrumentId;
                        scheduled.program = program;
                        if (scheduled.endBeat <= scheduled.startBeat + 1.0e-9)
                            continue;
                        if (scheduled.startBeat >= project_.songLengthBeats() - 1.0e-9)
                            continue;
                        if (scheduled.endBeat > project_.songLengthBeats())
                            scheduled.endBeat = project_.songLengthBeats();
                        slot.notes[slot.noteCount++] = scheduled;
                    }
                }
            }
            else if (track.type == TrackType::Drums)
            {
                const auto kitId = isValidInstrumentId(track.instrumentId)
                                       ? track.instrumentId
                                       : InstrumentId::StudioKitBasic;
                for (const auto& clip : track.drumClips)
                {
                    const double beatPerStep = drumBeatPerStep(clip.lengthBeats, clip.pattern.stepCount);
                    for (const auto& hit : clip.pattern.hits)
                    {
                        if (slot.drumHitCount >= SequenceSnapshot::kMaxDrumHits)
                            break;
                        const double localBeat = static_cast<double>(hit.step) * beatPerStep;
                        if (localBeat >= clip.lengthBeats - 1.0e-9)
                            continue;
                        const double absBeat = clip.startBeat + localBeat;
                        if (absBeat >= project_.songLengthBeats() - 1.0e-9)
                            continue;
                        ScheduledDrumHit d;
                        d.beat = absBeat;
                        d.velocity = hit.velocity;
                        d.program = drumProgramForLane(hit.lane);
                        d.instrumentId = kitId;
                        slot.drumHits[slot.drumHitCount++] = d;
                    }
                }
            }
        }

        snapshots_.publish();
        syncTransportFromProject();
    }

private:
    struct GeneratedFingerprint
    {
        Id trackId = kInvalidId;
        std::uint64_t digest = 0;
    };

    void ensureSelectedClipForTrack() noexcept
    {
        auto* track = project_.findTrack(selectedTrackId_);
        if (track == nullptr)
        {
            selectedClipId_ = kInvalidId;
            return;
        }
        if (track->type == TrackType::Drums)
        {
            if (project_.findDrumClip(track->id, selectedClipId_) != nullptr)
                return;
            selectedClipId_ = track->drumClips.empty() ? kInvalidId : track->drumClips.front().id;
            return;
        }
        if (project_.findClip(track->id, selectedClipId_) != nullptr)
            return;
        selectedClipId_ = track->clips.empty() ? kInvalidId : track->clips.front().id;
    }

    [[nodiscard]] MidiClip* resolveMidiClip(Track& track) noexcept
    {
        if (auto* clip = project_.findClip(track.id, selectedClipId_))
            return clip;
        return track.clips.empty() ? nullptr : &track.clips.front();
    }

    [[nodiscard]] DrumClip* resolveDrumClip(Track& track) noexcept
    {
        if (auto* clip = project_.findDrumClip(track.id, selectedClipId_))
            return clip;
        return track.drumClips.empty() ? nullptr : &track.drumClips.front();
    }

    bool executeInternal(std::unique_ptr<Command> command, bool fromSensei)
    {
        const auto before = fromSensei ? std::vector<GeneratedFingerprint> {}
                                       : captureGeneratedFingerprints();

        if (! history_.execute(project_, std::move(command)))
            return false;

        if (! fromSensei && generatedContentChanged(before))
            maybeMarkUserModified();

        maybeCelebrateCompleteLoop();
        ensureSelectedClipForTrack();
        syncTransportFromProject();
        publishSnapshot();
        return true;
    }

    [[nodiscard]] std::vector<GeneratedFingerprint> captureGeneratedFingerprints() const
    {
        std::vector<GeneratedFingerprint> out;
        for (const auto& track : project_.tracks())
        {
            if (! track.generatedOrigin)
                continue;
            out.push_back({ track.id, contentDigest(track) });
        }
        return out;
    }

    [[nodiscard]] bool generatedContentChanged(const std::vector<GeneratedFingerprint>& before) const
    {
        for (const auto& fp : before)
        {
            const auto* track = project_.findTrack(fp.trackId);
            if (track == nullptr || ! track->generatedOrigin)
                continue;
            if (contentDigest(*track) != fp.digest)
                return true;
        }
        return false;
    }

    [[nodiscard]] static std::uint64_t contentDigest(const Track& track) noexcept
    {
        std::uint64_t digest = 1469598103934665603ull;
        auto mix = [&digest](std::uint64_t value) {
            digest ^= value;
            digest *= 1099511628211ull;
        };

        if (track.type == TrackType::Drums)
        {
            mix(track.drumClips.size());
            for (const auto& clip : track.drumClips)
            {
                mix(static_cast<std::uint64_t>(clip.id));
                mix(static_cast<std::uint64_t>(clip.startBeat * 1000.0));
                mix(static_cast<std::uint64_t>(clip.lengthBeats * 1000.0));
                mix(static_cast<std::uint64_t>(clip.pattern.stepCount));
                mix(clip.pattern.hits.size());
                for (const auto& hit : clip.pattern.hits)
                {
                    mix(static_cast<std::uint64_t>(hit.step));
                    mix(static_cast<std::uint64_t>(hit.lane));
                    mix(static_cast<std::uint64_t>(hit.velocity * 1000.0f));
                }
            }
            return digest;
        }

        for (const auto& clip : track.clips)
        {
            mix(static_cast<std::uint64_t>(clip.id));
            mix(static_cast<std::uint64_t>(clip.startBeat * 1000.0));
            mix(static_cast<std::uint64_t>(clip.lengthBeats * 1000.0));
            mix(clip.notes.size());
            for (const auto& note : clip.notes)
            {
                mix(static_cast<std::uint64_t>(note.id));
                mix(static_cast<std::uint64_t>(note.pitch));
                mix(static_cast<std::uint64_t>(note.startBeat * 1000.0));
                mix(static_cast<std::uint64_t>(note.lengthBeats * 1000.0));
                mix(static_cast<std::uint64_t>(note.velocity * 1000.0f));
            }
        }
        return digest;
    }

    static DrumProgram drumProgramForLane(DrumLane lane) noexcept
    {
        switch (lane)
        {
            case DrumLane::Snare: return DrumProgram::Snare;
            case DrumLane::ClosedHat: return DrumProgram::ClosedHat;
            case DrumLane::Kick:
            default: return DrumProgram::Kick;
        }
    }

    void pushEvent(LearningEventKind kind)
    {
        lesson_.events.push_back(makeEvent(kind));
    }

    void maybeCelebrateCompleteLoop()
    {
        if (! lesson_.celebratedCompleteLoop && isCompleteLoop(project_))
        {
            pushEvent(LearningEventKind::FirstCompleteLoop);
            lesson_.celebratedCompleteLoop = true;
            lesson_.step = LessonStep::OfferSongShape;
        }
    }

    void maybeMarkUserModified()
    {
        if (lesson_.userModifiedGeneratedEmitted)
            return;

        pushEvent(LearningEventKind::UserModifiedGenerated);
        lesson_.userModifiedGeneratedEmitted = true;
        for (auto& track : project_.tracks())
            track.generatedOrigin = false;
    }

    Project project_;
    Transport transport_;
    CommandHistory history_;
    SnapshotPublisher snapshots_;
    LessonState lesson_;
    Id selectedNoteId_ = kInvalidId;
    Id selectedTrackId_ = kInvalidId;
    Id selectedClipId_ = kInvalidId;
    std::uint64_t generation_ = 0;
};

} // namespace sensei::core
