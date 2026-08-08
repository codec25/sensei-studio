#include <catch2/catch_test_macros.hpp>

#include "sensei/engine/MidiEventBuffer.hpp"

#include <array>

using namespace sensei::core;
using namespace sensei::engine;

TEST_CASE("beatToSampleOffset maps range endpoints", "[scheduler]")
{
    REQUIRE(beatToSampleOffset(0.0, 0.0, 4.0, 0, 128) == 0);
    REQUIRE(beatToSampleOffset(4.0, 0.0, 4.0, 0, 128) == 128);
    REQUIRE(beatToSampleOffset(2.0, 0.0, 4.0, 0, 128) == 64);
}

TEST_CASE("collectEventsForBeatRange is sample-accurate for note on/off", "[scheduler]")
{
    SequenceSnapshot snap;
    snap.noteCount = 1;
    snap.notes[0] = { 1, 60, 0.8f, 1.0, 3.0 }; // beats 1..3 inside 0..4

    std::array<MidiEvent, 16> events {};
    const int count = collectEventsForBeatRange(snap, 0.0, 4.0, 0, 128, events.data(), 16);
    REQUIRE(count == 2);

    // 1.0 / 4.0 * 128 = 32; 3.0 / 4.0 * 128 = 96
    REQUIRE(events[0].isNoteOn);
    REQUIRE(events[0].sampleOffset == 32);
    REQUIRE(events[0].pitch == 60);

    REQUIRE_FALSE(events[1].isNoteOn);
    REQUIRE(events[1].sampleOffset == 96);
}

TEST_CASE("Loop-boundary second range offsets are relative to baseSampleOffset", "[scheduler]")
{
    SequenceSnapshot snap;
    snap.noteCount = 1;
    snap.notes[0] = { 2, 64, 0.8f, 0.0, 0.5 }; // starts at loop start

    std::array<MidiEvent, 16> events {};
    const int count = collectEventsForBeatRange(snap, 0.0, 1.0, 100, 50, events.data(), 16);
    REQUIRE(count >= 1);
    REQUIRE(events[0].isNoteOn);
    REQUIRE(events[0].sampleOffset == 100);
}

TEST_CASE("Note-off sorts before note-on at the same sample (loop retrigger)", "[scheduler]")
{
    SequenceSnapshot snap;
    snap.noteCount = 2;
    // Note A: 0..1 (off at beat 1), Note B: 1..2 (on at beat 1)
    snap.notes[0] = { 1, 60, 0.8f, 0.0, 1.0 };
    snap.notes[1] = { 2, 62, 0.8f, 1.0, 2.0 };

    std::array<MidiEvent, 16> events {};
    const int count = collectEventsForBeatRange(snap, 0.0, 2.0, 0, 64, events.data(), 16);
    REQUIRE(count == 4); // on@0, off@32, on@32, off@64

    bool sawOffBeforeOn = false;
    for (int i = 0; i + 1 < count; ++i)
    {
        if (events[i].sampleOffset == events[i + 1].sampleOffset
            && ! events[i].isNoteOn && events[i + 1].isNoteOn)
        {
            sawOffBeforeOn = true;
        }
    }
    REQUIRE(sawOffBeforeOn);
}
