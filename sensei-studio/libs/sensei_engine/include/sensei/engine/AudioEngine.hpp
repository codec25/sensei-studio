#pragma once

#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/engine/InstrumentRack.hpp"
#include "sensei/engine/MidiScheduler.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>

namespace sensei::engine {

class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    void setTransport(sensei::core::Transport* transport) noexcept;
    void setSnapshotPublisher(const sensei::core::SnapshotPublisher* publisher) noexcept;

    bool initialise();
    void shutdown();

    void noteOn(sensei::core::InstrumentId instrumentId, int midiNote, float velocity = 0.8f) noexcept;
    void noteOff(sensei::core::InstrumentId instrumentId, int midiNote) noexcept;
    void noteOn(sensei::core::SoundProgram program, int midiNote, float velocity = 0.8f) noexcept;
    void noteOff(sensei::core::SoundProgram program, int midiNote) noexcept;
    void allNotesOff() noexcept;

    // Backward-compatible audition helpers (Warm Keys).
    void noteOn(int midiNote, float velocity = 0.8f) noexcept
    {
        noteOn(sensei::core::InstrumentId::WarmKeys, midiNote, velocity);
    }
    void noteOff(int midiNote) noexcept { noteOff(sensei::core::InstrumentId::WarmKeys, midiNote); }

    [[nodiscard]] bool isInitialised() const noexcept { return initialised_.load(); }

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    juce::AudioDeviceManager deviceManager_;
    std::atomic<sensei::core::Transport*> transport_ { nullptr };
    std::atomic<const sensei::core::SnapshotPublisher*> snapshots_ { nullptr };
    InstrumentRack rack_;
    MidiScheduler scheduler_;
    std::atomic<bool> initialised_ { false };
    std::atomic<double> sampleRate_ { 44100.0 };
};

} // namespace sensei::engine
