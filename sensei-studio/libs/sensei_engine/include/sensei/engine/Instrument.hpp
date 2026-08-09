#pragma once

#include "sensei/core/InstrumentId.hpp"

namespace sensei::engine {

// Realtime-safe pitched instrument interface. No heap/file/UI/network in methods.
class Instrument
{
public:
    virtual ~Instrument() = default;

    [[nodiscard]] virtual sensei::core::InstrumentId id() const noexcept = 0;

    virtual void prepare(double sampleRate, int maxBlockSize) noexcept = 0;
    virtual void noteOn(int midiNote, float velocity) noexcept = 0;
    virtual void noteOff(int midiNote) noexcept = 0;
    virtual void allNotesOff() noexcept = 0;
    virtual void process(float* left, float* right, int numSamples) noexcept = 0;
};

} // namespace sensei::engine
