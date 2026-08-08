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
    LoopHasMaterial
};

struct Observation
{
    ObservationKind kind = ObservationKind::NoNotes;
    std::string title;
    std::string fact;
    std::string advice; // optional soft suggestion; may be empty
};

} // namespace sensei::core
