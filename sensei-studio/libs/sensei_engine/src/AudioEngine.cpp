#include "sensei/engine/AudioEngine.hpp"

namespace sensei::engine {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::setTransport(sensei::core::Transport* transport) noexcept
{
    transport_.store(transport, std::memory_order_release);
}

void AudioEngine::setSnapshotPublisher(const sensei::core::SnapshotPublisher* publisher) noexcept
{
    snapshots_.store(publisher, std::memory_order_release);
}

bool AudioEngine::initialise()
{
    if (initialised_.load())
        return true;

    auto result = deviceManager_.initialiseWithDefaultDevices(0, 2);
    if (result.isNotEmpty())
        return false;

    deviceManager_.addAudioCallback(this);
    initialised_.store(true);
    return true;
}

void AudioEngine::shutdown()
{
    if (! initialised_.exchange(false))
        return;

    deviceManager_.removeAudioCallback(this);
    deviceManager_.closeAudioDevice();
    rack_.allNotesOff();
    scheduler_.reset();
}

void AudioEngine::noteOn(sensei::core::SoundProgram program, int midiNote, float velocity) noexcept
{
    rack_.noteOn(program, midiNote, velocity);
}

void AudioEngine::noteOff(sensei::core::SoundProgram program, int midiNote) noexcept
{
    rack_.noteOff(program, midiNote);
}

void AudioEngine::allNotesOff() noexcept
{
    rack_.allNotesOff();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double sr = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    sampleRate_.store(sr > 0.0 ? sr : 44100.0, std::memory_order_relaxed);
    rack_.prepare(sampleRate_.load(std::memory_order_relaxed));
    scheduler_.reset();
}

void AudioEngine::audioDeviceStopped()
{
    rack_.allNotesOff();
    scheduler_.reset();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const*,
                                                   int,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    float* left = numOutputChannels > 0 ? outputChannelData[0] : nullptr;
    float* right = numOutputChannels > 1 ? outputChannelData[1] : left;
    if (left == nullptr)
        return;

    auto* transport = transport_.load(std::memory_order_acquire);
    const auto* snapshots = snapshots_.load(std::memory_order_acquire);
    const double sampleRate = sampleRate_.load(std::memory_order_relaxed);

    if (transport != nullptr && snapshots != nullptr)
    {
        auto guard = snapshots->beginRead();
        scheduler_.process(guard.get(), *transport, rack_, left, right, numSamples, sampleRate);
        return;
    }

    rack_.process(left, right, numSamples);
}

} // namespace sensei::engine
