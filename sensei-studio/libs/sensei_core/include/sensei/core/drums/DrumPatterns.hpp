#pragma once

#include "sensei/core/Track.hpp"

namespace sensei::core {

inline void addHit(DrumPattern& pattern, int step, DrumLane lane, float velocity = 0.8f)
{
    if (step < 0 || step >= pattern.stepCount)
        return;
    if (pattern.hasHit(step, lane))
        return;
    pattern.hits.push_back({ step, lane, velocity });
}

[[nodiscard]] inline DrumPattern makeBasicRockPattern()
{
    DrumPattern p;
    p.stepCount = kDefaultDrumSteps;
    for (int bar = 0; bar < kDefaultBars; ++bar)
    {
        const int base = bar * kStepsPerBar;
        addHit(p, base + 0, DrumLane::Kick, 0.9f);
        addHit(p, base + 8, DrumLane::Kick, 0.85f);
        addHit(p, base + 4, DrumLane::Snare, 0.9f);
        addHit(p, base + 12, DrumLane::Snare, 0.9f);
        for (int s = 0; s < kStepsPerBar; s += 2)
            addHit(p, base + s, DrumLane::ClosedHat, (s % 4 == 0) ? 0.7f : 0.45f);
    }
    return p;
}

[[nodiscard]] inline DrumPattern makeLightPopPattern()
{
    DrumPattern p;
    p.stepCount = kDefaultDrumSteps;
    for (int bar = 0; bar < kDefaultBars; ++bar)
    {
        const int base = bar * kStepsPerBar;
        addHit(p, base + 0, DrumLane::Kick, 0.85f);
        addHit(p, base + 7, DrumLane::Kick, 0.7f);
        addHit(p, base + 8, DrumLane::Kick, 0.8f);
        addHit(p, base + 4, DrumLane::Snare, 0.85f);
        addHit(p, base + 12, DrumLane::Snare, 0.85f);
        for (int s = 0; s < kStepsPerBar; ++s)
            addHit(p, base + s, DrumLane::ClosedHat, 0.35f);
    }
    return p;
}

[[nodiscard]] inline DrumPattern makeFourOnFloorPattern()
{
    DrumPattern p;
    p.stepCount = kDefaultDrumSteps;
    for (int step = 0; step < p.stepCount; ++step)
    {
        if (step % 4 == 0)
            addHit(p, step, DrumLane::Kick, 0.9f);
        if (step % 8 == 4)
            addHit(p, step, DrumLane::Snare, 0.8f);
        if (step % 2 == 0)
            addHit(p, step, DrumLane::ClosedHat, 0.5f);
    }
    return p;
}

} // namespace sensei::core
