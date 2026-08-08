#pragma once

#include "sensei/core/Id.hpp"

#include <array>
#include <string>
#include <vector>

namespace sensei::core {

enum class ScaleMode
{
    Major,
    Minor
};

struct ChordEvent
{
    Id id = kInvalidId;
    double startBeat = 0.0;
    double lengthBeats = 4.0;
    std::string roman;      // e.g. "I", "vi"
    std::string chordName;  // e.g. "C", "Am"
    std::array<int, 3> pitches { 60, 64, 67 }; // triad MIDI

    [[nodiscard]] double endBeat() const noexcept { return startBeat + lengthBeats; }
};

struct HarmonyState
{
    int rootPitchClass = 0; // C = 0
    ScaleMode mode = ScaleMode::Major;
    std::string progressionId;
    std::vector<ChordEvent> chords;
};

inline const char* pitchClassName(int pc) noexcept
{
    static constexpr const char* names[] {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    return names[((pc % 12) + 12) % 12];
}

} // namespace sensei::core
