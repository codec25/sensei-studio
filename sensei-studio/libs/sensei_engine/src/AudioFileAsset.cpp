#include "sensei/engine/AudioFileAsset.hpp"

#include <algorithm>
#include <cmath>

namespace sensei::engine {

std::unique_ptr<AudioFileAsset> AudioFileAsset::loadFromFile(const juce::File& file,
                                                             int waveformBuckets)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0)
        return {};

    auto asset = std::unique_ptr<AudioFileAsset>(new AudioFileAsset());
    asset->sampleRate_ = reader->sampleRate;
    const int channels = juce::jlimit(1, 2, static_cast<int>(reader->numChannels));
    const int samples = static_cast<int>(juce::jmin<juce::int64>(reader->lengthInSamples,
                                                                 std::numeric_limits<int>::max()));
    asset->buffer_.setSize(channels, samples, false, true, false);
    if (! reader->read(&asset->buffer_, 0, samples, 0, true, channels > 1))
        return {};

    asset->rebuildWaveform(waveformBuckets);
    return asset;
}

std::unique_ptr<AudioFileAsset> AudioFileAsset::fromBuffer(const juce::AudioBuffer<float>& buffer,
                                                           double sampleRate,
                                                           int waveformBuckets)
{
    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0 || sampleRate <= 0.0)
        return {};

    auto asset = std::unique_ptr<AudioFileAsset>(new AudioFileAsset());
    const int channels = juce::jlimit(1, 2, buffer.getNumChannels());
    asset->buffer_.setSize(channels, buffer.getNumSamples(), false, true, false);
    for (int ch = 0; ch < channels; ++ch)
        asset->buffer_.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    asset->sampleRate_ = sampleRate;
    asset->rebuildWaveform(waveformBuckets);
    return asset;
}

double AudioFileAsset::durationSeconds() const noexcept
{
    return sampleRate_ > 0.0 ? static_cast<double>(buffer_.getNumSamples()) / sampleRate_ : 0.0;
}

void AudioFileAsset::rebuildWaveform(int buckets)
{
    waveform_.clear();
    if (buffer_.getNumSamples() <= 0 || buffer_.getNumChannels() <= 0)
        return;

    const int bucketCount = juce::jlimit(1, buffer_.getNumSamples(), juce::jmax(1, buckets));
    waveform_.resize(static_cast<std::size_t>(bucketCount));

    for (int bucket = 0; bucket < bucketCount; ++bucket)
    {
        const int start = static_cast<int>((static_cast<long long>(bucket) * buffer_.getNumSamples()) / bucketCount);
        const int end = static_cast<int>((static_cast<long long>(bucket + 1) * buffer_.getNumSamples()) / bucketCount);
        float mn = 1.0f;
        float mx = -1.0f;
        for (int ch = 0; ch < buffer_.getNumChannels(); ++ch)
        {
            const float* data = buffer_.getReadPointer(ch);
            for (int i = start; i < juce::jmax(start + 1, end); ++i)
            {
                const float v = data[juce::jlimit(0, buffer_.getNumSamples() - 1, i)];
                mn = std::min(mn, v);
                mx = std::max(mx, v);
            }
        }
        waveform_[static_cast<std::size_t>(bucket)] = { mn, mx };
    }
}

} // namespace sensei::engine
