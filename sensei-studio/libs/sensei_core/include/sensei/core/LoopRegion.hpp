#pragma once

namespace sensei::core {

struct LoopRegion
{
    double startBeat = 0.0;
    double lengthBeats = 16.0; // 4 bars of 4/4
    bool enabled = true;
};

} // namespace sensei::core
