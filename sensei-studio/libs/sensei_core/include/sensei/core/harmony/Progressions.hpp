#pragma once

#include "sensei/core/MidiNote.hpp"
#include "sensei/core/harmony/Chord.hpp"

#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace sensei::core {

struct ProgressionTemplate
{
    const char* id;
    const char* displayName;
    const char* explanation;
    std::array<const char*, 4> romans;
};

inline constexpr ProgressionTemplate kProgressions[] {
    { "I-V-vi-IV", "I – V – vi – IV",
      "A warm pop progression: home, lift, emotion, then settle.",
      { "I", "V", "vi", "IV" } },
    { "vi-IV-I-V", "vi – IV – I – V",
      "Starts emotional, then climbs back toward home and the dominant.",
      { "vi", "IV", "I", "V" } },
    { "I-IV-V-I", "I – IV – V – I",
      "A classic beginner loop: home, away, stronger away, home again.",
      { "I", "IV", "V", "I" } },
};

inline constexpr int kProgressionCount = 3;

[[nodiscard]] inline int romanToDegree(const char* roman) noexcept
{
    if (roman == nullptr)
        return 1;
    if (std::strcmp(roman, "I") == 0 || std::strcmp(roman, "i") == 0) return 1;
    if (std::strcmp(roman, "ii") == 0 || std::strcmp(roman, "II") == 0) return 2;
    if (std::strcmp(roman, "iii") == 0 || std::strcmp(roman, "III") == 0) return 3;
    if (std::strcmp(roman, "IV") == 0 || std::strcmp(roman, "iv") == 0) return 4;
    if (std::strcmp(roman, "V") == 0 || std::strcmp(roman, "v") == 0) return 5;
    if (std::strcmp(roman, "vi") == 0 || std::strcmp(roman, "VI") == 0) return 6;
    if (std::strcmp(roman, "vii") == 0 || std::strcmp(roman, "VII") == 0) return 7;
    return 1;
}

[[nodiscard]] inline bool romanIsMinorQuality(const char* roman) noexcept
{
    return roman != nullptr && roman[0] >= 'a' && roman[0] <= 'z';
}

[[nodiscard]] inline std::array<int, 3> triadForDegree(int rootPc,
                                                       ScaleMode mode,
                                                       int degree,
                                                       int baseOctaveMidi = 60) noexcept
{
    static constexpr int majorDegrees[] { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minorDegrees[] { 0, 2, 3, 5, 7, 8, 10 };
    const int* scale = mode == ScaleMode::Major ? majorDegrees : minorDegrees;
    const int d = ((degree - 1) % 7 + 7) % 7;
    const int thirdStep = (d + 2) % 7;
    const int fifthStep = (d + 4) % 7;

    auto absPc = [&](int step) { return (rootPc + scale[step]) % 12; };
    int root = baseOctaveMidi - (baseOctaveMidi % 12) + absPc(d);
    int third = baseOctaveMidi - (baseOctaveMidi % 12) + absPc(thirdStep);
    int fifth = baseOctaveMidi - (baseOctaveMidi % 12) + absPc(fifthStep);
    if (third < root)
        third += 12;
    if (fifth < third)
        fifth += 12;
    return { root, third, fifth };
}

[[nodiscard]] inline std::string chordNameFor(int rootPc, ScaleMode mode, const char* roman)
{
    const int degree = romanToDegree(roman);
    const auto triad = triadForDegree(rootPc, mode, degree, 60);
    std::string name = pitchClassName(((triad[0] % 12) + 12) % 12);
    if (romanIsMinorQuality(roman))
        name += "m";
    return name;
}

struct GeneratedChordMaterial
{
    HarmonyState harmony;
    std::vector<MidiNote> notes;
};

[[nodiscard]] inline const ProgressionTemplate* findProgression(const char* id) noexcept
{
    for (const auto& p : kProgressions)
        if (std::strcmp(p.id, id) == 0)
            return &p;
    return nullptr;
}

[[nodiscard]] inline GeneratedChordMaterial generateProgression(int rootPc,
                                                                ScaleMode mode,
                                                                const char* progressionId,
                                                                double loopBeats = kDefaultLoopBeats)
{
    GeneratedChordMaterial out;
    out.harmony.rootPitchClass = ((rootPc % 12) + 12) % 12;
    out.harmony.mode = mode;

    const auto* prog = findProgression(progressionId);
    if (prog == nullptr)
        prog = &kProgressions[0];

    out.harmony.progressionId = prog->id;
    const double chordLen = loopBeats / 4.0;
    Id noteIdSeed = 1; // caller reassigns via Project::generateId

    for (int i = 0; i < 4; ++i)
    {
        ChordEvent ev;
        ev.startBeat = static_cast<double>(i) * chordLen;
        ev.lengthBeats = chordLen;
        ev.roman = prog->romans[static_cast<std::size_t>(i)];
        ev.pitches = triadForDegree(out.harmony.rootPitchClass, mode, romanToDegree(ev.roman.c_str()), 60);
        ev.chordName = chordNameFor(out.harmony.rootPitchClass, mode, ev.roman.c_str());
        out.harmony.chords.push_back(ev);

        for (int p = 0; p < 3; ++p)
        {
            MidiNote note;
            note.id = noteIdSeed++; // temporary; replaced on apply
            note.pitch = ev.pitches[static_cast<std::size_t>(p)];
            note.startBeat = ev.startBeat;
            note.lengthBeats = ev.lengthBeats;
            note.velocity = 0.75f;
            out.notes.push_back(note);
        }
    }

    return out;
}

} // namespace sensei::core
