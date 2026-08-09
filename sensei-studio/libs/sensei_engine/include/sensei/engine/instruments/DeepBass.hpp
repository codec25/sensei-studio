#pragma once

#include "sensei/engine/Instrument.hpp"

#include <array>
#include <cmath>

namespace sensei::engine {

// Dedicated bass voice — fast attack, tight body, harmonic weight in the low end.
class DeepBass final : public Instrument
{
public:
    [[nodiscard]] sensei::core::InstrumentId id() const noexcept override
    {
        return sensei::core::InstrumentId::DeepBass;
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
        // Prefer lower register character: steal to mono-ish lowest voice.
        Voice* voice = &voices_[0];
        for (auto& v : voices_)
        {
            if (! v.active)
            {
                voice = &v;
                break;
            }
        }
        // Retrigger primary voice for bass focus.
        if (voices_[0].active)
            voices_[0] = {};
        voice = &voices_[0];

        const float vel = velocity < 0.0f ? 0.0f : (velocity > 1.0f ? 1.0f : velocity);
        voice->active = true;
        voice->midi = midiNote;
        voice->amp = 0.0f;
        voice->target = 0.32f * vel;
        voice->phase = 0.0;
        voice->phase2 = 0.0;
        voice->inc = midiToHz(midiNote) / sampleRate_;
        voice->inc2 = voice->inc * 2.0;
        voice->releasing = false;
    }

    void noteOff(int midiNote) noexcept override
    {
        for (auto& v : voices_)
            if (v.active && v.midi == midiNote && ! v.releasing)
            {
                v.releasing = true;
                v.target = 0.0f;
            }
    }

    void allNotesOff() noexcept override
    {
        // Hard clear — used on stop / instrument switch to avoid stuck notes.
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
                // Fast attack, tighter release than Warm Keys.
                const float coeff = v.releasing ? 0.0045f : 0.05f;
                v.amp += coeff * (v.target - v.amp);
                const float fund = static_cast<float>(std::sin(v.phase * kTwoPi));
                const float harm = static_cast<float>(std::sin(v.phase2 * kTwoPi));
                // Soft fold for a little grit without becoming a lead.
                float tone = fund * 0.85f + harm * 0.22f;
                tone = softClip(tone * 1.15f);
                sample += tone * v.amp;
                v.phase += v.inc;
                v.phase2 += v.inc2;
                if (v.phase >= 1.0) v.phase -= 1.0;
                if (v.phase2 >= 1.0) v.phase2 -= 1.0;
                if (v.releasing && v.amp < 0.0001f)
                    v = {};
            }
            left[i] += sample;
            if (right != nullptr)
                right[i] += sample;
        }
    }

private:
    static constexpr double kTwoPi = 6.28318530717958647692;
    static constexpr int kMaxVoices = 4;

    struct Voice
    {
        bool active = false;
        bool releasing = false;
        int midi = -1;
        float amp = 0.0f;
        float target = 0.0f;
        double phase = 0.0;
        double phase2 = 0.0;
        double inc = 0.0;
        double inc2 = 0.0;
    };

    static double midiToHz(int midi) noexcept
    {
        return 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
    }

    static float softClip(float x) noexcept
    {
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return x - (x * x * x) * 0.15f;
    }

    double sampleRate_ = 44100.0;
    std::array<Voice, kMaxVoices> voices_ {};
};

} // namespace sensei::engine
