#pragma once

#include "sensei/core/MidiNote.hpp"
#include "sensei/core/harmony/Chord.hpp"

#include <algorithm>
#include <vector>

namespace sensei::core {

inline constexpr const char* kRootBassExplanation =
    "Root-note bass is a safe beginner starting point: it locks the harmony without clutter.";

[[nodiscard]] inline std::vector<MidiNote> generateRootBass(const HarmonyState& harmony,
                                                            int octaveMidi = 36)
{
    std::vector<MidiNote> notes;
    Id tmp = 1;
    for (const auto& chord : harmony.chords)
    {
        MidiNote note;
        note.id = tmp++;
        const int pc = ((chord.pitches[0] % 12) + 12) % 12;
        int pitch = octaveMidi - (octaveMidi % 12) + pc;
        while (pitch < 28)
            pitch += 12;
        while (pitch > 48)
            pitch -= 12;
        note.pitch = pitch;
        note.startBeat = chord.startBeat;
        note.lengthBeats = std::max(0.25, chord.lengthBeats);
        note.velocity = 0.85f;
        notes.push_back(note);
    }
    return notes;
}

} // namespace sensei::core
