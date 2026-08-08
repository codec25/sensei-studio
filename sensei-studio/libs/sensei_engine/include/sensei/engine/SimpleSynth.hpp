#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace sensei::engine {

// Deliberately temporary built-in voice for Milestone A.
// Realtime-safe: no heap allocation in prepare/process/noteOn/noteOff.
class SimpleSynth
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (auto& voice : voices_)
            voice = {};
    }

    void noteOn(int midiNote, float velocity) noexcept
    {
        if (midiNote < 0 || midiNote > 127)
            return;

        const float vel = velocity < 0.0f ? 0.0f : (velocity > 1.0f ? 1.0f : velocity);
        Voice* voice = findVoiceForNote(midiNote);
        if (voice == nullptr)
            voice = findFreeOrStealVoice();

        voice->active = true;
        voice->midiNote = midiNote;
        voice->amplitude = 0.0f;
        voice->targetAmplitude = 0.2f * vel;
        voice->phase = 0.0;
        voice->phaseIncrement = midiToFrequency(midiNote) / sampleRate_;
        voice->releasing = false;
    }

    void noteOff(int midiNote) noexcept
    {
        for (auto& voice : voices_)
        {
            if (voice.active && voice.midiNote == midiNote && ! voice.releasing)
            {
                voice.releasing = true;
                voice.targetAmplitude = 0.0f;
            }
        }
    }

    void allNotesOff() noexcept
    {
        for (auto& voice : voices_)
        {
            if (voice.active)
            {
                voice.releasing = true;
                voice.targetAmplitude = 0.0f;
            }
        }
    }

    void process(float* left, float* right, int numSamples) noexcept
    {
        if (left == nullptr || numSamples <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;

            for (auto& voice : voices_)
            {
                if (! voice.active)
                    continue;

                // Simple one-pole amplitude slew (attack/release).
                const float coeff = voice.releasing ? 0.0025f : 0.015f;
                voice.amplitude += coeff * (voice.targetAmplitude - voice.amplitude);

                sample += static_cast<float>(std::sin(voice.phase * kTwoPi)) * voice.amplitude;
                voice.phase += voice.phaseIncrement;
                if (voice.phase >= 1.0)
                    voice.phase -= 1.0;

                if (voice.releasing && voice.amplitude < 0.0001f)
                    voice = {};
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
        bool releasing = false;
        int midiNote = -1;
        float amplitude = 0.0f;
        float targetAmplitude = 0.0f;
        double phase = 0.0;
        double phaseIncrement = 0.0;
    };

    static constexpr double kTwoPi = 6.283185307179586476925286766559;

    static double midiToFrequency(int midiNote) noexcept
    {
        return 440.0 * std::pow(2.0, (static_cast<double>(midiNote) - 69.0) / 12.0);
    }

    Voice* findVoiceForNote(int midiNote) noexcept
    {
        for (auto& voice : voices_)
        {
            if (voice.active && voice.midiNote == midiNote)
                return &voice;
        }
        return nullptr;
    }

    Voice* findFreeOrStealVoice() noexcept
    {
        for (auto& voice : voices_)
        {
            if (! voice.active)
                return &voice;
        }

        // Steal the quietest releasing voice, else voice 0.
        Voice* quietest = &voices_[0];
        for (auto& voice : voices_)
        {
            if (voice.releasing && voice.amplitude < quietest->amplitude)
                quietest = &voice;
        }
        return quietest;
    }

    double sampleRate_ = 44100.0;
    std::array<Voice, kMaxVoices> voices_ {};
};

} // namespace sensei::engine
