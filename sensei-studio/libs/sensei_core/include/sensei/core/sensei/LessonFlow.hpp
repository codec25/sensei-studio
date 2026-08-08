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
    OfferSongShape,
    OfferVariation,
    CelebrateSong,
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
    FirstCompleteLoop,
    FirstArrangementCreated,
    LoopDuplicated,
    FirstVariationCreated,
    IntroCreated,
    ContrastIntroduced,
    FirstFullSongStructureCreated
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
    bool songShapeAccepted = false;
    bool variationAccepted = false;
    bool celebratedSong = false;
    bool userModifiedGeneratedEmitted = false;
    bool quiet = false;
    std::string lastWhy;
    std::vector<LearningEvent> events;
};

[[nodiscard]] inline bool trackHasNotes(const Track& track) noexcept
{
    if (track.type == TrackType::Drums)
    {
        for (const auto& clip : track.drumClips)
            if (! clip.pattern.hits.empty())
                return true;
        return false;
    }
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

[[nodiscard]] inline bool hasFullSongStructure(const Project& project) noexcept
{
    bool intro = false, main = false, variation = false, outro = false;
    for (const auto& s : project.sections())
    {
        if (s.label == SectionLabel::Intro || s.name == "Intro")
            intro = true;
        if (s.name == "Main" || s.label == SectionLabel::Chorus)
            main = true;
        if (s.name == "Variation")
            variation = true;
        if (s.label == SectionLabel::Outro || s.name == "Outro")
            outro = true;
    }
    return intro && main && variation && outro;
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
                     "If you like it, keep it — or turn it into a simple song shape." };
        case LearningEventKind::FirstArrangementCreated:
            return { kind, "First arrangement created",
                     "Your loop now lives on a longer timeline with named sections.",
                     "Sections are a map. Clips are what you hear." };
        case LearningEventKind::LoopDuplicated:
            return { kind, "Loop duplicated",
                     "The 4-bar idea was repeated to fill longer sections without stretching timing.",
                     "Repetition is how short ideas become song-length." };
        case LearningEventKind::FirstVariationCreated:
            return { kind, "First variation created",
                     "One section now differs from the others in a small, audible way.",
                     "Contrast helps listeners feel a beginning, middle, and end." };
        case LearningEventKind::IntroCreated:
            return { kind, "Intro created",
                     "The song has a dedicated opening section before the main idea.",
                     "Intros can be thinner so the main section feels bigger." };
        case LearningEventKind::ContrastIntroduced:
            return { kind, "Contrast introduced",
                     "Something is quieter, thinner, or enters later across sections.",
                     "Contrast helps listeners feel a beginning, middle, and end." };
        case LearningEventKind::FirstFullSongStructureCreated:
            return { kind, "First structured song",
                     "Intro, Main, Variation, and Outro are in place on the timeline.",
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
            case LearningEventKind::FirstArrangementCreated: o.kind = ObservationKind::FirstArrangementCreated; break;
            case LearningEventKind::LoopDuplicated: o.kind = ObservationKind::LoopDuplicated; break;
            case LearningEventKind::FirstVariationCreated: o.kind = ObservationKind::FirstVariationCreated; break;
            case LearningEventKind::IntroCreated: o.kind = ObservationKind::IntroCreated; break;
            case LearningEventKind::ContrastIntroduced: o.kind = ObservationKind::ContrastIntroduced; break;
            case LearningEventKind::FirstFullSongStructureCreated: o.kind = ObservationKind::FirstFullSongStructureCreated; break;
        }
        return o;
    }

    if (hasFullSongStructure(project))
    {
        return { ObservationKind::FirstFullSongStructureCreated, "First structured song",
                 "Intro, Main, Variation, and Outro are in place on the timeline.",
                 "If you like it, keep it." };
    }

    if (isCompleteLoop(project))
    {
        return { ObservationKind::FirstCompleteLoop, "First complete 4-bar loop",
                 "Chords, drums, and bass are all present inside the loop.",
                 "Want to turn this loop into a simple song shape?" };
    }

    return { ObservationKind::NoNotes, "Let’s build a 4-bar idea",
             "Start with a key and a simple chord progression.",
             "Create mode stays quiet until you ask for the next step." };
}

} // namespace sensei::core
