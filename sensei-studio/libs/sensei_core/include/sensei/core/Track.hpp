#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/MidiClip.hpp"
#include "sensei/core/Types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sensei::core {

enum class TrackType
{
    Midi,
    Drums
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

struct Track
{
    Id id = kInvalidId;
    std::string name;
    TrackType type = TrackType::Midi;
    TrackRole role = TrackRole::Melody;
    std::vector<MidiClip> clips;
    std::vector<DrumClip> drumClips;
    // True while track content still originates from a Sensei-generated action.
    bool generatedOrigin = false;
};

} // namespace sensei::core
