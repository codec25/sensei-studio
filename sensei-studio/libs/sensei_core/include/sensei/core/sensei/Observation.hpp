#pragma once

#include <string>

namespace sensei::core {

enum class ObservationKind
{
    NoNotes,
    FirstIdea,
    NotesOutsideLoop,
    ChordDetected,
    LowPitchVariety,
    LoopHasMaterial,
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
    FirstFullSongStructureCreated,
    InstrumentIdentity
};

struct Observation
{
    ObservationKind kind = ObservationKind::NoNotes;
    std::string title;
    std::string fact;
    std::string advice;
};

} // namespace sensei::core
