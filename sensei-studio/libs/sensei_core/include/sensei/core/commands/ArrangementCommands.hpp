#pragma once

#include "sensei/core/Grid.hpp"
#include "sensei/core/Note.hpp"
#include "sensei/core/Section.hpp"
#include "sensei/core/commands/Command.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace sensei::core {

class SetSongLengthCommand final : public Command
{
public:
    explicit SetSongLengthCommand(double lengthBeats)
        : newLength_(lengthBeats > 0.0 ? lengthBeats : kDefaultLoopBeats)
    {
    }

    [[nodiscard]] std::string name() const override { return "Set Song Length"; }

    bool perform(Project& project) override
    {
        if (! captured_)
        {
            oldLength_ = project.songLengthBeats();
            captured_ = true;
        }
        project.setSongLengthBeats(newLength_);
        return true;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        project.setSongLengthBeats(oldLength_);
    }

private:
    double newLength_;
    double oldLength_ = kDefaultLoopBeats;
    bool captured_ = false;
};

class SetLoopRegionCommand final : public Command
{
public:
    SetLoopRegionCommand(double startBeat, double lengthBeats, bool enabled)
        : newLoop_ { std::max(0.0, startBeat),
                     lengthBeats > 0.0 ? lengthBeats : kDefaultLoopBeats,
                     enabled }
    {
    }

    [[nodiscard]] std::string name() const override { return "Set Loop Region"; }

    bool perform(Project& project) override
    {
        if (! captured_)
        {
            oldLoop_ = project.loop();
            captured_ = true;
        }
        project.loop() = newLoop_;
        return true;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        project.loop() = oldLoop_;
    }

private:
    LoopRegion newLoop_ {};
    LoopRegion oldLoop_ {};
    bool captured_ = false;
};

class CreateSectionCommand final : public Command
{
public:
    CreateSectionCommand(std::string name, SectionLabel label, double startBeat, double lengthBeats)
        : name_(std::move(name))
        , label_(label)
        , startBeat_(std::max(0.0, startBeat))
        , lengthBeats_(lengthBeats > 0.0 ? lengthBeats : kDefaultLoopBeats)
    {
    }

    [[nodiscard]] std::string name() const override { return "Create Section"; }
    [[nodiscard]] Id createdSectionId() const noexcept { return sectionId_; }

    bool perform(Project& project) override
    {
        // Reject overlapping sections (Milestone D beginner rule).
        if (sectionRangeConflicts(project.sections(), startBeat_, lengthBeats_, sectionId_))
            return false;

        if (sectionId_ == kInvalidId)
            sectionId_ = project.generateId();

        // Replace existing with same id on redo.
        project.sections().erase(std::remove_if(project.sections().begin(),
                                                project.sections().end(),
                                                [this](const Section& s) { return s.id == sectionId_; }),
                                 project.sections().end());

        Section section;
        section.id = sectionId_;
        section.name = name_;
        section.label = label_;
        section.startBeat = startBeat_;
        section.lengthBeats = lengthBeats_;
        project.sections().push_back(section);
        std::sort(project.sections().begin(), project.sections().end(),
                  [](const Section& a, const Section& b) { return a.startBeat < b.startBeat; });
        return true;
    }

    void undo(Project& project) override
    {
        project.sections().erase(std::remove_if(project.sections().begin(),
                                                project.sections().end(),
                                                [this](const Section& s) { return s.id == sectionId_; }),
                                 project.sections().end());
    }

private:
    std::string name_;
    SectionLabel label_;
    double startBeat_;
    double lengthBeats_;
    Id sectionId_ = kInvalidId;
};

class RenameSectionCommand final : public Command
{
public:
    RenameSectionCommand(Id sectionId, std::string name, SectionLabel label)
        : sectionId_(sectionId), newName_(std::move(name)), newLabel_(label)
    {
    }

    [[nodiscard]] std::string name() const override { return "Rename Section"; }

    bool perform(Project& project) override
    {
        auto* section = project.findSection(sectionId_);
        if (section == nullptr)
            return false;
        if (! captured_)
        {
            oldName_ = section->name;
            oldLabel_ = section->label;
            captured_ = true;
        }
        section->name = newName_;
        section->label = newLabel_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* section = project.findSection(sectionId_);
        if (section == nullptr || ! captured_)
            return;
        section->name = oldName_;
        section->label = oldLabel_;
    }

private:
    Id sectionId_;
    std::string newName_;
    SectionLabel newLabel_;
    std::string oldName_;
    SectionLabel oldLabel_ = SectionLabel::Custom;
    bool captured_ = false;
};

class ResizeSectionCommand final : public Command
{
public:
    ResizeSectionCommand(Id sectionId, double newStartBeat, double newLengthBeats)
        : sectionId_(sectionId)
        , newStart_(std::max(0.0, newStartBeat))
        , newLength_(newLengthBeats > 0.0 ? newLengthBeats : kBeatsPerBar)
    {
    }

