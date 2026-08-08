#pragma once

#include "sensei/core/Types.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>

namespace sensei::core {

// Core-owned transport. Public getters/setters are safe for UI (message thread)
// and audio-thread readers via atomics. No JUCE types here.
class Transport
{
public:
    Transport() = default;

    void play() noexcept { playing_.store(true, std::memory_order_release); }

    void stop() noexcept
    {
        playing_.store(false, std::memory_order_release);
        positionSeconds_.store(0.0, std::memory_order_release);
    }

    [[nodiscard]] bool isPlaying() const noexcept
    {
        return playing_.load(std::memory_order_acquire);
    }

    void setBpm(double bpm) noexcept
    {
        bpm_.store(clampBpm(bpm), std::memory_order_release);
    }

    [[nodiscard]] double bpm() const noexcept
    {
        return bpm_.load(std::memory_order_acquire);
    }

    void resetPosition() noexcept
    {
        positionSeconds_.store(0.0, std::memory_order_release);
    }

    // Called from the audio thread while playing. Realtime-safe.
    void advance(double deltaSeconds) noexcept
    {
        if (! isPlaying() || deltaSeconds <= 0.0)
            return;

        const auto current = positionSeconds_.load(std::memory_order_relaxed);
        positionSeconds_.store(current + deltaSeconds, std::memory_order_relaxed);
    }

    [[nodiscard]] double positionSeconds() const noexcept
    {
        return positionSeconds_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double positionBeats() const noexcept
    {
        const auto seconds = positionSeconds();
        const auto beatsPerSecond = bpm() / 60.0;
        return seconds * beatsPerSecond;
    }

    static double clampBpm(double bpm) noexcept
    {
        if (! std::isfinite(bpm))
            return kDefaultBpm;
        return std::clamp(bpm, kMinBpm, kMaxBpm);
    }

private:
    std::atomic<bool> playing_ { false };
    std::atomic<double> bpm_ { kDefaultBpm };
    std::atomic<double> positionSeconds_ { 0.0 };
};

} // namespace sensei::core
