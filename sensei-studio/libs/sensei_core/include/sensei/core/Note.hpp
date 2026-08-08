#pragma once

#include "sensei/core/Types.hpp"

#include <algorithm>
#include <cmath>

namespace sensei::core {

struct Note
{
    int midiNumber = kMiddleCMidi;
    float velocity = 0.8f;
};

inline double midiToFrequency(int midiNumber) noexcept
{
    return 440.0 * std::pow(2.0, (static_cast<double>(midiNumber) - 69.0) / 12.0);
}

inline float clampVelocity(float velocity) noexcept
{
    return std::clamp(velocity, 0.0f, 1.0f);
}

inline int clampMidiNote(int midiNumber) noexcept
{
    return std::clamp(midiNumber, 0, 127);
}

} // namespace sensei::core
