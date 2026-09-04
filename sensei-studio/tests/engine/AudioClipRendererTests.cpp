#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/AudioClip.hpp"
#include "sensei/engine/AudioClipRenderer.hpp"
#include "sensei/engine/AudioFileAsset.hpp"

#include <array>

using namespace sensei::core;
using namespace sensei::engine;

TEST_CASE("Audio asset builds deterministic waveform peaks", "[audio][waveform]")
{
    juce::AudioBuffer<float> source(1, 8);
    const float values[] { -1.0f, -0.5f, 0.25f, 0.75f, -0.25f, 0.5f, -0.75f, 1.0f };
    source.copyFrom(0, 0, values, 8);

    auto asset = AudioFileAsset::fromBuffer(source, 8.0, 4);
    REQUIRE(asset != nullptr);
    REQUIRE(asset->durationSeconds() == Catch::Approx(1.0));
    REQUIRE(asset->waveform().size() == 4);
    REQUIRE(asset->waveform()[0].min == Catch::Approx(-1.0f));
    REQUIRE(asset->waveform()[0].max == Catch::Approx(-0.5f));
    REQUIRE(asset->waveform()[3].min == Catch::Approx(-0.75f));
    REQUIRE(asset->waveform()[3].max == Catch::Approx(1.0f));
}

TEST_CASE("Audio clip renderer respects trim window gain pan and send", "[audio][render]")
{
    juce::AudioBuffer<float> source(1, 8);
    source.clear();
    for (int i = 0; i < 8; ++i)
        source.setSample(0, i, static_cast<float>(i) / 8.0f);

    auto asset = AudioFileAsset::fromBuffer(source, 8.0, 8);
    REQUIRE(asset != nullptr);

    AudioClip clip;
    clip.startBeat = 0.0;
    clip.lengthBeats = 2.0;
    clip.sourceOffsetSeconds = 0.25; // starts at source sample 2
    clip.sourceLengthSeconds = 0.5; // source samples 2..6
    clip.fadeIn.lengthSeconds = 0.0;
    clip.fadeOut.lengthSeconds = 0.0;
    clip.gainDb = 0.0;

    std::array<float, 8> left {};
    std::array<float, 8> right {};
    std::array<float, 8> sendL {};
    std::array<float, 8> sendR {};

    AudioRenderContext context;
    context.projectBeatStart = 0.0;
    context.bpm = 120.0;
    context.outputSampleRate = 8.0;
    context.trackGainLinear = 1.0f;
    context.pan = 0.0f;
    context.reverbSend = 0.5f;

    renderAudioClip(clip, *asset, context,
                    left.data(), right.data(), sendL.data(), sendR.data(), 8);

    REQUIRE(left[0] > 0.0f);
    REQUIRE(left[0] == Catch::Approx(right[0]));
    REQUIRE(sendL[0] == Catch::Approx(left[0] * 0.5f));
    REQUIRE(sendR[0] == Catch::Approx(right[0] * 0.5f));
}

TEST_CASE("Audio clip renderer applies edge fades", "[audio][fade]")
{
    juce::AudioBuffer<float> source(1, 16);
    for (int i = 0; i < 16; ++i)
        source.setSample(0, i, 1.0f);

    auto asset = AudioFileAsset::fromBuffer(source, 16.0, 8);
    REQUIRE(asset != nullptr);

    AudioClip clip;
    clip.startBeat = 0.0;
    clip.lengthBeats = 2.0;
    clip.sourceOffsetSeconds = 0.0;
    clip.sourceLengthSeconds = 1.0;
    clip.fadeIn = { 0.25, AudioFadeCurve::Linear };
    clip.fadeOut = { 0.25, AudioFadeCurve::Linear };

    std::array<float, 8> left {};
    std::array<float, 8> right {};
    AudioRenderContext context;
    context.projectBeatStart = 0.0;
    context.bpm = 120.0;
    context.outputSampleRate = 8.0;

    renderAudioClip(clip, *asset, context, left.data(), right.data(), nullptr, nullptr, 8);

    REQUIRE(left.front() == Catch::Approx(0.0f).margin(0.0001f));
    REQUIRE(left[3] > left[1]);
    REQUIRE(left[7] < left[5]);
}
