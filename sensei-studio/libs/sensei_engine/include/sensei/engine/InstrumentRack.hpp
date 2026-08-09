#pragma once

#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/engine/instruments/BrightPluck.hpp"
#include "sensei/engine/instruments/DeepBass.hpp"
#include "sensei/engine/instruments/StudioDrumKit.hpp"
#include "sensei/engine/instruments/WarmKeys.hpp"

namespace sensei::engine {

// Routes by stable InstrumentId to differentiated built-in engines.
class InstrumentRack
{
public:
    void prepare(double sampleRate, int maxBlockSize = 512) noexcept
    {
        warmKeys_.prepare(sampleRate, maxBlockSize);
        deepBass_.prepare(sampleRate, maxBlockSize);
        brightPluck_.prepare(sampleRate, maxBlockSize);
        studioKit_.prepare(sampleRate, maxBlockSize);
    }

    void noteOn(sensei::core::InstrumentId id, int midi, float velocity) noexcept
    {
        if (auto* inst = pitched(id))
            inst->noteOn(midi, velocity);
    }

    void noteOff(sensei::core::InstrumentId id, int midi) noexcept
    {
        if (auto* inst = pitched(id))
            inst->noteOff(midi);
    }

    // Compatibility overload used by older call sites.
    void noteOn(sensei::core::SoundProgram program, int midi, float velocity) noexcept
    {
        noteOn(instrumentForProgram(program), midi, velocity);
    }

    void noteOff(sensei::core::SoundProgram program, int midi) noexcept
    {
        noteOff(instrumentForProgram(program), midi);
    }

    void triggerDrum(sensei::core::DrumProgram pad, float velocity) noexcept
    {
        studioKit_.trigger(pad, velocity);
    }

    void triggerDrum(sensei::core::InstrumentId kitId,
                     sensei::core::DrumProgram pad,
                     float velocity) noexcept
    {
        if (kitId == sensei::core::InstrumentId::StudioKitBasic)
            studioKit_.trigger(pad, velocity);
        else
            studioKit_.trigger(pad, velocity); // single kit in Milestone E
    }

    void allNotesOff() noexcept
    {
        warmKeys_.allNotesOff();
        deepBass_.allNotesOff();
        brightPluck_.allNotesOff();
        studioKit_.allOff();
    }

    void process(float* left, float* right, int numSamples) noexcept
    {
        warmKeys_.process(left, right, numSamples);
        deepBass_.process(left, right, numSamples);
        brightPluck_.process(left, right, numSamples);
        studioKit_.process(left, right, numSamples);
    }

    [[nodiscard]] Instrument* pitched(sensei::core::InstrumentId id) noexcept
    {
        switch (id)
        {
            case sensei::core::InstrumentId::DeepBass: return &deepBass_;
            case sensei::core::InstrumentId::BrightPluck: return &brightPluck_;
            case sensei::core::InstrumentId::WarmKeys: return &warmKeys_;
            case sensei::core::InstrumentId::StudioKitBasic:
            default: return nullptr;
        }
    }

    [[nodiscard]] const Instrument* pitched(sensei::core::InstrumentId id) const noexcept
    {
        switch (id)
        {
            case sensei::core::InstrumentId::DeepBass: return &deepBass_;
            case sensei::core::InstrumentId::BrightPluck: return &brightPluck_;
            case sensei::core::InstrumentId::WarmKeys: return &warmKeys_;
            case sensei::core::InstrumentId::StudioKitBasic:
            default: return nullptr;
        }
    }

    [[nodiscard]] StudioDrumKit& drums() noexcept { return studioKit_; }

private:
    static sensei::core::InstrumentId instrumentForProgram(sensei::core::SoundProgram program) noexcept
    {
        switch (program)
        {
            case sensei::core::SoundProgram::Bass: return sensei::core::InstrumentId::DeepBass;
            case sensei::core::SoundProgram::Melody: return sensei::core::InstrumentId::BrightPluck;
            case sensei::core::SoundProgram::Chords:
            default: return sensei::core::InstrumentId::WarmKeys;
        }
    }

    WarmKeys warmKeys_;
    DeepBass deepBass_;
    BrightPluck brightPluck_;
    StudioDrumKit studioKit_;
};

} // namespace sensei::engine
