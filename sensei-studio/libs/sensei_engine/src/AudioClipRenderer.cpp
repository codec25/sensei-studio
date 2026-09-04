#include "sensei/engine/AudioClipRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace sensei::engine {
namespace {

float interpolatedSample(const juce::AudioBuffer<float>& source,
                         int channel,
                         double samplePosition) noexcept
{
    if (source.getNumSamples() <= 0)
        return 0.0f;

    const double clamped = std::clamp(samplePosition, 0.0,
                                      static_cast<double>(source.getNumSamples() - 1));
    const int i0 = static_cast<int>(clamped);
    const int i1 = juce::jmin(i0 + 1, source.getNumSamples() - 1);
    const float frac = static_cast<float>(clamped - static_cast<double>(i0));
    const float* data = source.getReadPointer(juce::jlimit(0, source.getNumChannels() - 1, channel));
    return data[i0] + (data[i1] - data[i0]) * frac;
}

float clipEnvelope(const sensei::core::AudioClip& clip, double secondsIntoClip) noexcept
{
    float gain = 1.0f;
    if (clip.fadeIn.lengthSeconds > 0.0 && secondsIntoClip < clip.fadeIn.lengthSeconds)
        gain *= static_cast<float>(sensei::core::fadeGain(secondsIntoClip / clip.fadeIn.lengthSeconds,
                                                          clip.fadeIn.curve));

    const double remaining = clip.sourceLengthSeconds - secondsIntoClip;
    if (clip.fadeOut.lengthSeconds > 0.0 && remaining < clip.fadeOut.lengthSeconds)
        gain *= static_cast<float>(sensei::core::fadeGain(remaining / clip.fadeOut.lengthSeconds,
                                                          clip.fadeOut.curve));
    return gain;
}

} // namespace

void renderAudioClip(const sensei::core::AudioClip& clip,
                     const AudioFileAsset& asset,
                     const AudioRenderContext& context,
                     float* left,
                     float* right,
                     float* reverbLeft,
                     float* reverbRight,
                     int numSamples) noexcept
{
    if (left == nullptr || right == nullptr || numSamples <= 0 || context.bpm <= 0.0
        || context.outputSampleRate <= 0.0 || asset.sampleRate() <= 0.0
        || clip.lengthBeats <= 0.0 || clip.sourceLengthSeconds <= 0.0)
        return;

    const auto& source = asset.buffer();
    if (source.getNumChannels() <= 0 || source.getNumSamples() <= 0)
        return;

    const double beatsPerOutputSample = context.bpm / (60.0 * context.outputSampleRate);
    const float pan = juce::jlimit(-1.0f, 1.0f, context.pan);
    const float angle = (pan + 1.0f) * juce::MathConstants<float>::quarterPi;
    const float panL = std::cos(angle);
    const float panR = std::sin(angle);
    const float gain = static_cast<float>(sensei::core::dbToLinear(clip.gainDb))
                       * juce::jmax(0.0f, context.trackGainLinear);
    const float send = juce::jlimit(0.0f, 1.0f, context.reverbSend);

    for (int outSample = 0; outSample < numSamples; ++outSample)
    {
        const double beat = context.projectBeatStart + outSample * beatsPerOutputSample;
        const double clipBeat = beat - clip.startBeat;
        if (clipBeat < 0.0 || clipBeat >= clip.lengthBeats)
            continue;

        const double normalized = clip.lengthBeats > 0.0 ? clipBeat / clip.lengthBeats : 0.0;
        double secondsIntoClip = normalized * clip.sourceLengthSeconds;
        secondsIntoClip = std::clamp(secondsIntoClip, 0.0, clip.sourceLengthSeconds);

        const double sourceSeconds = clip.reversed
            ? clip.sourceOffsetSeconds + (clip.sourceLengthSeconds - secondsIntoClip)
            : clip.sourceOffsetSeconds + secondsIntoClip;
        const double sourceSample = sourceSeconds * asset.sampleRate();

        const float srcL = interpolatedSample(source, 0, sourceSample);
        const float srcR = source.getNumChannels() > 1
            ? interpolatedSample(source, 1, sourceSample)
            : srcL;
        const float env = clipEnvelope(clip, secondsIntoClip);
        const float mixedL = srcL * gain * env * panL;
        const float mixedR = srcR * gain * env * panR;

        left[outSample] += mixedL;
        right[outSample] += mixedR;
        if (reverbLeft != nullptr)
            reverbLeft[outSample] += mixedL * send;
        if (reverbRight != nullptr)
            reverbRight[outSample] += mixedR * send;
    }
}

} // namespace sensei::engine
