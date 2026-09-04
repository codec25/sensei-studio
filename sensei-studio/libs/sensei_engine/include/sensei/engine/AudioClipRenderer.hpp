#pragma once

#include "sensei/core/AudioClip.hpp"
#include "sensei/engine/AudioFileAsset.hpp"

#include <juce_audio_basics/juce_audio_basics.h>

namespace sensei::engine {

struct AudioRenderContext
{
    double projectBeatStart = 0.0;
    double bpm = 120.0;
    double outputSampleRate = 44100.0;
    float trackGainLinear = 1.0f;
    float pan = 0.0f;       // -1 left, +1 right
    float reverbSend = 0.0f;
};

// Adds one clip into the destination buffers. The source asset must already be
// decoded; no allocation, file I/O or locks are performed here.
void renderAudioClip(const sensei::core::AudioClip& clip,
                     const AudioFileAsset& asset,
                     const AudioRenderContext& context,
                     float* left,
                     float* right,
                     float* reverbLeft,
                     float* reverbRight,
                     int numSamples) noexcept;

} // namespace sensei::engine
