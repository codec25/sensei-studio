#pragma once

#include "sensei/core/commands/Command.hpp"

namespace sensei::core {

class SetTrackMuteCommand final : public Command
{
public:
    SetTrackMuteCommand(Id trackId, bool muted)
        : trackId_(trackId), newMuted_(muted)
    {
    }

    [[nodiscard]] std::string name() const override { return "Set Track Mute"; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return false;
        if (! captured_)
        {
            oldMuted_ = track->muted;
            captured_ = true;
        }
        track->muted = newMuted_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr || ! captured_)
            return;
        track->muted = oldMuted_;
    }

private:
    Id trackId_;
    bool newMuted_ = false;
    bool oldMuted_ = false;
    bool captured_ = false;
};

class SetTrackSoloCommand final : public Command
{
public:
    SetTrackSoloCommand(Id trackId, bool solo)
        : trackId_(trackId), newSolo_(solo)
    {
    }

    [[nodiscard]] std::string name() const override { return "Set Track Solo"; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr)
            return false;
        if (! captured_)
        {
            oldSolo_ = track->solo;
            captured_ = true;
        }
        track->solo = newSolo_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr || ! captured_)
            return;
        track->solo = oldSolo_;
    }

private:
    Id trackId_;
    bool newSolo_ = false;
    bool oldSolo_ = false;
    bool captured_ = false;
};

} // namespace sensei::core
