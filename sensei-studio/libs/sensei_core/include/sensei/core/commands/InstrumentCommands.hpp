#pragma once

#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/commands/Command.hpp"

namespace sensei::core {

class SetTrackInstrumentCommand final : public Command
{
public:
    SetTrackInstrumentCommand(Id trackId, InstrumentId instrumentId)
        : trackId_(trackId), newId_(instrumentId)
    {
    }

    [[nodiscard]] std::string name() const override { return "Set Track Instrument"; }

    bool perform(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr || ! isValidInstrumentId(newId_))
            return false;

        // Drum tracks only accept drum kits; MIDI tracks only accept pitched instruments.
        const auto info = instrumentInfo(newId_);
        if (track->type == TrackType::Drums && ! info.isDrumKit)
            return false;
        if (track->type == TrackType::Midi && info.isDrumKit)
            return false;

        if (! captured_)
        {
            oldId_ = track->instrumentId;
            captured_ = true;
        }
        track->instrumentId = newId_;
        return true;
    }

    void undo(Project& project) override
    {
        auto* track = project.findTrack(trackId_);
        if (track == nullptr || ! captured_)
            return;
        track->instrumentId = oldId_;
    }

private:
    Id trackId_;
    InstrumentId newId_;
    InstrumentId oldId_ = InstrumentId::WarmKeys;
    bool captured_ = false;
};

} // namespace sensei::core
