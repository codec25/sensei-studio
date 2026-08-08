#pragma once

#include "sensei/core/Grid.hpp"
#include "sensei/core/Note.hpp"
#include "sensei/core/commands/Command.hpp"
#include "sensei/core/harmony/Chord.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace sensei::core {

// Replace all notes in a MIDI clip (used by chord/bass generators).
class ReplaceClipNotesCommand final : public Command
{
public:
    ReplaceClipNotesCommand(Id trackId, Id clipId, std::vector<MidiNote> notes)
        : trackId_(trackId), clipId_(clipId), newNotes_(std::move(notes))
    {
    }

    [[nodiscard]] std::string name() const override { return "Replace Clip Notes"; }

    bool perform(Project& project) override
    {
        auto* clip = project.findClip(trackId_, clipId_);
        if (clip == nullptr)
            return false;

        if (! captured_)
        {
            oldNotes_ = clip->notes;
            captured_ = true;
        }

        clip->notes.clear();
        for (auto note : newNotes_)
        {
            if (note.id == kInvalidId)
                note.id = project.generateId();
            note.pitch = clampMidiNote(note.pitch);
            note.startBeat = clampNonNegativeBeat(note.startBeat);
            note.lengthBeats = snapLength(note.lengthBeats);
            note.velocity = clampVelocity(note.velocity);
            clip->notes.push_back(note);
        }
        return true;
    }

    void undo(Project& project) override
    {
        auto* clip = project.findClip(trackId_, clipId_);
        if (clip == nullptr || ! captured_)
            return;
        clip->notes = oldNotes_;
    }

private:
    Id trackId_;
    Id clipId_;
    std::vector<MidiNote> newNotes_;
    std::vector<MidiNote> oldNotes_;
    bool captured_ = false;
};

class SetHarmonyCommand final : public Command
{
public:
    explicit SetHarmonyCommand(HarmonyState harmony)
        : newHarmony_(std::move(harmony))
    {
    }

    [[nodiscard]] std::string name() const override { return "Set Harmony"; }

    bool perform(Project& project) override
    {
        if (! captured_)
        {
            oldHarmony_ = project.harmony();
            captured_ = true;
        }
        for (auto& chord : newHarmony_.chords)
            if (chord.id == kInvalidId)
                chord.id = project.generateId();
        project.harmony() = newHarmony_;
        return true;
    }

    void undo(Project& project) override
    {
        if (! captured_)
            return;
        project.harmony() = oldHarmony_;
    }

private:
    HarmonyState newHarmony_;
    HarmonyState oldHarmony_;
    bool captured_ = false;
};

class ReplaceDrumPatternCommand final : public Command
{
public:
    ReplaceDrumPatternCommand(Id trackId, Id clipId, DrumPattern pattern)
        : trackId_(trackId), clipId_(clipId), newPattern_(std::move(pattern))
    {
    }

    [[nodiscard]] std::string name() const override { return "Replace Drum Pattern"; }

    bool perform(Project& project) override
    {
        auto* clip = project.findDrumClip(trackId_, clipId_);
        if (clip == nullptr)
            return false;

        if (! captured_)
        {
            oldPattern_ = clip->pattern;
            captured_ = true;
        }

        if (newPattern_.id == kInvalidId)
            newPattern_.id = project.generateId();
        clip->pattern = newPattern_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* clip = project.findDrumClip(trackId_, clipId_);
        if (clip == nullptr || ! captured_)
            return;
        clip->pattern = oldPattern_;
    }

private:
    Id trackId_;
    Id clipId_;
    DrumPattern newPattern_;
    DrumPattern oldPattern_;
    bool captured_ = false;
};

class ToggleDrumHitCommand final : public Command
{
public:
    ToggleDrumHitCommand(Id trackId, Id clipId, int step, DrumLane lane, float velocity = 0.8f)
        : trackId_(trackId), clipId_(clipId), step_(step), lane_(lane), velocity_(velocity)
    {
    }

    [[nodiscard]] std::string name() const override { return "Toggle Drum Hit"; }

    bool perform(Project& project) override
    {
        auto* clip = project.findDrumClip(trackId_, clipId_);
        if (clip == nullptr)
            return false;

        auto& hits = clip->pattern.hits;
        const auto it = std::find_if(hits.begin(), hits.end(), [&](const DrumHit& h) {
            return h.step == step_ && h.lane == lane_;
        });

        if (! captured_)
        {
            hadHit_ = it != hits.end();
            if (hadHit_)
                oldVelocity_ = it->velocity;
            captured_ = true;
        }

        if (it != hits.end())
            hits.erase(it);
        else
            hits.push_back({ step_, lane_, velocity_ });
        return true;
    }

    void undo(Project& project) override
    {
        auto* clip = project.findDrumClip(trackId_, clipId_);
        if (clip == nullptr || ! captured_)
            return;

        auto& hits = clip->pattern.hits;
        const auto it = std::find_if(hits.begin(), hits.end(), [&](const DrumHit& h) {
            return h.step == step_ && h.lane == lane_;
        });

        if (hadHit_)
        {
            if (it == hits.end())
                hits.push_back({ step_, lane_, oldVelocity_ });
        }
        else if (it != hits.end())
        {
            hits.erase(it);
        }
    }

private:
    Id trackId_;
    Id clipId_;
    int step_;
    DrumLane lane_;
    float velocity_;
    bool captured_ = false;
    bool hadHit_ = false;
    float oldVelocity_ = 0.8f;
};

} // namespace sensei::core
