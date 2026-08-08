#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/LoopRegion.hpp"
#include "sensei/core/Track.hpp"
#include "sensei/core/Types.hpp"
#include "sensei/core/harmony/Chord.hpp"

#include <string>
#include <vector>

namespace sensei::core {

class Project
{
public:
    [[nodiscard]] static Project createStarter(std::string name = "My First Sensei Project");

    [[nodiscard]] Id id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void setName(std::string name) { name_ = std::move(name); }

    [[nodiscard]] const LoopRegion& loop() const noexcept { return loop_; }
    [[nodiscard]] LoopRegion& loop() noexcept { return loop_; }

    [[nodiscard]] const HarmonyState& harmony() const noexcept { return harmony_; }
    [[nodiscard]] HarmonyState& harmony() noexcept { return harmony_; }

    [[nodiscard]] const std::vector<Track>& tracks() const noexcept { return tracks_; }
    [[nodiscard]] std::vector<Track>& tracks() noexcept { return tracks_; }

    [[nodiscard]] Id generateId() noexcept { return nextId_++; }
    [[nodiscard]] Id peekNextId() const noexcept { return nextId_; }

    [[nodiscard]] Track* findTrack(Id trackId) noexcept;
    [[nodiscard]] const Track* findTrack(Id trackId) const noexcept;
    [[nodiscard]] Track* findTrackByRole(TrackRole role) noexcept;
    [[nodiscard]] const Track* findTrackByRole(TrackRole role) const noexcept;
    [[nodiscard]] MidiClip* findClip(Id trackId, Id clipId) noexcept;
    [[nodiscard]] const MidiClip* findClip(Id trackId, Id clipId) const noexcept;
    [[nodiscard]] MidiNote* findNote(Id trackId, Id clipId, Id noteId) noexcept;
    [[nodiscard]] const MidiNote* findNote(Id trackId, Id clipId, Id noteId) const noexcept;

    // Backward-compatible helpers: primary pitched track is Chords.
    [[nodiscard]] Track* primaryMidiTrack() noexcept;
    [[nodiscard]] const Track* primaryMidiTrack() const noexcept;
    [[nodiscard]] MidiClip* primaryClip() noexcept;
    [[nodiscard]] const MidiClip* primaryClip() const noexcept;

    [[nodiscard]] std::size_t totalNoteCount() const noexcept;
    [[nodiscard]] std::size_t totalDrumHitCount() const noexcept;

private:
    Id id_ = kInvalidId;
    std::string name_;
    LoopRegion loop_ {};
    HarmonyState harmony_ {};
    std::vector<Track> tracks_;
    Id nextId_ = 1;
};

} // namespace sensei::core
