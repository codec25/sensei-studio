#pragma once

#include "sensei/core/Id.hpp"
#include "sensei/core/Types.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace sensei::core {

enum class SectionLabel
{
    Intro,
    Verse,
    Chorus,
    Build,
    Drop,
    Outro,
    Custom
};

struct Section
{
    Id id = kInvalidId;
    std::string name;
    SectionLabel label = SectionLabel::Custom;
    double startBeat = 0.0;
    double lengthBeats = kDefaultLoopBeats;

    [[nodiscard]] double endBeat() const noexcept { return startBeat + lengthBeats; }
};

[[nodiscard]] inline const char* sectionLabelName(SectionLabel label) noexcept
{
    switch (label)
    {
        case SectionLabel::Intro: return "Intro";
        case SectionLabel::Verse: return "Verse";
        case SectionLabel::Chorus: return "Chorus";
        case SectionLabel::Build: return "Build";
        case SectionLabel::Drop: return "Drop";
        case SectionLabel::Outro: return "Outro";
        case SectionLabel::Custom: return "Custom";
    }
    return "Custom";
}

// Milestone D rule: named song sections must not overlap.
// Ranges are half-open [start, end). Touching endpoints are allowed.
[[nodiscard]] inline bool sectionsOverlap(double aStart, double aLength,
                                          double bStart, double bLength) noexcept
{
    if (! (aLength > 0.0) || ! (bLength > 0.0))
        return false;
    const double aEnd = aStart + aLength;
    const double bEnd = bStart + bLength;
    return aStart < bEnd - 1.0e-9 && bStart < aEnd - 1.0e-9;
}

[[nodiscard]] inline bool sectionRangeConflicts(const std::vector<Section>& sections,
                                                double startBeat,
                                                double lengthBeats,
                                                Id ignoreId = kInvalidId) noexcept
{
    for (const auto& section : sections)
    {
        if (ignoreId != kInvalidId && section.id == ignoreId)
            continue;
        if (sectionsOverlap(startBeat, lengthBeats, section.startBeat, section.lengthBeats))
            return true;
    }
    return false;
}

[[nodiscard]] inline double derivedSongLengthBeats(const std::vector<Section>& sections,
                                                   double fallbackBeats = kDefaultLoopBeats) noexcept
{
    double end = 0.0;
    for (const auto& section : sections)
        end = std::max(end, section.endBeat());
    if (end > 0.0)
        return end;
    return fallbackBeats > 0.0 ? fallbackBeats : kDefaultLoopBeats;
}

} // namespace sensei::core
