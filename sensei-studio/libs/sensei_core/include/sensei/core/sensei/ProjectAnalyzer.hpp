#pragma once

#include "sensei/core/Project.hpp"
#include "sensei/core/sensei/Observation.hpp"

#include <cmath>
#include <set>
#include <vector>

namespace sensei::core {

class ProjectAnalyzer
{
public:
    [[nodiscard]] static Observation analyze(const Project& project)
    {
        const auto* clip = project.primaryClip();
        const auto& loop = project.loop();
        const std::size_t noteCount = project.totalNoteCount();

        if (noteCount == 0 || clip == nullptr)
        {
            return {
                ObservationKind::NoNotes,
                "No notes yet",
                "The MIDI clip is empty.",
                "Click in the piano roll to place your first note."
            };
        }

        if (noteCount <= 3)
        {
            return {
                ObservationKind::FirstIdea,
                "First musical idea created",
                "You currently have " + std::to_string(noteCount) + " MIDI note"
                    + (noteCount == 1 ? "" : "s") + ".",
                "Try adding a few more notes before judging the idea."
            };
        }

        bool outside = false;
        for (const auto& note : clip->notes)
        {
            if (note.startBeat < loop.startBeat
                || note.endBeat() > loop.startBeat + loop.lengthBeats + 1.0e-9)
            {
                outside = true;
                break;
            }
        }

        if (outside)
        {
            return {
                ObservationKind::NotesOutsideLoop,
                "Notes extend outside the loop",
                "At least one note starts or ends outside the current loop region.",
                "That can be intentional. If you want a clean loop, keep material inside the loop."
            };
        }

        if (hasChord(clip->notes))
        {
            return {
                ObservationKind::ChordDetected,
                "Chord detected",
                "Three or more different pitches overlap in time.",
                {}
            };
        }

        std::set<int> pitches;
        for (const auto& note : clip->notes)
            pitches.insert(note.pitch);

        if (noteCount >= 6 && pitches.size() <= 2)
        {
            return {
                ObservationKind::LowPitchVariety,
                "Very little pitch variation",
                "There are " + std::to_string(noteCount) + " notes using only "
                    + std::to_string(pitches.size()) + " pitch"
                    + (pitches.size() == 1 ? "" : "es") + ".",
                "If you want more movement, change only the ending of the phrase."
            };
        }

        return {
            ObservationKind::LoopHasMaterial,
            "Loop contains musical material",
            "The loop has " + std::to_string(noteCount) + " notes across "
                + std::to_string(pitches.size()) + " pitches.",
            {}
        };
    }

private:
    static bool hasChord(const std::vector<MidiNote>& notes)
    {
        for (std::size_t i = 0; i < notes.size(); ++i)
        {
            std::set<int> overlapping;
            overlapping.insert(notes[i].pitch);

            for (std::size_t j = 0; j < notes.size(); ++j)
            {
                if (i == j)
                    continue;

                const bool overlaps = notes[i].startBeat < notes[j].endBeat() - 1.0e-9
                                      && notes[j].startBeat < notes[i].endBeat() - 1.0e-9;
                if (overlaps)
                    overlapping.insert(notes[j].pitch);
            }

            if (overlapping.size() >= 3)
                return true;
        }
        return false;
    }
};

} // namespace sensei::core
