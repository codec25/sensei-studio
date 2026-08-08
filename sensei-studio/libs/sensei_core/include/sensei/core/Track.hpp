#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/MidiClip.hpp"

#include <string>
#include <vector>

namespace sensei::core {

enum class TrackType
{
    Midi
};

struct Track
{
    Id id = kInvalidId;
    std::string name;
    TrackType type = TrackType::Midi;
    std::vector<MidiClip> clips;
};

} // namespace sensei::core
