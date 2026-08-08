#pragma once

namespace sensei::core {

inline constexpr double kDefaultBpm = 94.0;
inline constexpr double kMinBpm = 40.0;
inline constexpr double kMaxBpm = 240.0;

inline constexpr int kMiddleCMidi = 60;
inline constexpr int kBeatsPerBar = 4;
inline constexpr int kDefaultBars = 4;
inline constexpr double kDefaultLoopBeats = static_cast<double>(kBeatsPerBar * kDefaultBars);

} // namespace sensei::core
