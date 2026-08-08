#pragma once

#include "sensei/core/Types.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>

namespace sensei::core {

// Core-owned transport. Public getters/setters are safe for UI (message thread)
// and audio-thread readers via atomics. No JUCE types here.
// Musical position is stored in beats; advance() converts seconds using BPM
// and wraps into the loop region when looping is enabled.
class Transport
{
public:
    Transport() = default;

    void play() noexcept { playing_.store(true, std::memory_order_release); }

    void stop() noexcept
    {
        playing_.store(false, std::memory_order_release);
        positionBeats_.store(0.0, std::memory_order_release);
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

    void setLoop(double startBeat, double lengthBeats, bool enabled) noexcept
    {
        loopStartBeats_.store(std::max(0.0, startBeat), std::memory_order_release);
        loopLengthBeats_.store(lengthBeats > 0.0 ? lengthBeats : kDefaultLoopBeats,
                               std::memory_order_release);
        loopEnabled_.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool loopEnabled() const noexcept
    {
        return loopEnabled_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double loopStartBeats() const noexcept
    {
        return loopStartBeats_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double loopLengthBeats() const noexcept
    {
        return loopLengthBeats_.load(std::memory_order_acquire);
    }

    void resetPosition() noexcept
    {
        positionBeats_.store(0.0, std::memory_order_release);
    }

    // Called from the audio thread while playing. Realtime-safe.
    void advance(double deltaSeconds) noexcept
    {
        if (! isPlaying() || deltaSeconds <= 0.0)
            return;

        const double bpmValue = bpm();
        if (bpmValue <= 0.0)
            return;

        double beats = positionBeats_.load(std::memory_order_relaxed)
                       + deltaSeconds * (bpmValue / 60.0);
        beats = wrapBeats(beats);
        positionBeats_.store(beats, std::memory_order_relaxed);
    }

    [[nodiscard]] double positionBeats() const noexcept
    {
        return positionBeats_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double positionSeconds() const noexcept
    {
        const double bpmValue = bpm();
        if (bpmValue <= 0.0)
            return 0.0;
        return positionBeats() * (60.0 / bpmValue);
    }

    static double clampBpm(double bpm) noexcept
    {
        if (! std::isfinite(bpm))
            return kDefaultBpm;
        return std::clamp(bpm, kMinBpm, kMaxBpm);
    }

private:
    [[nodiscard]] double wrapBeats(double beats) const noexcept
    {
        if (! loopEnabled_.load(std::memory_order_relaxed))
            return beats;

        const double start = loopStartBeats_.load(std::memory_order_relaxed);
        const double length = loopLengthBeats_.load(std::memory_order_relaxed);
        if (length <= 0.0)
            return beats;

        double relative = beats - start;
        relative = std::fmod(relative, length);
        if (relative < 0.0)
            relative += length;
        return start + relative;
    }

    std::atomic<bool> playing_ { false };
    std::atomic<double> bpm_ { kDefaultBpm };
    std::atomic<double> positionBeats_ { 0.0 };
    std::atomic<bool> loopEnabled_ { true };
    std::atomic<double> loopStartBeats_ { 0.0 };
    std::atomic<double> loopLengthBeats_ { kDefaultLoopBeats };
};

} // namespace sensei::core
