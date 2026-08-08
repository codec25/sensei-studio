#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/Types.hpp"

namespace sensei::core {

struct MidiNote
{
    Id id = kInvalidId;
    int pitch = kMiddleCMidi;
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    float velocity = 0.8f;

    [[nodiscard]] double endBeat() const noexcept { return startBeat + lengthBeats; }
};

} // namespace sensei::core
