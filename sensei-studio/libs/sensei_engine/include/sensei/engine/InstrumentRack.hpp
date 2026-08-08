#pragma once

#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/engine/SimpleDrumEngine.hpp"
#include "sensei/engine/SimpleSynth.hpp"

namespace sensei::engine {

// Temporary multi-program rack for Milestone C learning sounds.
class InstrumentRack
{
public:
    void prepare(double sampleRate) noexcept
    {
        chords_.prepare(sampleRate);
        bass_.prepare(sampleRate);
        melody_.prepare(sampleRate);
        drums_.prepare(sampleRate);
    }

    void noteOn(sensei::core::SoundProgram program, int midi, float velocity) noexcept
    {
        synthFor(program).noteOn(midi, velocity);
    }

    void noteOff(sensei::core::SoundProgram program, int midi) noexcept
    {
        synthFor(program).noteOff(midi);
    }

    void triggerDrum(sensei::core::DrumProgram program, float velocity) noexcept
    {
        drums_.trigger(program, velocity);
    }

    void allNotesOff() noexcept
    {
        chords_.allNotesOff();
        bass_.allNotesOff();
        melody_.allNotesOff();
        drums_.allOff();
    }

    void process(float* left, float* right, int numSamples) noexcept
    {
        chords_.process(left, right, numSamples);
        bass_.process(left, right, numSamples);
        melody_.process(left, right, numSamples);
        drums_.process(left, right, numSamples);
    }

    SimpleSynth& chords() noexcept { return chords_; }
    SimpleSynth& bass() noexcept { return bass_; }
    SimpleSynth& melody() noexcept { return melody_; }

private:
    SimpleSynth& synthFor(sensei::core::SoundProgram program) noexcept
    {
        switch (program)
        {
            case sensei::core::SoundProgram::Bass: return bass_;
            case sensei::core::SoundProgram::Melody: return melody_;
            case sensei::core::SoundProgram::Chords:
            default: return chords_;
        }
    }

    SimpleSynth chords_;
    SimpleSynth bass_;
    SimpleSynth melody_;
    SimpleDrumEngine drums_;
};

} // namespace sensei::engine
