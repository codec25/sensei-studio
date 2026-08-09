#pragma once

#include "sensei/engine/Instrument.hpp"

#include <array>
#include <cmath>

namespace sensei::engine {

// Soft sustained keys/pad — darker, slower attack, longer release than bass/pluck.
class WarmKeys final : public Instrument
{
public:
    [[nodiscard]] sensei::core::InstrumentId id() const noexcept override
    {
        return sensei::core::InstrumentId::WarmKeys;
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
        const float vel = clamp01(velocity);
        Voice* voice = findVoiceForNote(midiNote);
        if (voice == nullptr)
            voice = findFreeOrSteal();
        voice->active = true;
        voice->midi = midiNote;
        voice->amp = 0.0f;
        voice->target = 0.18f * vel;
        voice->phase1 = 0.0;
        voice->phase2 = 0.0;
        voice->inc1 = midiToHz(midiNote) / sampleRate_;
        voice->inc2 = voice->inc1 * 2.0; // soft octave blend
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
                // Soft attack, long release.
                const float coeff = v.releasing ? 0.0012f : 0.006f;
                v.amp += coeff * (v.target - v.amp);
                const float sine = static_cast<float>(std::sin(v.phase1 * kTwoPi));
                const float tri = triangle(v.phase2);
                // Darker: more fundamental, less brightness.
                sample += (sine * 0.72f + tri * 0.18f) * v.amp;
                v.phase1 += v.inc1;
                v.phase2 += v.inc2;
                if (v.phase1 >= 1.0) v.phase1 -= 1.0;
                if (v.phase2 >= 1.0) v.phase2 -= 1.0;
                if (v.releasing && v.amp < 0.00008f)
                    v = {};
            }
            // Gentle one-pole lowpass feel via soft clip of highs (simple mix).
            left[i] += sample * 0.9f;
            if (right != nullptr)
                right[i] += sample * 0.9f;
        }
    }

private:
    static constexpr double kTwoPi = 6.28318530717958647692;
    static constexpr int kMaxVoices = 12;

    struct Voice
    {
        bool active = false;
        bool releasing = false;
        int midi = -1;
        float amp = 0.0f;
        float target = 0.0f;
        double phase1 = 0.0;
        double phase2 = 0.0;
        double inc1 = 0.0;
        double inc2 = 0.0;
    };

    static float clamp01(float v) noexcept
    {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    static double midiToHz(int midi) noexcept
    {
        return 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
    }

    static float triangle(double phase) noexcept
    {
        const float t = static_cast<float>(phase);
        return t < 0.5f ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
    }

    Voice* findVoiceForNote(int midi) noexcept
    {
        for (auto& v : voices_)
            if (v.active && v.midi == midi)
                return &v;
        return nullptr;
    }

    Voice* findFreeOrSteal() noexcept
    {
        for (auto& v : voices_)
            if (! v.active)
                return &v;
        Voice* quietest = &voices_[0];
        for (auto& v : voices_)
            if (v.amp < quietest->amp)
                quietest = &v;
        return quietest;
    }

    double sampleRate_ = 44100.0;
    std::array<Voice, kMaxVoices> voices_ {};
};

} // namespace sensei::engine
