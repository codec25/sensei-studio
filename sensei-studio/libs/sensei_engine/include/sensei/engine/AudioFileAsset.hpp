#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <memory>
#include <vector>

namespace sensei::engine {

struct WaveformPeak
{
    float min = 0.0f;
    float max = 0.0f;
};

// Immutable decoded source audio. File I/O and waveform analysis happen off the
// realtime audio callback; rendering only reads the resulting buffer/peaks.
class AudioFileAsset final
{
public:
    static std::unique_ptr<AudioFileAsset> loadFromFile(const juce::File& file,
                                                        int waveformBuckets = 1024);

    // Test/import helper for already-decoded audio. Copies on the caller thread.
    static std::unique_ptr<AudioFileAsset> fromBuffer(const juce::AudioBuffer<float>& buffer,
                                                      double sampleRate,
                                                      int waveformBuckets = 1024);

    [[nodiscard]] const juce::AudioBuffer<float>& buffer() const noexcept { return buffer_; }
    [[nodiscard]] double sampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] double durationSeconds() const noexcept;
    [[nodiscard]] const std::vector<WaveformPeak>& waveform() const noexcept { return waveform_; }

private:
    AudioFileAsset() = default;
    void rebuildWaveform(int buckets);

    juce::AudioBuffer<float> buffer_;
    double sampleRate_ = 44100.0;
    std::vector<WaveformPeak> waveform_;
};

} // namespace sensei::engine
