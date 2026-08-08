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

// Maps drum sequencer steps onto the project loop. Invalid/zero stepCount falls
// back to kDefaultDrumSteps so playback stays deterministic.
[[nodiscard]] inline double drumBeatPerStep(double loopLengthBeats, int stepCount) noexcept
{
    const int steps = stepCount > 0 ? stepCount : kDefaultDrumSteps;
    if (! (loopLengthBeats > 0.0))
        loopLengthBeats = kDefaultLoopBeats;
    return loopLengthBeats / static_cast<double>(steps);
}

[[nodiscard]] inline double drumStepToBeat(int step, double loopLengthBeats, int stepCount) noexcept
{
    return static_cast<double>(step) * drumBeatPerStep(loopLengthBeats, stepCount);
}

struct Track
{
    Id id = kInvalidId;
    std::string name;
    TrackType type = TrackType::Midi;
    TrackRole role = TrackRole::Melody;
    std::vector<MidiClip> clips;
    DrumPattern drumPattern;
    // True while track content still originates from a Sensei-generated action.
    bool generatedOrigin = false;
};

} // namespace sensei::core
