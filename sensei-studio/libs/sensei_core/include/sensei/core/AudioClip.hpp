#pragma once

#include "sensei/core/Id.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace sensei::core {

// F.2C audio editing contract. Source time is stored in seconds so trimming is
// independent of project tempo; arrangement placement remains beat-based.
enum class AudioFadeCurve
{
    Linear,
    EqualPower,
    SCurve
};

struct AudioFade
{
    double lengthSeconds = 0.004; // tiny default edge fade prevents clicks
    AudioFadeCurve curve = AudioFadeCurve::Linear;
};

struct AudioClip
{
    Id id = kInvalidId;
    std::string name;
    std::string sourcePath;

    double startBeat = 0.0;
    double lengthBeats = 0.0;

    // Non-destructive window into the source file.
    double sourceOffsetSeconds = 0.0;
    double sourceLengthSeconds = 0.0;

    double gainDb = 0.0;
    bool reversed = false;
    AudioFade fadeIn {};
    AudioFade fadeOut {};
};

[[nodiscard]] inline double clamp01(double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

[[nodiscard]] inline double dbToLinear(double db) noexcept
{
    return std::pow(10.0, db / 20.0);
}

// Fade gain is normalized 0..1. This is deliberately deterministic Core math;
// the audio engine later evaluates the same contract sample-accurately.
[[nodiscard]] inline double fadeGain(double position01, AudioFadeCurve curve) noexcept
{
    const double t = clamp01(position01);
    switch (curve)
    {
        case AudioFadeCurve::EqualPower:
            return std::sin(t * 1.57079632679489661923); // pi / 2
        case AudioFadeCurve::SCurve:
            return t * t * (3.0 - 2.0 * t);
        case AudioFadeCurve::Linear:
        default:
            return t;
    }
}

struct CrossfadeGains
{
    double outgoing = 1.0;
    double incoming = 0.0;
};

// Equal-power is the musical default because two correlated-ish clips are less
// likely to produce the obvious centre dip of two linear fades.
[[nodiscard]] inline CrossfadeGains crossfadeGains(double position01,
                                                    AudioFadeCurve curve = AudioFadeCurve::EqualPower) noexcept
{
    const double t = clamp01(position01);
    if (curve == AudioFadeCurve::EqualPower)
        return { std::cos(t * 1.57079632679489661923),
                 std::sin(t * 1.57079632679489661923) };

    if (curve == AudioFadeCurve::SCurve)
    {
        const double in = fadeGain(t, AudioFadeCurve::SCurve);
        return { 1.0 - in, in };
    }

    return { 1.0 - t, t };
}

inline void sanitizeAudioClip(AudioClip& clip) noexcept
{
    clip.startBeat = std::max(0.0, clip.startBeat);
    clip.lengthBeats = std::max(0.0, clip.lengthBeats);
    clip.sourceOffsetSeconds = std::max(0.0, clip.sourceOffsetSeconds);
    clip.sourceLengthSeconds = std::max(0.0, clip.sourceLengthSeconds);

    const double maxFade = clip.sourceLengthSeconds * 0.5;
    clip.fadeIn.lengthSeconds = std::clamp(clip.fadeIn.lengthSeconds, 0.0, maxFade);
    clip.fadeOut.lengthSeconds = std::clamp(clip.fadeOut.lengthSeconds, 0.0, maxFade);

    // Start/end fades may touch but never overlap.
    const double fadeTotal = clip.fadeIn.lengthSeconds + clip.fadeOut.lengthSeconds;
    if (fadeTotal > clip.sourceLengthSeconds && fadeTotal > 0.0)
    {
        const double scale = clip.sourceLengthSeconds / fadeTotal;
        clip.fadeIn.lengthSeconds *= scale;
        clip.fadeOut.lengthSeconds *= scale;
    }
}

// Trim helpers are non-destructive: they move the source window and timeline
// edge without editing the underlying file. bpm is only used to translate the
// arrangement edge movement into source seconds.
inline void trimAudioClipStart(AudioClip& clip, double deltaBeats, double bpm) noexcept
{
    if (!(bpm > 0.0) || !(deltaBeats > 0.0) || !(clip.lengthBeats > 0.0))
        return;

    const double clampedBeats = std::min(deltaBeats, clip.lengthBeats);
    const double seconds = clampedBeats * 60.0 / bpm;
    const double availableSeconds = std::min(seconds, clip.sourceLengthSeconds);
    const double actualBeats = availableSeconds * bpm / 60.0;

    clip.startBeat += actualBeats;
    clip.lengthBeats = std::max(0.0, clip.lengthBeats - actualBeats);
    clip.sourceOffsetSeconds += availableSeconds;
    clip.sourceLengthSeconds = std::max(0.0, clip.sourceLengthSeconds - availableSeconds);
    sanitizeAudioClip(clip);
}

inline void trimAudioClipEnd(AudioClip& clip, double deltaBeats, double bpm) noexcept
{
    if (!(bpm > 0.0) || !(deltaBeats > 0.0) || !(clip.lengthBeats > 0.0))
        return;

    const double clampedBeats = std::min(deltaBeats, clip.lengthBeats);
    const double seconds = clampedBeats * 60.0 / bpm;
    const double availableSeconds = std::min(seconds, clip.sourceLengthSeconds);
    const double actualBeats = availableSeconds * bpm / 60.0;

    clip.lengthBeats = std::max(0.0, clip.lengthBeats - actualBeats);
    clip.sourceLengthSeconds = std::max(0.0, clip.sourceLengthSeconds - availableSeconds);
    sanitizeAudioClip(clip);
}

} // namespace sensei::core
