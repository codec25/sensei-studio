#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/Types.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace sensei::core {

// POD note for audio-thread consumption. Fixed-capacity; no heap on audio thread.
struct ScheduledNote
{
    Id id = kInvalidId;
    int pitch = 0;
    float velocity = 0.0f;
    double startBeat = 0.0;
    double endBeat = 0.0;
};

// Realtime-safe musical snapshot.
//
// Publication strategy (triple slot, no shared_ptr):
// - Three fixed SequenceSnapshot slots live for the process lifetime.
// - Message thread writes into a slot that is not currently published.
// - Message thread then atomically publishes that slot index.
// - Audio thread only loads the published index and reads that slot.
// - Nothing is allocated or destroyed on the audio thread.
struct SequenceSnapshot
{
    static constexpr std::size_t kMaxNotes = 256;

    std::uint64_t generation = 0;
    double bpm = kDefaultBpm;
    double loopStartBeats = 0.0;
    double loopLengthBeats = kDefaultLoopBeats;
    bool loopEnabled = true;
    std::uint32_t noteCount = 0;
    std::array<ScheduledNote, kMaxNotes> notes {};
};

class SnapshotPublisher
{
public:
    SnapshotPublisher() = default;

    // Message thread: fill a writable slot, then publish().
    [[nodiscard]] SequenceSnapshot& beginWrite() noexcept
    {
        const int published = publishedIndex_.load(std::memory_order_acquire);
        writeIndex_ = (published + 1) % kSlotCount;
        if (writeIndex_ == published)
            writeIndex_ = (writeIndex_ + 1) % kSlotCount;
        return slots_[static_cast<std::size_t>(writeIndex_)];
    }

    void publish() noexcept
    {
        publishedIndex_.store(writeIndex_, std::memory_order_release);
    }

    // Audio thread / any reader: never frees memory.
    [[nodiscard]] const SequenceSnapshot& read() const noexcept
    {
        const int index = publishedIndex_.load(std::memory_order_acquire);
        return slots_[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] std::uint64_t publishedGeneration() const noexcept
    {
        return read().generation;
    }

private:
    static constexpr int kSlotCount = 3;
    std::array<SequenceSnapshot, kSlotCount> slots_ {};
    std::atomic<int> publishedIndex_ { 0 };
    int writeIndex_ { 1 };
};

} // namespace sensei::core
