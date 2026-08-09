#pragma once

#include "sensei/engine/Instrument.hpp"

#include <array>
#include <cmath>

namespace sensei::engine {

// Short bright pluck — fast decay, brighter harmonics, distinct transient.
class BrightPluck final : public Instrument
{
public:
    [[nodiscard]] sensei::core::InstrumentId id() const noexcept override
    {
        return sensei::core::InstrumentId::BrightPluck;
    }

    void prepare(double sampleRate, int) noexcept override
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (auto& v : voices_)
            v = {};
    }

    void noteOn(int midiNote, float velocity) noexcept override
    {
        if (midiNote < 0 || midiNote > 127)
            return;
        const float vel = velocity < 0.0f ? 0.0f : (velocity > 1.0f ? 1.0f : velocity);
        Voice* voice = nullptr;
        for (auto& v : voices_)
            if (! v.active)
            {
                voice = &v;
                break;
            }
        if (voice == nullptr)
            voice = &voices_[static_cast<std::size_t>(stealIndex_++ % kMaxVoices)];

        voice->active = true;
        voice->midi = midiNote;
        voice->amp = 0.28f * vel; // immediate pluck peak
        voice->phase = 0.0;
        voice->phase2 = 0.0;
        voice->inc = midiToHz(midiNote) / sampleRate_;
        voice->inc2 = voice->inc * 3.0; // brighter partial
        voice->age = 0.0;
    }

    void noteOff(int midiNote) noexcept override
    {
        // Pluck is decay-driven; noteOff accelerates release slightly.
        for (auto& v : voices_)
            if (v.active && v.midi == midiNote)
                v.age += 0.05;
    }

    void allNotesOff() noexcept override
    {
        for (auto& v : voices_)
            v = {};
    }

    void process(float* left, float* right, int numSamples) noexcept override
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
                // Exponential-ish decay — short sustain.
                v.amp *= 0.9992f;
                const float saw = sawtooth(v.phase);
                const float partial = static_cast<float>(std::sin(v.phase2 * kTwoPi));
                sample += (saw * 0.55f + partial * 0.35f) * v.amp;
                v.phase += v.inc;
                v.phase2 += v.inc2;
                if (v.phase >= 1.0) v.phase -= 1.0;
                if (v.phase2 >= 1.0) v.phase2 -= 1.0;
                if (v.amp < 0.00015f || v.age > 0.9)
                    v = {};
            }
            left[i] += sample;
            if (right != nullptr)
                right[i] += sample;
        }
    }

private:
    static constexpr double kTwoPi = 6.28318530717958647692;
    static constexpr int kMaxVoices = 10;

    struct Voice
    {
        bool active = false;
        int midi = -1;
        float amp = 0.0f;
        double phase = 0.0;
        double phase2 = 0.0;
        double inc = 0.0;
        double inc2 = 0.0;
        double age = 0.0;
    };

    static double midiToHz(int midi) noexcept
    {
        return 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
    }

    static float sawtooth(double phase) noexcept
    {
        return static_cast<float>(2.0 * phase - 1.0);
    }

    double sampleRate_ = 44100.0;
    int stealIndex_ = 0;
    std::array<Voice, kMaxVoices> voices_ {};
};

} // namespace sensei::engine
