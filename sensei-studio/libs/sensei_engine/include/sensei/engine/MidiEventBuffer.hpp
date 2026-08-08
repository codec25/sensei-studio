#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/SequenceSnapshot.hpp"

#include <cstdint>

namespace sensei::engine {

struct MidiEvent
{
    int sampleOffset = 0; // within the audio block; note-off may equal range end
    bool isNoteOn = false;
    sensei::core::Id id = sensei::core::kInvalidId;
    int pitch = 0;
    float velocity = 0.0f;
};

// Maps a beat inside [rangeStartBeat, rangeStartBeat + rangeBeats) to a sample
// offset in [baseSampleOffset, baseSampleOffset + rangeSampleCount].
inline int beatToSampleOffset(double beat,
                              double rangeStartBeat,
                              double rangeBeats,
                              int baseSampleOffset,
                              int rangeSampleCount) noexcept
{
    if (rangeBeats <= 0.0 || rangeSampleCount <= 0)
        return baseSampleOffset;

    const double t = (beat - rangeStartBeat) / rangeBeats;
    int offset = baseSampleOffset;
    if (t <= 0.0)
        offset = baseSampleOffset;
    else if (t >= 1.0)
        offset = baseSampleOffset + rangeSampleCount;
    else
        offset = baseSampleOffset + static_cast<int>(t * static_cast<double>(rangeSampleCount));

    if (offset < baseSampleOffset)
        offset = baseSampleOffset;
    if (offset > baseSampleOffset + rangeSampleCount)
        offset = baseSampleOffset + rangeSampleCount;
    return offset;
}

inline void insertionSortEvents(MidiEvent* events, int count) noexcept
{
    for (int i = 1; i < count; ++i)
    {
        MidiEvent key = events[i];
        int j = i - 1;
        while (j >= 0)
        {
            const bool outOfOrder = events[j].sampleOffset > key.sampleOffset
                                    || (events[j].sampleOffset == key.sampleOffset
                                        && events[j].isNoteOn && ! key.isNoteOn);
            if (! outOfOrder)
                break;
            events[j + 1] = events[j];
            --j;
        }
        events[j + 1] = key;
    }
}

// Collect note-on/off for beat range [fromBeat, toBeat) into sample offsets
// [baseSampleOffset, baseSampleOffset + rangeSampleCount].
// No heap allocation. Returns event count.
inline int collectEventsForBeatRange(const sensei::core::SequenceSnapshot& snapshot,
                                     double fromBeat,
                                     double toBeat,
                                     int baseSampleOffset,
                                     int rangeSampleCount,
                                     MidiEvent* out,
                                     int maxEvents) noexcept
{
    if (out == nullptr || maxEvents <= 0 || toBeat <= fromBeat || rangeSampleCount < 0)
        return 0;

    const double rangeBeats = toBeat - fromBeat;
    int count = 0;

    for (std::uint32_t i = 0; i < snapshot.noteCount; ++i)
    {
        const auto& note = snapshot.notes[i];

        if (note.startBeat >= fromBeat && note.startBeat < toBeat)
        {
            if (count >= maxEvents)
                break;

            MidiEvent ev;
            ev.isNoteOn = true;
            ev.id = note.id;
            ev.pitch = note.pitch;
            ev.velocity = note.velocity;
            ev.sampleOffset = beatToSampleOffset(
                note.startBeat, fromBeat, rangeBeats, baseSampleOffset, rangeSampleCount);
            if (ev.sampleOffset >= baseSampleOffset + rangeSampleCount)
                ev.sampleOffset = baseSampleOffset + rangeSampleCount - 1;
            if (ev.sampleOffset < baseSampleOffset)
                ev.sampleOffset = baseSampleOffset;
            out[count++] = ev;
        }

        if (note.endBeat > fromBeat && note.endBeat <= toBeat + 1.0e-12)
        {
            if (count >= maxEvents)
                break;

            MidiEvent ev;
            ev.isNoteOn = false;
            ev.id = note.id;
            ev.pitch = note.pitch;
            ev.velocity = 0.0f;
            ev.sampleOffset = beatToSampleOffset(
                note.endBeat, fromBeat, rangeBeats, baseSampleOffset, rangeSampleCount);
            out[count++] = ev;
        }
    }

    insertionSortEvents(out, count);
    return count;
}

} // namespace sensei::engine
