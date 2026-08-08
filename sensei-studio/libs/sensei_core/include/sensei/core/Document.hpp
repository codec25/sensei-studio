#pragma once

#include "sensei/core/Project.hpp"
#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/core/commands/CommandHistory.hpp"
#include "sensei/core/commands/NoteCommands.hpp"
#include "sensei/core/sensei/ProjectAnalyzer.hpp"

#include <cstdint>
#include <memory>

namespace sensei::core {

// Message-thread document facade. Owns canonical musical data + transport + history.
// Publishes fixed-slot SequenceSnapshots for the audio engine.
class Document
{
public:
    Document()
        : project_(Project::createStarter())
    {
        transport_.setBpm(kDefaultBpm);
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

    [[nodiscard]] Id selectedNoteId() const noexcept { return selectedNoteId_; }
    void setSelectedNoteId(Id id) noexcept { selectedNoteId_ = id; }

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

    [[nodiscard]] Observation analyze() const
    {
        return ProjectAnalyzer::analyze(project_);
    }

    // Rebuilds a fixed snapshot slot and publishes its index. Message thread only.
    void publishSnapshot()
    {
        auto& slot = snapshots_.beginWrite();
        slot = {};
        slot.generation = ++generation_;
        slot.bpm = transport_.bpm();
        slot.loopStartBeats = project_.loop().startBeat;
        slot.loopLengthBeats = project_.loop().lengthBeats;
        slot.loopEnabled = project_.loop().enabled;

        if (const auto* clip = project_.primaryClip())
        {
            for (const auto& note : clip->notes)
            {
                if (slot.noteCount >= SequenceSnapshot::kMaxNotes)
                    break;

                ScheduledNote scheduled;
                scheduled.id = note.id;
                scheduled.pitch = note.pitch;
                scheduled.velocity = note.velocity;
                // Convert clip-local note times to absolute beats.
                scheduled.startBeat = clip->startBeat + note.startBeat;
                scheduled.endBeat = clip->startBeat + note.endBeat();
                slot.notes[slot.noteCount++] = scheduled;
            }
        }

        snapshots_.publish();
        syncTransportLoopFromProject();
    }

private:
    Project project_;
    Transport transport_;
    CommandHistory history_;
    SnapshotPublisher snapshots_;
    Id selectedNoteId_ = kInvalidId;
    std::uint64_t generation_ = 0;
};

} // namespace sensei::core
