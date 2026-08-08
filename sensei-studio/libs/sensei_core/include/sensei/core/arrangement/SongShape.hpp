#pragma once

#include "sensei/core/Project.hpp"
#include "sensei/core/Section.hpp"
#include "sensei/core/commands/ArrangementCommands.hpp"
#include "sensei/core/commands/CompoundCommand.hpp"
#include "sensei/core/commands/TrackContentCommands.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace sensei::core {

inline constexpr double kBarsToBeats(int bars) noexcept
{
    return static_cast<double>(bars * kBeatsPerBar);
}

struct SongShapePlan
{
    static constexpr int kIntroBars = 4;
    static constexpr int kMainBars = 8;
    static constexpr int kVariationBars = 8;
    static constexpr int kOutroBars = 4;

    double introStart = 0.0;
    double introLength = kBarsToBeats(kIntroBars);
    double mainStart = kBarsToBeats(kIntroBars);
    double mainLength = kBarsToBeats(kMainBars);
    double variationStart = kBarsToBeats(kIntroBars + kMainBars);
    double variationLength = kBarsToBeats(kVariationBars);
    double outroStart = kBarsToBeats(kIntroBars + kMainBars + kVariationBars);
    double outroLength = kBarsToBeats(kOutroBars);

    [[nodiscard]] double songLengthBeats() const noexcept
    {
        return outroStart + outroLength;
    }
};

[[nodiscard]] inline const char* kContrastWhy() noexcept
{
    return "Contrast helps listeners feel a beginning, middle, and end — "
           "same idea, different weight.";
}

// Thin a drum pattern by removing one lane (default: closed hat).
[[nodiscard]] inline DrumPattern thinDrumPattern(const DrumPattern& source,
                                                 DrumLane removeLane = DrumLane::ClosedHat)
{
    DrumPattern out = source;
    out.id = kInvalidId;
    out.hits.erase(std::remove_if(out.hits.begin(), out.hits.end(),
                                  [removeLane](const DrumHit& h) { return h.lane == removeLane; }),
                   out.hits.end());
    return out;
}

// Simplify chord clip notes: keep roots only (lowest pitch per onset group).
[[nodiscard]] inline std::vector<MidiNote> simplifyChordNotes(const std::vector<MidiNote>& notes)
{
    std::vector<MidiNote> sorted = notes;
    std::sort(sorted.begin(), sorted.end(), [](const MidiNote& a, const MidiNote& b) {
        if (std::abs(a.startBeat - b.startBeat) > 1.0e-9)
            return a.startBeat < b.startBeat;
        return a.pitch < b.pitch;
    });

    std::vector<MidiNote> out;
    for (const auto& note : sorted)
    {
        if (! out.empty() && std::abs(out.back().startBeat - note.startBeat) < 1.0e-6)
            continue; // keep lowest pitch already pushed
        auto copy = note;
        copy.id = kInvalidId;
        out.push_back(copy);
    }
    return out;
}

namespace detail {

inline void duplicateSeedToFill(CompoundCommand& compound,
                                Project& project,
                                Id trackId,
                                Id seedClipId,
                                bool isDrum,
                                double sectionStart,
                                double sectionLength,
                                double seedLength)
{
    if (! (seedLength > 0.0) || ! (sectionLength > 0.0))
        return;

    // Repeat the seed clip without time-stretching until the section is filled.
    for (double t = sectionStart; t + 1.0e-9 < sectionStart + sectionLength; t += seedLength)
    {
        // Skip if a clip already occupies this start (the original seed at 0).
        bool exists = false;
        if (const auto* track = project.findTrack(trackId))
        {
            if (isDrum)
            {
                for (const auto& c : track->drumClips)
                    if (std::abs(c.startBeat - t) < 1.0e-6)
                        exists = true;
            }
            else
            {
                for (const auto& c : track->clips)
                    if (std::abs(c.startBeat - t) < 1.0e-6)
                        exists = true;
            }
        }
        if (exists)
            continue;
        compound.add(std::make_unique<DuplicateClipCommand>(trackId, seedClipId, t));
    }
}

} // namespace detail