    [[nodiscard]] std::string name() const override { return "Resize Section"; }

    bool perform(Project& project) override
    {
        auto* section = project.findSection(sectionId_);
        if (section == nullptr)
            return false;

        // Reject if the new range would overlap another section.
        if (sectionRangeConflicts(project.sections(), newStart_, newLength_, sectionId_))
            return false;

        if (! captured_)
        {
            oldStart_ = section->startBeat;
            oldLength_ = section->lengthBeats;
            captured_ = true;
        }
        section->startBeat = newStart_;
        section->lengthBeats = newLength_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* section = project.findSection(sectionId_);
        if (section == nullptr || ! captured_)
            return;
        section->startBeat = oldStart_;
        section->lengthBeats = oldLength_;
    }

private:
    Id sectionId_;
    double newStart_;
    double newLength_;
    double oldStart_ = 0.0;
    double oldLength_ = 0.0;
    bool captured_ = false;
};

class AddMidiClipCommand final : public Command
{
public:
    AddMidiClipCommand(Id trackId, MidiClip clip)
        : trackId_(trackId), clip_(std::move(clip))
    {
    }

    [[nodiscard]] std::string name() const override { return "Add Clip"; }
    [[nodiscard]] Id createdClipId() const noexcept { return clip_.id; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr || track->type != TrackType::Midi)
            return false;
        if (clip_.id == kInvalidId)
            clip_.id = project.generateId();
        for (auto& note : clip_.notes)
            if (note.id == kInvalidId)
                note.id = project.generateId();

        track->clips.erase(std::remove_if(track->clips.begin(), track->clips.end(),
                                          [this](const MidiClip& c) { return c.id == clip_.id; }),
                           track->clips.end());
        track->clips.push_back(clip_);
        return true;
    }

    void undo(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return;
        track->clips.erase(std::remove_if(track->clips.begin(), track->clips.end(),
                                          [this](const MidiClip& c) { return c.id == clip_.id; }),
                           track->clips.end());
    }

private:
    Id trackId_;
    MidiClip clip_;
};

class AddDrumClipCommand final : public Command
{
public:
    AddDrumClipCommand(Id trackId, DrumClip clip)
        : trackId_(trackId), clip_(std::move(clip))
    {
    }

    [[nodiscard]] std::string name() const override { return "Add Drum Clip"; }
    [[nodiscard]] Id createdClipId() const noexcept { return clip_.id; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr || track->type != TrackType::Drums)
            return false;
        if (clip_.id == kInvalidId)
            clip_.id = project.generateId();
        if (clip_.pattern.id == kInvalidId)
            clip_.pattern.id = project.generateId();

        track->drumClips.erase(std::remove_if(track->drumClips.begin(), track->drumClips.end(),
                                              [this](const DrumClip& c) { return c.id == clip_.id; }),
                               track->drumClips.end());
        track->drumClips.push_back(clip_);
        return true;
    }

    void undo(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return;
        track->drumClips.erase(std::remove_if(track->drumClips.begin(), track->drumClips.end(),
                                              [this](const DrumClip& c) { return c.id == clip_.id; }),
                               track->drumClips.end());
    }

private:
    Id trackId_;
    DrumClip clip_;
};

class MoveClipCommand final : public Command
{
public:
    MoveClipCommand(Id trackId, Id clipId, double newStartBeat)
        : trackId_(trackId), clipId_(clipId), newStart_(std::max(0.0, snapBeat(newStartBeat)))
    {
    }

    [[nodiscard]] std::string name() const override { return "Move Clip"; }

    bool perform(Project& project) override
    {
        if (auto* clip = project.findClip(trackId_, clipId_))
        {
            if (! captured_)
            {
                oldStart_ = clip->startBeat;
                captured_ = true;
            }
            clip->startBeat = newStart_;
            return true;
        }
        if (auto* clip = project.findDrumClip(trackId_, clipId_))
        {
            if (! captured_)
            {
                oldStart_ = clip->startBeat;
                captured_ = true;
            }
            clip->startBeat = newStart_;
            return true;
        }
        return false;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        if (auto* midi = project.findClip(trackId_, clipId_))
            midi->startBeat = oldStart_;
        else if (auto* drum = project.findDrumClip(trackId_, clipId_))
            drum->startBeat = oldStart_;
    }

private:
    Id trackId_;
    Id clipId_;
    double newStart_;
    double oldStart_ = 0.0;
    bool captured_ = false;
};

// Non-destructive resize: note/hit data is preserved; snapshot/playback gates by length.
class ResizeClipCommand final : public Command
{
public:
    ResizeClipCommand(Id trackId, Id clipId, double newLengthBeats)
        : trackId_(trackId)
        , clipId_(clipId)
        , newLength_(newLengthBeats > 0.0 ? snapLength(newLengthBeats) : kBeatsPerBar)
    {
    }

    [[nodiscard]] std::string name() const override { return "Resize Clip"; }

