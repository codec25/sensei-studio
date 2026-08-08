#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/MidiNote.hpp"

#include <string>
#include <vector>

namespace sensei::core {

struct MidiClip
{
    Id id = kInvalidId;
    std::string name;
    double startBeat = 0.0;
    double lengthBeats = 16.0;
    std::vector<MidiNote> notes;
};

} // namespace sensei::core
