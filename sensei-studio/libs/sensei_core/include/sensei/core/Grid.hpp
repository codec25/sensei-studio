#pragma once

#include <algorithm>
#include <cmath>

namespace sensei::core {

inline constexpr double kDefaultGridBeats = 0.25; // 1/16 note in 4/4
inline constexpr double kMinNoteLengthBeats = 0.25;

inline double snapBeat(double beat, double gridBeats = kDefaultGridBeats) noexcept
{
    if (! std::isfinite(beat) || gridBeats <= 0.0)
        return 0.0;

    return std::round(beat / gridBeats) * gridBeats;
}

inline double snapLength(double lengthBeats, double gridBeats = kDefaultGridBeats) noexcept
{
    if (! std::isfinite(lengthBeats))
        return kMinNoteLengthBeats;

    const double snapped = std::round(lengthBeats / gridBeats) * gridBeats;
    return std::max(gridBeats, snapped);
}

inline double clampNonNegativeBeat(double beat) noexcept
{
    if (! std::isfinite(beat) || beat < 0.0)
        return 0.0;
    return beat;
}

} // namespace sensei::core
