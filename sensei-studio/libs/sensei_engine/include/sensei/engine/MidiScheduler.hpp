#pragma once

#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/engine/SimpleSynth.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace sensei::engine {

// Schedules note-on/note-off from a Core SequenceSnapshot using the audio timeline.
// Realtime-safe: no heap allocation in process().
class MidiScheduler
{
public:
    static constexpr int kMaxActive = 64;

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
                 int numSamples,
                 double sampleRate) noexcept
    {
        if (sampleRate <= 0.0 || numSamples <= 0)
            return;

        const bool playing = transport.isPlaying();

        if (! playing)
        {
            if (wasPlaying_)
            {
                synth.allNotesOff();
                clearActive(synth);
            }
            wasPlaying_ = false;
            lastGeneration_ = snapshot.generation;
            return;
        }

        if (! wasPlaying_ || snapshot.generation != lastGeneration_)
        {
            synth.allNotesOff();
            clearActive(synth);
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

        if (snapshot.loopEnabled && loopLength > 0.0 && startBeat + blockBeats > loopEnd)
        {
            const double firstLen = loopEnd - startBeat;
            const int firstSamples = static_cast<int>(std::lround((firstLen / blockBeats) * numSamples));
            scheduleRange(snapshot, synth, startBeat, loopEnd, 0, sampleRate, beatsPerSecond);
            transport.advance(firstLen / beatsPerSecond);

            const int secondSamples = numSamples - firstSamples;
            if (secondSamples > 0)
            {
                // Voices that end exactly on the loop boundary are already off.
                // Retrigger notes that start at loopStart.
                scheduleRange(snapshot,
                              synth,
                              loopStart,
                              loopStart + (static_cast<double>(secondSamples) / sampleRate) * beatsPerSecond,
                              firstSamples,
                              sampleRate,
                              beatsPerSecond);
                transport.advance(static_cast<double>(secondSamples) / sampleRate);
            }
        }
        else
        {
            scheduleRange(snapshot,
                          synth,
                          startBeat,
                          startBeat + blockBeats,
                          0,
                          sampleRate,
                          beatsPerSecond);
            transport.advance(static_cast<double>(numSamples) / sampleRate);
        }
    }

private:
    struct Active
    {
        sensei::core::Id id = sensei::core::kInvalidId;
        int pitch = -1;
    };

    void clearActive(SimpleSynth&) noexcept
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
                startNote(note, synth);
        }
    }

    void scheduleRange(const sensei::core::SequenceSnapshot& snapshot,
                       SimpleSynth& synth,
                       double fromBeat,
                       double toBeat,
                       int sampleOffset,
                       double sampleRate,
                       double beatsPerSecond) noexcept
    {
        if (toBeat <= fromBeat)
            return;

        for (std::uint32_t i = 0; i < snapshot.noteCount; ++i)
        {
            const auto& note = snapshot.notes[i];

            if (note.startBeat >= fromBeat && note.startBeat < toBeat)
                startNote(note, synth);

            if (note.endBeat > fromBeat && note.endBeat <= toBeat + 1.0e-12)
                stopNote(note.id, note.pitch, synth);
        }

        (void) sampleOffset;
        (void) sampleRate;
        (void) beatsPerSecond;
    }

    void startNote(const sensei::core::ScheduledNote& note, SimpleSynth& synth) noexcept
    {
        // Avoid duplicate active id.
        for (int i = 0; i < activeCount_; ++i)
        {
            if (active_[static_cast<std::size_t>(i)].id == note.id)
                return;
        }

        if (activeCount_ >= kMaxActive)
            return;

        active_[static_cast<std::size_t>(activeCount_++)] = { note.id, note.pitch };
        synth.noteOn(note.pitch, note.velocity);
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
    int activeCount_ = 0;
    std::uint64_t lastGeneration_ = 0;
    bool wasPlaying_ = false;
};

} // namespace sensei::engine
