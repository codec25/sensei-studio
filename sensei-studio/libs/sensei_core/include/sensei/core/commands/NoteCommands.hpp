#pragma once

#include "sensei/core/Grid.hpp"
#include "sensei/core/Note.hpp"
#include "sensei/core/commands/Command.hpp"

#include <algorithm>

namespace sensei::core {

class AddNoteCommand final : public Command
{
public:
    AddNoteCommand(Id trackId, Id clipId, int pitch, double startBeat, double lengthBeats, float velocity)
        : trackId_(trackId)
        , clipId_(clipId)
        , pitch_(clampMidiNote(pitch))
        , startBeat_(clampNonNegativeBeat(snapBeat(startBeat)))
        , lengthBeats_(snapLength(lengthBeats))
        , velocity_(clampVelocity(velocity))
    {
    }

    [[nodiscard]] std::string name() const override { return "Add Note"; }

    [[nodiscard]] Id createdNoteId() const noexcept { return noteId_; }

    bool perform(Project& project) override
    {
        auto* clip = project.findClip(trackId_, clipId_);
        if (clip == nullptr)
            return false;

        if (noteId_ == kInvalidId)
            noteId_ = project.generateId();

        // Replace existing note with same id on redo.
        clip->notes.erase(std::remove_if(clip->notes.begin(),
                                         clip->notes.end(),
                                         [this](const MidiNote& n) { return n.id == noteId_; }),
                          clip->notes.end());

        MidiNote note;
        note.id = noteId_;
        note.pitch = pitch_;
        note.startBeat = startBeat_;
        note.lengthBeats = lengthBeats_;
        note.velocity = velocity_;
        clip->notes.push_back(note);
        return true;
    }

    void undo(Project& project) override
    {
        auto* clip = project.findClip(trackId_, clipId_);
        if (clip == nullptr)
            return;

        clip->notes.erase(std::remove_if(clip->notes.begin(),
                                         clip->notes.end(),
                                         [this](const MidiNote& n) { return n.id == noteId_; }),
                          clip->notes.end());
    }

private:
    Id trackId_;
    Id clipId_;
    Id noteId_ = kInvalidId;
    int pitch_;
    double startBeat_;
    double lengthBeats_;
    float velocity_;
};

class DeleteNoteCommand final : public Command
{
public:
    DeleteNoteCommand(Id trackId, Id clipId, Id noteId)
        : trackId_(trackId), clipId_(clipId), noteId_(noteId)
    {
    }

    [[nodiscard]] std::string name() const override { return "Delete Note"; }

    bool perform(Project& project) override
    {
        auto* clip = project.findClip(trackId_, clipId_);
        if (clip == nullptr)
            return false;

        const auto it = std::find_if(clip->notes.begin(),
                                     clip->notes.end(),
                                     [this](const MidiNote& n) { return n.id == noteId_; });
        if (it == clip->notes.end())
            return false;

        removed_ = *it;
        hadNote_ = true;
        clip->notes.erase(it);
        return true;
    }

    void undo(Project& project) override
    {
        if (! hadNote_)
            return;

        auto* clip = project.findClip(trackId_, clipId_);
        if (clip == nullptr)
            return;

        clip->notes.push_back(removed_);
    }

private:
    Id trackId_;
    Id clipId_;
    Id noteId_;
    MidiNote removed_ {};
    bool hadNote_ = false;
};

class MoveNoteCommand final : public Command
{
public:
    MoveNoteCommand(Id trackId, Id clipId, Id noteId, double newStartBeat, int newPitch)
        : trackId_(trackId)
        , clipId_(clipId)
        , noteId_(noteId)
        , newStartBeat_(clampNonNegativeBeat(snapBeat(newStartBeat)))
        , newPitch_(clampMidiNote(newPitch))
    {
    }

    [[nodiscard]] std::string name() const override { return "Move Note"; }

    bool perform(Project& project) override
    {
        auto* note = project.findNote(trackId_, clipId_, noteId_);
        if (note == nullptr)
            return false;

        if (! captured_)
        {
            oldStartBeat_ = note->startBeat;
            oldPitch_ = note->pitch;
            captured_ = true;
        }

        note->startBeat = newStartBeat_;
        note->pitch = newPitch_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* note = project.findNote(trackId_, clipId_, noteId_);
        if (note == nullptr || ! captured_)
            return;

        note->startBeat = oldStartBeat_;
        note->pitch = oldPitch_;
    }

private:
    Id trackId_;
    Id clipId_;
    Id noteId_;
    double newStartBeat_;
    int newPitch_;
    double oldStartBeat_ = 0.0;
    int oldPitch_ = 0;
    bool captured_ = false;
};

class ResizeNoteCommand final : public Command
{
public:
    ResizeNoteCommand(Id trackId, Id clipId, Id noteId, double newLengthBeats)
        : trackId_(trackId)
        , clipId_(clipId)
        , noteId_(noteId)
        , newLengthBeats_(snapLength(newLengthBeats))
    {
    }

    [[nodiscard]] std::string name() const override { return "Resize Note"; }

    bool perform(Project& project) override
    {
        auto* note = project.findNote(trackId_, clipId_, noteId_);
        if (note == nullptr)
            return false;

        if (! captured_)
        {
            oldLengthBeats_ = note->lengthBeats;
            captured_ = true;
        }

        note->lengthBeats = newLengthBeats_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* note = project.findNote(trackId_, clipId_, noteId_);
        if (note == nullptr || ! captured_)
            return;

        note->lengthBeats = oldLengthBeats_;
    }

private:
    Id trackId_;
    Id clipId_;
    Id noteId_;
    double newLengthBeats_;
    double oldLengthBeats_ = 0.0;
    bool captured_ = false;
};

} // namespace sensei::core
