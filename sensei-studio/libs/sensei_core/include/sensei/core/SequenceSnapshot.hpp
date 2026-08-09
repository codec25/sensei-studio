#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/Types.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace sensei::core {

// Legacy role-ish program tag (kept for a few call sites / tests). Prefer InstrumentId.
enum class SoundProgram : std::uint8_t
{
    Chords = 0,
    Bass = 1,
    Melody = 2
};

enum class DrumProgram : std::uint8_t
{
    Kick = 0,
    Snare = 1,
    ClosedHat = 2
};

[[nodiscard]] inline constexpr SoundProgram soundProgramForInstrument(InstrumentId id) noexcept
{
    switch (id)
    {
        case InstrumentId::DeepBass: return SoundProgram::Bass;
        case InstrumentId::BrightPluck: return SoundProgram::Melody;
        case InstrumentId::WarmKeys:
        default: return SoundProgram::Chords;
    }
}

// POD note for audio-thread consumption. Fixed-capacity; no heap on audio thread.
struct ScheduledNote
{
    Id id = kInvalidId;
    int pitch = 0;
    float velocity = 0.0f;
    double startBeat = 0.0;
    double endBeat = 0.0;
    InstrumentId instrumentId = InstrumentId::WarmKeys;
    SoundProgram program = SoundProgram::Chords; // derived mirror for compatibility
};

struct ScheduledDrumHit
{
    double beat = 0.0;
    DrumProgram program = DrumProgram::Kick;
    InstrumentId instrumentId = InstrumentId::StudioKitBasic;
    float velocity = 0.8f;
};

struct SequenceSnapshot
{
    static constexpr std::size_t kMaxNotes = 2048;
    static constexpr std::size_t kMaxDrumHits = 1024;

    std::uint64_t generation = 0;
    double bpm = kDefaultBpm;
    double loopStartBeats = 0.0;
    double loopLengthBeats = kDefaultLoopBeats;
    bool loopEnabled = true;
    double songLengthBeats = kDefaultLoopBeats;
    std::uint32_t noteCount = 0;
    std::uint32_t drumHitCount = 0;
    std::array<ScheduledNote, kMaxNotes> notes {};
    std::array<ScheduledDrumHit, kMaxDrumHits> drumHits {};
};

// Realtime-safe snapshot publication (single message-thread writer, single audio reader).
//
// DESIGN:
// - Message thread writes into private `back_` (never read by audio).
// - `publish()` copies `back_` → `front_` under a mutex (message thread only blocks here).
// - Audio thread `beginRead()` uses `try_lock()`:
//     - If acquired: copies `front_` → `audioLocal_` (fixed-size POD copy, no heap), unlocks.
//     - If busy: keeps the previous `audioLocal_` (last known-good snapshot).
// - Audio then reads only `audioLocal_` for the rest of the callback.
//
// INVARIANT:
// - Audio never blocks (try_lock only), never allocates/deallocates heap.
// - Audio never reads memory the message thread is mutating (`back_` / `front_` under lock).
// - `audioLocal_` is written only by the audio thread; ownership/lifetime is explicit.
// - A successful try_lock copy is a coherent snapshot; a failed try_lock reuses the
//   previous coherent local copy (at most one publish late — acceptable for Milestone B).
class SnapshotPublisher
{
public:
    class ReadGuard
    {
    public:
        ReadGuard() = default;
        explicit ReadGuard(const SequenceSnapshot* snapshot) noexcept : snapshot_(snapshot) {}

        [[nodiscard]] const SequenceSnapshot& get() const noexcept { return *snapshot_; }
        [[nodiscard]] bool valid() const noexcept { return snapshot_ != nullptr; }

    private:
        const SequenceSnapshot* snapshot_ = nullptr;
    };

    SnapshotPublisher()
    {
        audioLocal_ = front_;
    }

    // Message thread only.
    [[nodiscard]] SequenceSnapshot& beginWrite() noexcept { return back_; }

    // Message thread only.
    void publish()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        front_ = back_;
        publishedGeneration_.store(front_.generation, std::memory_order_release);
    }

    // Audio thread only. Keep the returned guard for the callback duration.
    [[nodiscard]] ReadGuard beginRead() const
    {
        if (mutex_.try_lock())
        {
            audioLocal_ = front_;
            mutex_.unlock();
            ++successfulCopies_;
        }
        else
        {
            ++skippedCopies_;
        }

        return ReadGuard { &audioLocal_ };
    }

    [[nodiscard]] std::uint64_t publishedGeneration() const noexcept
    {
        return publishedGeneration_.load(std::memory_order_acquire);
    }

    // Test helpers
    [[nodiscard]] std::uint64_t successfulCopies() const noexcept { return successfulCopies_; }
    [[nodiscard]] std::uint64_t skippedCopies() const noexcept { return skippedCopies_; }

    [[nodiscard]] const SequenceSnapshot& debugReadPublished() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return front_;
    }

private:
    SequenceSnapshot back_ {};
    SequenceSnapshot front_ {};
    mutable SequenceSnapshot audioLocal_ {};
    mutable std::mutex mutex_;
    std::atomic<std::uint64_t> publishedGeneration_ { 0 };
    mutable std::uint64_t successfulCopies_ { 0 };
    mutable std::uint64_t skippedCopies_ { 0 };
};

} // namespace sensei::core
