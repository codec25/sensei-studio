#pragma once

#include "sensei/core/Project.hpp"
#include "sensei/core/bass/BassGenerator.hpp"
#include "sensei/core/sensei/Observation.hpp"

#include <string>
#include <vector>

namespace sensei::core {

enum class LessonStep
{
    AskStart,
    ChooseKey,
    ChooseProgression,
    OfferDrums,
    OfferBass,
    CelebrateLoop,
    Quiet
};

enum class UserChoice
{
    LikeIt,
    DoSomething,
    Why,
    Later
};

enum class LearningEventKind
{
    ChordProgressionCreated,
    DrumPatternCreated,
    RootBassAdded,
    UserModifiedGenerated,
    FirstCompleteLoop
};

struct LearningEvent
{
    LearningEventKind kind {};
    std::string title;
    std::string fact;
    std::string advice;
};

struct LessonState
{
    LessonStep step = LessonStep::AskStart;
    bool chordsAccepted = false;
    bool drumsAccepted = false;
    bool bassAccepted = false;
    bool celebratedCompleteLoop = false;
    bool userModifiedGeneratedEmitted = false;
    bool quiet = false;
    std::string lastWhy;
    std::vector<LearningEvent> events;
};

[[nodiscard]] inline bool trackHasNotes(const Track& track) noexcept
{
    if (track.type == TrackType::Drums)
        return ! track.drumPattern.hits.empty();
    for (const auto& clip : track.clips)
        if (! clip.notes.empty())
            return true;
    return false;
}

[[nodiscard]] inline bool isCompleteLoop(const Project& project) noexcept
{
    bool chords = false, bass = false, drums = false;
    for (const auto& track : project.tracks())
    {
        if (track.role == TrackRole::Chords && trackHasNotes(track))
            chords = true;
        if (track.role == TrackRole::Bass && trackHasNotes(track))
            bass = true;
        if (track.role == TrackRole::Drums && trackHasNotes(track))
            drums = true;
    }
    return chords && bass && drums;
}

inline LearningEvent makeEvent(LearningEventKind kind)
{
    switch (kind)
    {
        case LearningEventKind::ChordProgressionCreated:
            return { kind, "Chord progression created",
                     "You placed a 4-bar harmonic loop in the Chords track.",
                     "Listen before changing anything — harmony is the map." };
        case LearningEventKind::DrumPatternCreated:
            return { kind, "First drum pattern created",
                     "Kick, snare, and hats now outline the groove.",
                     "If it feels busy, remove hats before touching the kick." };
        case LearningEventKind::RootBassAdded:
            return { kind, "Root-note bass added",
                     "Bass is following the chord roots.",
                     std::string(kRootBassExplanation) };
        case LearningEventKind::UserModifiedGenerated:
            return { kind, "You changed a generated idea",
                     "Sensei notices you edited material that started as a suggestion.",
                     "That is the point — generate, then make it yours." };
        case LearningEventKind::FirstCompleteLoop:
            return { kind, "First complete 4-bar loop",
                     "Chords, drums, and bass are all present inside the loop.",
                     "If you like it, keep it. Sensei will stay quiet unless you ask." };
    }
    return {};
}

inline Observation observationFromLesson(const LessonState& lesson, const Project& project)
{
    if (! lesson.events.empty())
    {
        const auto& e = lesson.events.back();
        Observation o;
        o.title = e.title;
        o.fact = e.fact;
        o.advice = e.advice;
        switch (e.kind)
        {
            case LearningEventKind::FirstCompleteLoop: o.kind = ObservationKind::FirstCompleteLoop; break;
            case LearningEventKind::ChordProgressionCreated: o.kind = ObservationKind::ChordProgressionCreated; break;
            case LearningEventKind::DrumPatternCreated: o.kind = ObservationKind::DrumPatternCreated; break;
            case LearningEventKind::RootBassAdded: o.kind = ObservationKind::RootBassAdded; break;
            case LearningEventKind::UserModifiedGenerated: o.kind = ObservationKind::UserModifiedGenerated; break;
        }
        return o;
    }

    if (isCompleteLoop(project))
    {
        return { ObservationKind::FirstCompleteLoop, "First complete 4-bar loop",
                 "Chords, drums, and bass are all present inside the loop.",
                 "If you like it, keep it." };
    }

    return { ObservationKind::NoNotes, "Let’s build a 4-bar idea",
             "Start with a key and a simple chord progression.",
             "Create mode stays quiet until you ask for the next step." };
}

} // namespace sensei::core