// Builds a compound command that creates Intro/Main/Variation/Outro and
// repeats the current 4-bar seed clips to fill longer sections (no stretch).
[[nodiscard]] inline std::unique_ptr<CompoundCommand> makeApplySongShapeCommand(Project& project)
{
    const SongShapePlan plan;
    auto compound = std::make_unique<CompoundCommand>("Turn loop into a song");

    compound->add(std::make_unique<CreateSectionCommand>(
        "Intro", SectionLabel::Intro, plan.introStart, plan.introLength));
    compound->add(std::make_unique<CreateSectionCommand>(
        "Main", SectionLabel::Chorus, plan.mainStart, plan.mainLength));
    compound->add(std::make_unique<CreateSectionCommand>(
        "Variation", SectionLabel::Verse, plan.variationStart, plan.variationLength));
    compound->add(std::make_unique<CreateSectionCommand>(
        "Outro", SectionLabel::Outro, plan.outroStart, plan.outroLength));

    compound->add(std::make_unique<SetSongLengthCommand>(plan.songLengthBeats()));
    // Whole-song playback by default; loop region preserved as Main for optional re-enable.
    compound->add(std::make_unique<SetLoopRegionCommand>(plan.mainStart, plan.mainLength, false));

    const double seedLength = kDefaultLoopBeats;

    for (auto& track : project.tracks())
    {
        if (track.type == TrackType::Midi)
        {
            if (track.clips.empty())
                continue;
            // Prefer the clip at beat 0 as the seed.
            Id seedId = track.clips.front().id;
            double seedLen = track.clips.front().lengthBeats;
            for (const auto& clip : track.clips)
            {
                if (std::abs(clip.startBeat) < 1.0e-6)
                {
                    seedId = clip.id;
                    seedLen = clip.lengthBeats > 0.0 ? clip.lengthBeats : seedLength;
                    break;
                }
            }
            detail::duplicateSeedToFill(*compound, project, track.id, seedId, false,
                                        plan.mainStart, plan.mainLength, seedLen);
            detail::duplicateSeedToFill(*compound, project, track.id, seedId, false,
                                        plan.variationStart, plan.variationLength, seedLen);
            detail::duplicateSeedToFill(*compound, project, track.id, seedId, false,
                                        plan.outroStart, plan.outroLength, seedLen);
        }
        else if (track.type == TrackType::Drums)
        {
            if (track.drumClips.empty())
                continue;
            Id seedId = track.drumClips.front().id;
            double seedLen = track.drumClips.front().lengthBeats;
            for (const auto& clip : track.drumClips)
            {
                if (std::abs(clip.startBeat) < 1.0e-6)
                {
                    seedId = clip.id;
                    seedLen = clip.lengthBeats > 0.0 ? clip.lengthBeats : seedLength;
                    break;
                }
            }
            detail::duplicateSeedToFill(*compound, project, track.id, seedId, true,
                                        plan.mainStart, plan.mainLength, seedLen);
            detail::duplicateSeedToFill(*compound, project, track.id, seedId, true,
                                        plan.variationStart, plan.variationLength, seedLen);
            detail::duplicateSeedToFill(*compound, project, track.id, seedId, true,
                                        plan.outroStart, plan.outroLength, seedLen);
        }
    }

    return compound;
}

// Removes drum + bass clips that start inside the Intro (contrast: thinner opening).
[[nodiscard]] inline std::unique_ptr<CompoundCommand> makeIntroMuteDrumsAndBassCommand(Project& project)
{
    auto compound = std::make_unique<CompoundCommand>("Intro: bring instruments in later");
    const Section* intro = nullptr;
    for (const auto& s : project.sections())
        if (s.label == SectionLabel::Intro)
            intro = &s;
    if (intro == nullptr)
        return compound;

    for (const auto role : { TrackRole::Drums, TrackRole::Bass })
    {
        auto* track = project.findTrackByRole(role);
        if (track == nullptr)
            continue;
        if (track->type == TrackType::Drums)
        {
            for (const auto& clip : track->drumClips)
            {
                if (clip.startBeat + 1.0e-9 >= intro->startBeat
                    && clip.startBeat < intro->endBeat() - 1.0e-9)
                    compound->add(std::make_unique<DeleteClipCommand>(track->id, clip.id));
            }
        }
        else
        {
            for (const auto& clip : track->clips)
            {
                if (clip.startBeat + 1.0e-9 >= intro->startBeat
                    && clip.startBeat < intro->endBeat() - 1.0e-9)
                    compound->add(std::make_unique<DeleteClipCommand>(track->id, clip.id));
            }
        }
    }
    return compound;
}

// Thins hats in every drum clip that starts inside Variation or Outro.
[[nodiscard]] inline std::unique_ptr<CompoundCommand> makeThinVariationDrumsCommand(Project& project)
{
    auto compound = std::make_unique<CompoundCommand>("Variation: thin the drums");
    auto* drums = project.findTrackByRole(TrackRole::Drums);
    if (drums == nullptr)
        return compound;

    auto inLabeled = [&](double start, SectionLabel label) {
        for (const auto& s : project.sections())
            if (s.label == label && start + 1.0e-9 >= s.startBeat && start < s.endBeat() - 1.0e-9)
                return true;
        return false;
    };

    for (const auto& clip : drums->drumClips)
    {
        if (inLabeled(clip.startBeat, SectionLabel::Verse)
            || inLabeled(clip.startBeat, SectionLabel::Outro))
        {
            compound->add(std::make_unique<ReplaceDrumPatternCommand>(
                drums->id, clip.id, thinDrumPattern(clip.pattern, DrumLane::ClosedHat)));
        }
    }
    return compound;
}

} // namespace sensei::core
