#pragma once

#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/SequenceSnapshot.hpp"

#include <array>
#include <cmath>

namespace sensei::engine {

// Distinct kick / snare-clap / closed-hat kit. Pad table ready for more voices later.
class StudioDrumKit
{
public:
    [[nodiscard]] sensei::core::InstrumentId id() const noexcept
    {
        return sensei::core::InstrumentId::StudioKitBasic;
    }

    void prepare(double sampleRate, int) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (auto& v : voices_)
            v = {};
    }

    void trigger(sensei::core::DrumProgram pad, float velocity) noexcept
    {
        const float vel = velocity < 0.0f ? 0.0f : (velocity > 1.0f ? 1.0f : velocity);
        Voice* voice = nullptr;
        for (auto& v : voices_)
            if (! v.active)
            {
                voice = &v;
                break;
            }
        if (voice == nullptr)
            voice = &voices_[0];

        voice->active = true;
        voice->pad = pad;
        voice->amp = 0.0f;
        voice->age = 0.0;
        voice->phase = 0.0;
        voice->noise = 0.1234567f;

        switch (pad)
        {
            case sensei::core::DrumProgram::Kick:
                voice->target = 0.55f * vel;
                voice->freq = 140.0;
                break;
            case sensei::core::DrumProgram::Snare:
                voice->target = 0.42f * vel;
                voice->freq = 220.0;
                break;
            case sensei::core::DrumProgram::ClosedHat:
            default:
                voice->target = 0.16f * vel;
                voice->freq = 0.0;
                break;
        }
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
                v.amp += 0.08f * (v.target - v.amp);

                if (v.pad == sensei::core::DrumProgram::Kick)
                {
                    // Punchy pitch sweep + click.
                    v.freq = 140.0 * std::pow(0.12, v.age * 14.0) + 38.0;
                    const float body = static_cast<float>(std::sin(v.phase * kTwoPi));
                    const float click = (v.age < 0.004) ? 0.35f : 0.0f;
                    sample += (body + click) * v.amp;
                    v.phase += v.freq / sampleRate_;
                    v.target *= 0.9991f;
                    if (v.age > 0.28)
                        v.active = false;
                }
                else if (v.pad == sensei::core::DrumProgram::Snare)
                {
                    // Body tone + clap-ish noise burst.
                    const float body = static_cast<float>(std::sin(v.phase * kTwoPi)) * 0.25f;
                    const float noise = nextNoise(v) * 0.85f;
                    sample += (body + noise) * v.amp;
                    v.phase += 210.0 / sampleRate_;
                    v.target *= 0.9978f;
                    if (v.age > 0.2)
                        v.active = false;
                }
                else
                {
                    // Closed hat: short bright HP-ish noise.
                    float n = nextNoise(v);
                    n = n - v.noiseLp;
                    v.noiseLp += 0.35f * (nextNoise(v) - v.noiseLp);
                    sample += n * v.amp;
                    v.target *= 0.96f;
                    if (v.age > 0.05)
                        v.active = false;
                }
            }
            left[i] += sample;
            if (right != nullptr)
                right[i] += sample;
        }
    }

private:
    static constexpr double kTwoPi = 6.28318530717958647692;
    static constexpr int kMaxVoices = 16;

    struct Voice
    {
        bool active = false;
        sensei::core::DrumProgram pad = sensei::core::DrumProgram::Kick;
        float amp = 0.0f;
        float target = 0.0f;
        double age = 0.0;
        double phase = 0.0;
        double freq = 0.0;
        float noise = 0.1f;
        float noiseLp = 0.0f;
    };

    static float nextNoise(Voice& v) noexcept
    {
        // xorshift-ish
        std::uint32_t x = static_cast<std::uint32_t>(v.noise * 100000.0f) + 1u;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        v.noise = static_cast<float>(x & 0xFFFF) / 65535.0f;
        return v.noise * 2.0f - 1.0f;
    }

    double sampleRate_ = 44100.0;
    std::array<Voice, kMaxVoices> voices_ {};
};

} // namespace sensei::engine
