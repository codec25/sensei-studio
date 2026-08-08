#pragma once

#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/engine/MidiEventBuffer.hpp"
#include "sensei/engine/SimpleSynth.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace sensei::engine {

// Schedules note-on/note-off from a Core SequenceSnapshot using the audio timeline.
// Applies events at sample-accurate offsets and renders the synth in segments.
// Realtime-safe: no heap allocation in process().
class MidiScheduler
{
public:
    static constexpr int kMaxActive = 64;
    static constexpr int kMaxEvents = 512;

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
                 SimpleSynth& synth,
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
                synth.allNotesOff();
                clearActive();
            }
            wasPlaying_ = false;
            lastGeneration_ = snapshot.generation;
            synth.process(left, right, numSamples);
            return;
        }

        if (! wasPlaying_ || snapshot.generation != lastGeneration_)
        {
            synth.allNotesOff();
            clearActive();
            catchUpNotes(snapshot, transport.positionBeats(), synth);
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
            if (firstSamples < 0)
                firstSamples = 0;
            if (firstSamples > numSamples)
                firstSamples = numSamples;

            eventCount += collectEventsForBeatRange(snapshot,
                                                   startBeat,
                                                   loopEnd,
                                                   0,
                                                   firstSamples,
                                                   events_.data() + eventCount,
                                                   kMaxEvents - eventCount);

            const int secondSamples = numSamples - firstSamples;
            if (secondSamples > 0)
            {
                const double secondBeats = (static_cast<double>(secondSamples) / sampleRate) * beatsPerSecond;
                eventCount += collectEventsForBeatRange(snapshot,
                                                       loopStart,
                                                       loopStart + secondBeats,
                                                       firstSamples,
                                                       secondSamples,
                                                       events_.data() + eventCount,
                                                       kMaxEvents - eventCount);
            }

            insertionSortEvents(events_.data(), eventCount);
            renderWithEvents(synth, left, right, numSamples, eventCount);
            transport.advance(static_cast<double>(numSamples) / sampleRate);
        }
        else
        {
            eventCount = collectEventsForBeatRange(snapshot,
                                                  startBeat,
                                                  startBeat + blockBeats,
                                                  0,
                                                  numSamples,
                                                  events_.data(),
                                                  kMaxEvents);
            renderWithEvents(synth, left, right, numSamples, eventCount);
            transport.advance(static_cast<double>(numSamples) / sampleRate);
        }
    }

private:
    struct Active
    {
        sensei::core::Id id = sensei::core::kInvalidId;
        int pitch = -1;
    };

    void clearActive() noexcept
    {
        activeCount_ = 0;
        for (auto& a : active_)
            a = {};
    }

    void catchUpNotes(const sensei::core::SequenceSnapshot& snapshot,
                      double positionBeats,
                      SimpleSynth& synth) noexcept
    {
        for (std::uint32_t i = 0; i < snapshot.noteCount; ++i)
        {
            const auto& note = snapshot.notes[i];
            if (note.startBeat <= positionBeats + 1.0e-9 && positionBeats < note.endBeat)
                startNote(note.id, note.pitch, note.velocity, synth);
        }
    }

    void renderWithEvents(SimpleSynth& synth,
                          float* left,
                          float* right,
                          int numSamples,
                          int eventCount) noexcept
    {
        int cursor = 0;
        for (int i = 0; i < eventCount; ++i)
        {
            const auto& ev = events_[static_cast<std::size_t>(i)];
            int at = ev.sampleOffset;
            if (at < cursor)
                at = cursor;
            if (at > numSamples)
                at = numSamples;

            const int seg = at - cursor;
            if (seg > 0)
            {
                synth.process(left + cursor, right != nullptr ? right + cursor : nullptr, seg);
                cursor = at;
            }

            applyEvent(ev, synth);
        }

        if (cursor < numSamples)
            synth.process(left + cursor, right != nullptr ? right + cursor : nullptr, numSamples - cursor);
    }

    void applyEvent(const MidiEvent& ev, SimpleSynth& synth) noexcept
    {
        if (ev.isNoteOn)
            startNote(ev.id, ev.pitch, ev.velocity, synth);
        else
            stopNote(ev.id, ev.pitch, synth);
    }

    void startNote(sensei::core::Id id, int pitch, float velocity, SimpleSynth& synth) noexcept
    {
        for (int i = 0; i < activeCount_; ++i)
        {
            if (active_[static_cast<std::size_t>(i)].id == id)
                return;
        }

        if (activeCount_ >= kMaxActive)
            return;

        active_[static_cast<std::size_t>(activeCount_++)] = { id, pitch };
        synth.noteOn(pitch, velocity);
    }

    void stopNote(sensei::core::Id id, int pitch, SimpleSynth& synth) noexcept
    {
        for (int i = 0; i < activeCount_; ++i)
        {
            auto& a = active_[static_cast<std::size_t>(i)];
            if (a.id == id)
            {
                synth.noteOff(pitch >= 0 ? pitch : a.pitch);
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
