#pragma once

#include "sensei/core/Project.hpp"
#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/core/bass/BassGenerator.hpp"
#include "sensei/core/commands/CommandHistory.hpp"
#include "sensei/core/commands/CompoundCommand.hpp"
#include "sensei/core/commands/NoteCommands.hpp"
#include "sensei/core/commands/TrackContentCommands.hpp"
#include "sensei/core/drums/DrumPatterns.hpp"
#include "sensei/core/harmony/Progressions.hpp"
#include "sensei/core/sensei/LessonFlow.hpp"
#include "sensei/core/sensei/ProjectAnalyzer.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sensei::core {

class Document
{
public:
    Document()
        : project_(Project::createStarter())
    {
        transport_.setBpm(kDefaultBpm);
        if (auto* chords = project_.findTrackByRole(TrackRole::Chords))
            selectedTrackId_ = chords->id;
        syncTransportLoopFromProject();
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
    void setSelectedTrackId(Id id) noexcept { selectedTrackId_ = id; }

    void syncTransportLoopFromProject() noexcept
    {
        const auto& loop = project_.loop();
        transport_.setLoop(loop.startBeat, loop.lengthBeats, loop.enabled);
    }

    void setBpm(double bpm) noexcept
    {
        transport_.setBpm(bpm);
        publishSnapshot();
    }

    bool execute(std::unique_ptr<Command> command)
    {
        if (! history_.execute(project_, std::move(command)))
            return false;
        maybeMarkUserModified();
        maybeCelebrateCompleteLoop();
        publishSnapshot();
        return true;
    }

    bool undo()
    {
        if (! history_.undo(project_))
            return false;
        publishSnapshot();
        return true;
    }

    bool redo()
    {
        if (! history_.redo(project_))
            return false;
        publishSnapshot();
        return true;
    }

    bool applyProgression(int rootPc, ScaleMode mode, const char* progressionId)
    {
        auto* track = project_.findTrackByRole(TrackRole::Chords);
        if (track == nullptr || track->clips.empty())
            return false;

        auto material = generateProgression(rootPc, mode, progressionId, project_.loop().lengthBeats);
        for (auto& n : material.notes)
            n.id = kInvalidId;

        auto compound = std::make_unique<CompoundCommand>("Add chord progression");
        compound->add(std::make_unique<SetHarmonyCommand>(material.harmony));
        compound->add(std::make_unique<ReplaceClipNotesCommand>(
            track->id, track->clips.front().id, std::move(material.notes)));

        if (! execute(std::move(compound)))
            return false;

        pushEvent(LearningEventKind::ChordProgressionCreated);
        lesson_.chordsAccepted = true;
        lesson_.step = LessonStep::OfferDrums;
        lesson_.quiet = false;
        return true;
    }

    bool applyStarterDrums(const char* patternName = "basic-rock")
    {
        auto* track = project_.findTrackByRole(TrackRole::Drums);
        if (track == nullptr)
            return false;

        DrumPattern pattern = makeBasicRockPattern();
        if (std::string(patternName) == "light-pop")
            pattern = makeLightPopPattern();
        else if (std::string(patternName) == "four-on-floor")
            pattern = makeFourOnFloorPattern();
        pattern.id = kInvalidId;

        auto compound = std::make_unique<CompoundCommand>("Add starter drums");
        compound->add(std::make_unique<ReplaceDrumPatternCommand>(track->id, std::move(pattern)));
        if (! execute(std::move(compound)))
            return false;

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

        auto notes = generateRootBass(project_.harmony());
        for (auto& n : notes)
            n.id = kInvalidId;

        auto compound = std::make_unique<CompoundCommand>("Add root-note bass");
        compound->add(std::make_unique<ReplaceClipNotesCommand>(
            track->id, track->clips.front().id, std::move(notes)));
        if (! execute(std::move(compound)))
            return false;

        pushEvent(LearningEventKind::RootBassAdded);
        lesson_.bassAccepted = true;
        maybeCelebrateCompleteLoop();
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
                if (lesson_.step == LessonStep::Quiet || lesson_.step == LessonStep::CelebrateLoop)
                    lesson_.step = ! lesson_.chordsAccepted ? LessonStep::ChooseKey
                                  : ! lesson_.drumsAccepted ? LessonStep::OfferDrums
                                  : ! lesson_.bassAccepted  ? LessonStep::OfferBass
                                                           : LessonStep::CelebrateLoop;
                break;
        }
    }

    [[nodiscard]] Observation analyze() const
    {
        if (! lesson_.quiet && ! lesson_.events.empty())
            return observationFromLesson(lesson_, project_);
        if (isCompleteLoop(project_))
            return observationFromLesson(lesson_, project_);
        return ProjectAnalyzer::analyze(project_);
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

        for (const auto& track : project_.tracks())
        {
            if (track.type == TrackType::Midi)
            {
                const auto program = programForRole(track.role);
                for (const auto& clip : track.clips)
                {
                    for (const auto& note : clip.notes)
                    {
                        if (slot.noteCount >= SequenceSnapshot::kMaxNotes)
                            break;
                        ScheduledNote scheduled;
                        scheduled.id = note.id;
                        scheduled.pitch = note.pitch;
                        scheduled.velocity = note.velocity;
                        scheduled.startBeat = clip.startBeat + note.startBeat;
                        scheduled.endBeat = clip.startBeat + note.endBeat();
                        scheduled.program = program;
                        slot.notes[slot.noteCount++] = scheduled;
                    }
                }
            }
            else if (track.type == TrackType::Drums)
            {
                const double secondsPerStep = project_.loop().lengthBeats
                                              / static_cast<double>(track.drumPattern.stepCount > 0
                                                                        ? track.drumPattern.stepCount
                                                                        : kDefaultDrumSteps);
                // step -> beat: step * (1/16 note) = step * 0.25
                for (const auto& hit : track.drumPattern.hits)
                {
                    if (slot.drumHitCount >= SequenceSnapshot::kMaxDrumHits)
                        break;
                    ScheduledDrumHit d;
                    d.beat = static_cast<double>(hit.step) * 0.25;
                    d.velocity = hit.velocity;
                    d.program = drumProgramForLane(hit.lane);
                    slot.drumHits[slot.drumHitCount++] = d;
                }
                (void) secondsPerStep;
            }
        }

        snapshots_.publish();
        syncTransportLoopFromProject();
    }

private:
    static SoundProgram programForRole(TrackRole role) noexcept
    {
        switch (role)
        {
            case TrackRole::Bass: return SoundProgram::Bass;
            case TrackRole::Melody: return SoundProgram::Melody;
            case TrackRole::Chords:
            default: return SoundProgram::Chords;
        }
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
            lesson_.step = LessonStep::CelebrateLoop;
        }
    }

    void maybeMarkUserModified()
    {
        // Lightweight: if user edits after generated material exists, note it once.
        if ((lesson_.chordsAccepted || lesson_.drumsAccepted || lesson_.bassAccepted)
            && lesson_.events.empty())
        {
            return;
        }
    }

    Project project_;
    Transport transport_;
    CommandHistory history_;
    SnapshotPublisher snapshots_;
    LessonState lesson_;
    Id selectedNoteId_ = kInvalidId;
    Id selectedTrackId_ = kInvalidId;
    std::uint64_t generation_ = 0;
};

} // namespace sensei::core
