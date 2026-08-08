#include <catch2/catch_test_macros.hpp>

#include "sensei/core/SequenceSnapshot.hpp"

#include <atomic>
#include <chrono>
#include <thread>

using namespace sensei::core;

TEST_CASE("publish updates generation visible to readers", "[snapshot]")
{
    SnapshotPublisher pub;
    {
        auto& slot = pub.beginWrite();
        slot.generation = 7;
        slot.noteCount = 0;
        pub.publish();
    }
    REQUIRE(pub.publishedGeneration() == 7);

    {
        auto& slot = pub.beginWrite();
        slot.generation = 8;
        REQUIRE(pub.publishedGeneration() == 7);
        pub.publish();
    }
    REQUIRE(pub.publishedGeneration() == 8);

    auto guard = pub.beginRead();
    REQUIRE(guard.get().generation == 8);
}

TEST_CASE("Held audio-local snapshot is immutable while writer publishes", "[snapshot]")
{
    SnapshotPublisher pub;
    {
        auto& slot = pub.beginWrite();
        slot.generation = 1;
        slot.noteCount = 1;
        slot.notes[0].id = 1;
        slot.notes[0].pitch = 60;
        slot.notes[0].startBeat = 0.0;
        slot.notes[0].endBeat = 1.0;
        pub.publish();
    }

    auto guard = pub.beginRead();
    const auto gen = guard.get().generation;
    const auto id = guard.get().notes[0].id;

    // Writer publishes many newer snapshots while audio holds its local view.
    for (std::uint64_t g = 2; g < 50; ++g)
    {
        auto& slot = pub.beginWrite();
        slot.generation = g;
        slot.noteCount = 1;
        slot.notes[0].id = g;
        slot.notes[0].pitch = 60;
        slot.notes[0].startBeat = 0.0;
        slot.notes[0].endBeat = 1.0;
        pub.publish();
    }

    REQUIRE(guard.get().generation == gen);
    REQUIRE(guard.get().notes[0].id == id);
}

TEST_CASE("Concurrent publish/read stress keeps coherent snapshots", "[snapshot][stress]")
{
    SnapshotPublisher pub;
    std::atomic<bool> start { false };
    std::atomic<bool> stop { false };
    std::atomic<std::uint64_t> badReads { 0 };
    std::atomic<std::uint64_t> reads { 0 };

    {
        auto& slot = pub.beginWrite();
        slot.generation = 1;
        slot.noteCount = 1;
        slot.notes[0].id = 1;
        slot.notes[0].pitch = 60;
        slot.notes[0].startBeat = 0.0;
        slot.notes[0].endBeat = 1.0;
        pub.publish();
    }

    std::thread writer([&] {
        while (! start.load())
            std::this_thread::yield();

        std::uint64_t gen = 2;
        while (! stop.load())
        {
            auto& slot = pub.beginWrite();
            slot.bpm = 94.0;
            slot.loopStartBeats = 0.0;
            slot.loopLengthBeats = 16.0;
            slot.loopEnabled = true;
            slot.noteCount = 1;
            slot.notes[0].id = gen;
            slot.notes[0].pitch = static_cast<int>(60 + (gen % 12));
            slot.notes[0].velocity = 0.8f;
            slot.notes[0].startBeat = 0.0;
            slot.notes[0].endBeat = 1.0;
            slot.generation = gen;
            pub.publish();
            ++gen;
        }
    });

    std::thread reader([&] {
        while (! start.load())
            std::this_thread::yield();

        while (! stop.load())
        {
            auto guard = pub.beginRead();
            const auto& snap = guard.get();
            if (snap.noteCount == 1)
            {
                if (snap.notes[0].id != snap.generation)
                    badReads.fetch_add(1);
                if (snap.notes[0].endBeat < snap.notes[0].startBeat)
                    badReads.fetch_add(1);
            }
            reads.fetch_add(1);
        }
    });

    start.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    stop.store(true);
    writer.join();
    reader.join();

    REQUIRE(reads.load() > 0);
    REQUIRE(badReads.load() == 0);
}
