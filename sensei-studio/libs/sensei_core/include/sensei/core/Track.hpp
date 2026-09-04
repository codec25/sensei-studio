#pragma once

#include "sensei/core/AudioClip.hpp"
#include "sensei/core/Id.hpp"
#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/MidiClip.hpp"
#include "sensei/core/Types.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace sensei::core {

enum class TrackType
{
    Midi,
    Drums,
    Audio
};

enum class TrackRole
{
    Chords,
    Bass,
    Melody,
    Drums
};

enum class DrumLane : std::uint8_t
{
    Kick = 0,
    Snare = 1,
    ClosedHat = 2
};

inline constexpr int kDrumLaneCount = 3;
inline constexpr int kStepsPerBar = 16;
inline constexpr int kDefaultDrumSteps = kStepsPerBar * kDefaultBars; // 64

struct DrumHit
{
    int step = 0;
    DrumLane lane = DrumLane::Kick;
    float velocity = 0.8f;
};

struct DrumPattern
{
    Id id = kInvalidId;
    int stepCount = kDefaultDrumSteps;
    std::vector<DrumHit> hits;

    [[nodiscard]] bool hasHit(int step, DrumLane lane) const noexcept
    {
        for (const auto& hit : hits)
            if (hit.step == step && hit.lane == lane)
                return true;
        return false;
    }
};

// Placeable drum clip on the arrangement timeline. Pattern steps map to
// this clip's lengthBeats (not the whole song).
struct DrumClip
{
    Id id = kInvalidId;
    std::string name;
    double startBeat = 0.0;
    double lengthBeats = kDefaultLoopBeats;
    DrumPattern pattern;
};

// Maps drum sequencer steps onto a clip (or loop) length. Invalid/zero
// stepCount falls back to kDefaultDrumSteps so playback stays deterministic.
[[nodiscard]] inline double drumBeatPerStep(double lengthBeats, int stepCount) noexcept
{
    const int steps = stepCount > 0 ? stepCount : kDefaultDrumSteps;
    if (! (lengthBeats > 0.0))
        lengthBeats = kDefaultLoopBeats;
    return lengthBeats / static_cast<double>(steps);
}

[[nodiscard]] inline double drumStepToBeat(int step, double lengthBeats, int stepCount) noexcept
{
    return static_cast<double>(step) * drumBeatPerStep(lengthBeats, stepCount);
}

[[nodiscard]] inline constexpr InstrumentId defaultInstrumentForRole(TrackRole role) noexcept
{
    switch (role)
    {
        case TrackRole::Bass: return InstrumentId::DeepBass;
        case TrackRole::Melody: return InstrumentId::BrightPluck;
        case TrackRole::Drums: return InstrumentId::StudioKitBasic;
        case TrackRole::Chords:
        default: return InstrumentId::WarmKeys;
    }
}

// Shared mixer state. Reverb is modelled as a send, not baked into clips, so
// several tracks can feed one return effect like professional DAWs do.
struct TrackMixState
{
    double gainDb = 0.0;
    double pan = 0.0;          // -1 left ... +1 right
    double reverbSend01 = 0.0; // 0 dry send ... 1 full send

    void sanitize() noexcept
    {
        gainDb = std::clamp(gainDb, -96.0, 24.0);
        pan = std::clamp(pan, -1.0, 1.0);
        reverbSend01 = std::clamp(reverbSend01, 0.0, 1.0);
    }
};

struct Track
{
    Id id = kInvalidId;
    std::string name;
    TrackType type = TrackType::Midi;
    TrackRole role = TrackRole::Melody;
    // Stable instrument/preset identity (Core metadata only — Engine owns DSP).
    InstrumentId instrumentId = InstrumentId::WarmKeys;
    // Playback routing — mute always silences; solo (when any solo active) gates others.
    bool muted = false;
    bool solo = false;
    TrackMixState mix {};
    std::vector<MidiClip> clips;
    std::vector<DrumClip> drumClips;
    std::vector<AudioClip> audioClips;
    // True while track content still originates from a Sensei-generated action.
    bool generatedOrigin = false;
};

[[nodiscard]] inline bool projectHasSolo(const std::vector<Track>& tracks) noexcept
{
    for (const auto& track : tracks)
        if (track.solo)
            return true;
    return false;
}

// Deterministic audible gate used by snapshot publication (and tests).
[[nodiscard]] inline bool isTrackAudible(const Track& track, bool anySolo) noexcept
{
    if (track.muted)
        return false;
    if (anySolo)
        return track.solo;
    return true;
}

} // namespace sensei::core
