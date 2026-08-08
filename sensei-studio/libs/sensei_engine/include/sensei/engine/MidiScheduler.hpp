#pragma once

#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/engine/InstrumentRack.hpp"
#include "sensei/engine/MidiEventBuffer.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace sensei::engine {

class MidiScheduler
{
public:
    static constexpr int kMaxActive = 128;
    static constexpr int kMaxEvents = 1024;

    void reset() noexcept
    {
        activeCount_ = 0;
        lastGeneration_ = 0;
        wasPlaying_ = false;
        for (auto& a : active_)
            a = {};
    }

    void process(const sensei::core::SequenceSnapshot& snapshot,
                 sensei::core::Transport& transport,
                 InstrumentRack& rack,
                 float* left,
                 float* right,
                 int numSamples,
                 double sampleRate) noexcept
    {
        if (sampleRate <= 0.0 || numSamples <= 0 || left == nullptr)
            return;

        const bool playing = transport.isPlaying();
        if (! playing)
        {
            if (wasPlaying_)
            {
                rack.allNotesOff();
                clearActive();
            }
            wasPlaying_ = false;
            lastGeneration_ = snapshot.generation;
            rack.process(left, right, numSamples);
            return;
        }

        if (! wasPlaying_ || snapshot.generation != lastGeneration_)
        {
            rack.allNotesOff();
            clearActive();
            catchUpNotes(snapshot, transport.positionBeats(), rack);
            lastGeneration_ = snapshot.generation;
        }

        wasPlaying_ = true;

        const double bpm = transport.bpm() > 0.0 ? transport.bpm() : snapshot.bpm;
        const double beatsPerSecond = bpm / 60.0;
        const double blockBeats = (static_cast<double>(numSamples) / sampleRate) * beatsPerSecond;
        const double startBeat = transport.positionBeats();
        const double loopStart = snapshot.loopEnabled ? snapshot.loopStartBeats : 0.0;
        const double loopLength = snapshot.loopEnabled ? snapshot.loopLengthBeats : 0.0;
        const double loopEnd = loopStart + loopLength;

        int eventCount = 0;
        if (snapshot.loopEnabled && loopLength > 0.0 && startBeat + blockBeats > loopEnd)
        {
            const double firstLenBeats = loopEnd - startBeat;
            int firstSamples = static_cast<int>(std::floor((firstLenBeats / blockBeats) * numSamples + 1.0e-9));
            firstSamples = firstSamples < 0 ? 0 : (firstSamples > numSamples ? numSamples : firstSamples);

            eventCount += collectEventsForBeatRange(snapshot, startBeat, loopEnd, 0, firstSamples,
                                                   events_.data() + eventCount, kMaxEvents - eventCount);

            const int secondSamples = numSamples - firstSamples;
            if (secondSamples > 0)
            {
                const double secondBeats = (static_cast<double>(secondSamples) / sampleRate) * beatsPerSecond;
                eventCount += collectEventsForBeatRange(snapshot, loopStart, loopStart + secondBeats,
                                                       firstSamples, secondSamples,
                                                       events_.data() + eventCount, kMaxEvents - eventCount);
            }
            insertionSortEvents(events_.data(), eventCount);
            renderWithEvents(rack, left, right, numSamples, eventCount);
            transport.advance(static_cast<double>(numSamples) / sampleRate);
        }
        else
        {
            eventCount = collectEventsForBeatRange(snapshot, startBeat, startBeat + blockBeats,
                                                  0, numSamples, events_.data(), kMaxEvents);
            renderWithEvents(rack, left, right, numSamples, eventCount);
            transport.advance(static_cast<double>(numSamples) / sampleRate);
        }
    }

private:
    struct Active
    {
        sensei::core::Id id = sensei::core::kInvalidId;
        int pitch = -1;
        sensei::core::SoundProgram program = sensei::core::SoundProgram::Chords;
    };

    void clearActive() noexcept
    {
        activeCount_ = 0;
        for (auto& a : active_)
            a = {};
    }

    void catchUpNotes(const sensei::core::SequenceSnapshot& snapshot,
                      double positionBeats,
                      InstrumentRack& rack) noexcept
    {
        for (std::uint32_t i = 0; i < snapshot.noteCount; ++i)
        {
            const auto& note = snapshot.notes[i];
            if (note.startBeat <= positionBeats + 1.0e-9 && positionBeats < note.endBeat)
                startNote(note.id, note.pitch, note.velocity, note.program, rack);
        }
    }

    void renderWithEvents(InstrumentRack& rack, float* left, float* right, int numSamples, int eventCount) noexcept
    {
        int cursor = 0;
        for (int i = 0; i < eventCount; ++i)
        {
            const auto& ev = events_[static_cast<std::size_t>(i)];
            int at = ev.sampleOffset;
            if (at < cursor) at = cursor;
            if (at > numSamples) at = numSamples;
            const int seg = at - cursor;
            if (seg > 0)
            {
                rack.process(left + cursor, right != nullptr ? right + cursor : nullptr, seg);
                cursor = at;
            }
            applyEvent(ev, rack);
        }
        if (cursor < numSamples)
            rack.process(left + cursor, right != nullptr ? right + cursor : nullptr, numSamples - cursor);
    }

    void applyEvent(const MidiEvent& ev, InstrumentRack& rack) noexcept
    {
        if (ev.isDrum)
        {
            rack.triggerDrum(ev.drum, ev.velocity);
            return;
        }
        if (ev.isNoteOn)
            startNote(ev.id, ev.pitch, ev.velocity, ev.program, rack);
        else
            stopNote(ev.id, ev.pitch, ev.program, rack);
    }

    void startNote(sensei::core::Id id, int pitch, float velocity,
                   sensei::core::SoundProgram program, InstrumentRack& rack) noexcept
    {
        for (int i = 0; i < activeCount_; ++i)
            if (active_[static_cast<std::size_t>(i)].id == id)
                return;
        if (activeCount_ >= kMaxActive)
            return;
        active_[static_cast<std::size_t>(activeCount_++)] = { id, pitch, program };
        rack.noteOn(program, pitch, velocity);
    }

    void stopNote(sensei::core::Id id, int pitch,
                  sensei::core::SoundProgram program, InstrumentRack& rack) noexcept
    {
        for (int i = 0; i < activeCount_; ++i)
        {
            auto& a = active_[static_cast<std::size_t>(i)];
            if (a.id == id)
            {
                rack.noteOff(program, pitch >= 0 ? pitch : a.pitch);
                active_[static_cast<std::size_t>(i)] = active_[static_cast<std::size_t>(activeCount_ - 1)];
                active_[static_cast<std::size_t>(activeCount_ - 1)] = {};
                --activeCount_;
                return;
            }
        }
    }

    std::array<Active, kMaxActive> active_ {};
    std::array<MidiEvent, kMaxEvents> events_ {};
    int activeCount_ = 0;
    std::uint64_t lastGeneration_ = 0;
    bool wasPlaying_ = false;
};

} // namespace sensei::engine