    bool perform(Project& project) override
    {
        if (auto* clip = project.findClip(trackId_, clipId_))
        {
            if (! captured_)
            {
                oldLength_ = clip->lengthBeats;
                captured_ = true;
            }
            clip->lengthBeats = newLength_;
            return true;
        }
        if (auto* clip = project.findDrumClip(trackId_, clipId_))
        {
            if (! captured_)
            {
                oldLength_ = clip->lengthBeats;
                captured_ = true;
            }
            clip->lengthBeats = newLength_;
            return true;
        }
        return false;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        if (auto* midi = project.findClip(trackId_, clipId_))
            midi->lengthBeats = oldLength_;
        else if (auto* drum = project.findDrumClip(trackId_, clipId_))
            drum->lengthBeats = oldLength_;
    }

private:
    Id trackId_;
    Id clipId_;
    double newLength_;
    double oldLength_ = 0.0;
    bool captured_ = false;
};

class DeleteClipCommand final : public Command
{
public:
    DeleteClipCommand(Id trackId, Id clipId)
        : trackId_(trackId), clipId_(clipId)
    {
    }

    [[nodiscard]] std::string name() const override { return "Delete Clip"; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return false;

        if (track->type == TrackType::Midi)
        {
            const auto it = std::find_if(track->clips.begin(), track->clips.end(),
                                         [this](const MidiClip& c) { return c.id == clipId_; });
            if (it == track->clips.end())
                return false;
            if (! captured_)
            {
                midi_ = *it;
                isDrum_ = false;
                captured_ = true;
            }
            track->clips.erase(it);
            return true;
        }

        const auto it = std::find_if(track->drumClips.begin(), track->drumClips.end(),
                                     [this](const DrumClip& c) { return c.id == clipId_; });
        if (it == track->drumClips.end())
            return false;
        if (! captured_)
        {
            drum_ = *it;
            isDrum_ = true;
            captured_ = true;
        }
        track->drumClips.erase(it);
        return true;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return;
        if (isDrum_)
            track->drumClips.push_back(drum_);
        else
            track->clips.push_back(midi_);
    }

private:
    Id trackId_;
    Id clipId_;
    MidiClip midi_ {};
    DrumClip drum_ {};
    bool isDrum_ = false;
    bool captured_ = false;
};

class DuplicateClipCommand final : public Command
{
public:
    DuplicateClipCommand(Id trackId, Id clipId, double newStartBeat)
        : trackId_(trackId), sourceClipId_(clipId), newStart_(std::max(0.0, snapBeat(newStartBeat)))
    {
    }

    [[nodiscard]] std::string name() const override { return "Duplicate Clip"; }
    [[nodiscard]] Id createdClipId() const noexcept { return createdId_; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return false;

        if (track->type == TrackType::Midi)
        {
            const auto* source = project.findClip(trackId_, sourceClipId_);
            if (source == nullptr)
                return false;
            if (! captured_)
            {
                copyMidi_ = *source;
                copyMidi_.id = project.generateId();
                copyMidi_.startBeat = newStart_;
                copyMidi_.name = source->name + " copy";
                for (auto& note : copyMidi_.notes)
                    note.id = project.generateId();
                createdId_ = copyMidi_.id;
                captured_ = true;
            }
            track->clips.erase(std::remove_if(track->clips.begin(), track->clips.end(),
                                              [this](const MidiClip& c) { return c.id == createdId_; }),
                               track->clips.end());
            track->clips.push_back(copyMidi_);
            return true;
        }

        const auto* source = project.findDrumClip(trackId_, sourceClipId_);
        if (source == nullptr)
            return false;
        if (! captured_)
        {
            copyDrum_ = *source;
            copyDrum_.id = project.generateId();
            copyDrum_.startBeat = newStart_;
            copyDrum_.name = source->name + " copy";
            copyDrum_.pattern.id = project.generateId();
            createdId_ = copyDrum_.id;
            captured_ = true;
        }
        track->drumClips.erase(std::remove_if(track->drumClips.begin(), track->drumClips.end(),
                                              [this](const DrumClip& c) { return c.id == createdId_; }),
                               track->drumClips.end());
        track->drumClips.push_back(copyDrum_);
        return true;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return;
        if (track->type == TrackType::Drums)
        {
            track->drumClips.erase(std::remove_if(track->drumClips.begin(), track->drumClips.end(),
                                                  [this](const DrumClip& c) { return c.id == createdId_; }),
                                   track->drumClips.end());
        }
        else
        {
            track->clips.erase(std::remove_if(track->clips.begin(), track->clips.end(),
                                              [this](const MidiClip& c) { return c.id == createdId_; }),
                               track->clips.end());
        }
    }

private:
    Id trackId_;
    Id sourceClipId_;
    double newStart_;
    Id createdId_ = kInvalidId;
    MidiClip copyMidi_ {};
    DrumClip copyDrum_ {};
    bool captured_ = false;
};

} // namespace sensei::core
