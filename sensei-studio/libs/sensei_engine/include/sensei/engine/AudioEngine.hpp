#pragma once

#include "sensei/core/SequenceSnapshot.hpp"
#include "sensei/core/Transport.hpp"
#include "sensei/engine/MidiScheduler.hpp"
#include "sensei/engine/SimpleSynth.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>

namespace sensei::engine {

// Owns the audio device callback.
// Depends on Core transport + SnapshotPublisher reads only.
// Must not depend on UI, teaching logic, networking, or AI.
class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    void setTransport(sensei::core::Transport* transport) noexcept;
    void setSnapshotPublisher(const sensei::core::SnapshotPublisher* publisher) noexcept;

    // Initializes the default output device. Call from the message thread.
    bool initialise();
    void shutdown();

    // Audition / trigger API for the UI. Safe while transport is stopped.
    void noteOn(int midiNote, float velocity = 0.8f) noexcept;
    void noteOff(int midiNote) noexcept;
    void allNotesOff() noexcept;

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
    SimpleSynth synth_;
    MidiScheduler scheduler_;
    std::atomic<bool> initialised_ { false };
    std::atomic<double> sampleRate_ { 44100.0 };
};

} // namespace sensei::engine
