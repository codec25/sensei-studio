#pragma once

#include "sensei/core/SequenceSnapshot.hpp"

#include <array>
#include <cmath>

namespace sensei::engine {

// Temporary synthesized kick/snare/hat. Realtime-safe, no heap in process/trigger.
class SimpleDrumEngine
{
public:
    static constexpr int kMaxVoices = 16;

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (auto& v : voices_)
            v = {};
    }

    void trigger(sensei::core::DrumProgram program, float velocity) noexcept
    {
        const float vel = velocity < 0.0f ? 0.0f : (velocity > 1.0f ? 1.0f : velocity);
        Voice* voice = nullptr;
        for (auto& v : voices_)
        {
            if (! v.active)
            {
                voice = &v;
                break;
            }
        }
        if (voice == nullptr)
            voice = &voices_[0];

        voice->active = true;
        voice->program = program;
        voice->amp = 0.0f;
        voice->target = (program == sensei::core::DrumProgram::ClosedHat ? 0.12f : 0.35f) * vel;
        voice->age = 0.0;
        voice->phase = 0.0;
        voice->freq = program == sensei::core::DrumProgram::Kick ? 120.0 : 180.0;
    }

    void allOff() noexcept
    {
        for (auto& v : voices_)
            v = {};
    }

    void process(float* left, float* right, int numSamples) noexcept
    {
        if (left == nullptr || numSamples <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;
            for (auto& v : voices_)
            {
                if (! v.active)
                    continue;

                v.age += 1.0 / sampleRate_;
                v.amp += 0.05f * (v.target - v.amp);

                if (v.program == sensei::core::DrumProgram::Kick)
                {
                    v.freq = 120.0 * std::pow(0.15, v.age * 12.0) + 40.0;
                    sample += static_cast<float>(std::sin(v.phase * kTwoPi)) * v.amp;
                    v.phase += v.freq / sampleRate_;
                    v.target *= 0.9992f;
                    if (v.age > 0.25)
                        v.active = false;
                }
                else if (v.program == sensei::core::DrumProgram::Snare)
                {
                    const float noise = pseudoNoise(v.age) * 0.7f;
                    sample += (static_cast<float>(std::sin(v.phase * kTwoPi)) * 0.2f + noise) * v.amp;
                    v.phase += 180.0 / sampleRate_;
                    v.target *= 0.9985f;
                    if (v.age > 0.18)
                        v.active = false;
                }
                else
                {
                    sample += pseudoNoise(v.age) * v.amp;
                    v.target *= 0.985f;
                    if (v.age > 0.05)
                        v.active = false;
                }

                if (v.amp < 0.0001f && v.target < 0.0001f)
                    v.active = false;
            }

            left[i] += sample;
            if (right != nullptr)
                right[i] += sample;
        }
    }

private:
    struct Voice
    {
        bool active = false;
        sensei::core::DrumProgram program = sensei::core::DrumProgram::Kick;
        float amp = 0.0f;
        float target = 0.0f;
        double age = 0.0;
        double phase = 0.0;
        double freq = 100.0;
    };

    static constexpr double kTwoPi = 6.28318530717958647692;

    static float pseudoNoise(double t) noexcept
    {
        const auto x = static_cast<unsigned int>(t * 100000.0);
        return (static_cast<float>((x * 1103515245u + 12345u) & 0x7fffffff) / 2147483647.0f) * 2.0f - 1.0f;
    }

    double sampleRate_ = 44100.0;
    std::array<Voice, kMaxVoices> voices_ {};
};

} // namespace sensei::engine
