#include "sensei/core/Project.hpp"

namespace sensei::core {
namespace {

Track makeMidiTrack(Project& project, const char* name, TrackRole role)
{
    Track track;
    track.id = project.generateId();
    track.name = name;
    track.type = TrackType::Midi;
    track.role = role;

    MidiClip clip;
    clip.id = project.generateId();
    clip.name = "Loop";
    clip.startBeat = 0.0;
    clip.lengthBeats = kDefaultLoopBeats;
    track.clips.push_back(std::move(clip));
    return track;
}

} // namespace

Project Project::createStarter(std::string name)
{
    Project project;
    project.id_ = project.generateId();
    project.name_ = std::move(name);
    project.loop_ = LoopRegion { 0.0, kDefaultLoopBeats, true };
    project.songLengthBeats_ = kDefaultLoopBeats;
    project.harmony_ = {};
    project.harmony_.rootPitchClass = 0;
    project.harmony_.mode = ScaleMode::Major;

    project.tracks_.push_back(makeMidiTrack(project, "Chords", TrackRole::Chords));
    project.tracks_.push_back(makeMidiTrack(project, "Bass", TrackRole::Bass));

    Track drums;
    drums.id = project.generateId();
    drums.name = "Drums";
    drums.type = TrackType::Drums;
    drums.role = TrackRole::Drums;
    DrumClip drumClip;
    drumClip.id = project.generateId();
    drumClip.name = "Loop";
    drumClip.startBeat = 0.0;
    drumClip.lengthBeats = kDefaultLoopBeats;
    drumClip.pattern.id = project.generateId();
    drumClip.pattern.stepCount = kDefaultDrumSteps;
    drums.drumClips.push_back(std::move(drumClip));
    project.tracks_.push_back(std::move(drums));

    project.tracks_.push_back(makeMidiTrack(project, "Melody", TrackRole::Melody));
    return project;
}

Track* Project::findTrack(Id trackId) noexcept
{
    for (auto& track : tracks_)
        if (track.id == trackId)
            return &track;
    return nullptr;
}

const Track* Project::findTrack(Id trackId) const noexcept
{
    for (const auto& track : tracks_)
        if (track.id == trackId)
            return &track;
    return nullptr;
}

Track* Project::findTrackByRole(TrackRole role) noexcept
{
    for (auto& track : tracks_)
        if (track.role == role)
            return &track;
    return nullptr;
}

const Track* Project::findTrackByRole(TrackRole role) const noexcept
{
    for (const auto& track : tracks_)
        if (track.role == role)
            return &track;
    return nullptr;
}

MidiClip* Project::findClip(Id trackId, Id clipId) noexcept
{
    if (auto* track = findTrack(trackId))
    {
        for (auto& clip : track->clips)
            if (clip.id == clipId)
                return &clip;
    }
    return nullptr;
}

const MidiClip* Project::findClip(Id trackId, Id clipId) const noexcept
{
    if (const auto* track = findTrack(trackId))
    {
        for (const auto& clip : track->clips)
            if (clip.id == clipId)
                return &clip;
    }
    return nullptr;
}

DrumClip* Project::findDrumClip(Id trackId, Id clipId) noexcept
{
    if (auto* track = findTrack(trackId))
    {
        for (auto& clip : track->drumClips)
            if (clip.id == clipId)
                return &clip;
    }
    return nullptr;
}

const DrumClip* Project::findDrumClip(Id trackId, Id clipId) const noexcept
{
    if (const auto* track = findTrack(trackId))
    {
        for (const auto& clip : track->drumClips)
            if (clip.id == clipId)
                return &clip;
    }
    return nullptr;
}

Section* Project::findSection(Id sectionId) noexcept
{
    for (auto& section : sections_)
        if (section.id == sectionId)
            return &section;
    return nullptr;
}

const Section* Project::findSection(Id sectionId) const noexcept
{
    for (const auto& section : sections_)
        if (section.id == sectionId)
            return &section;
    return nullptr;
}

MidiNote* Project::findNote(Id trackId, Id clipId, Id noteId) noexcept
{
    if (auto* clip = findClip(trackId, clipId))
    {
        for (auto& note : clip->notes)
            if (note.id == noteId)
                return &note;
    }
    return nullptr;
}

const MidiNote* Project::findNote(Id trackId, Id clipId, Id noteId) const noexcept
{
    if (const auto* clip = findClip(trackId, clipId))
    {
        for (const auto& note : clip->notes)
            if (note.id == noteId)
                return &note;
    }
    return nullptr;
}

Track* Project::primaryMidiTrack() noexcept
{
    if (auto* chords = findTrackByRole(TrackRole::Chords))
        return chords;
    return tracks_.empty() ? nullptr : &tracks_.front();
}

const Track* Project::primaryMidiTrack() const noexcept
{
    if (const auto* chords = findTrackByRole(TrackRole::Chords))
        return chords;
    return tracks_.empty() ? nullptr : &tracks_.front();
}

MidiClip* Project::primaryClip() noexcept
{
    auto* track = primaryMidiTrack();
    if (track == nullptr || track->clips.empty())
        return nullptr;
    return &track->clips.front();
}

const MidiClip* Project::primaryClip() const noexcept
{
    const auto* track = primaryMidiTrack();
    if (track == nullptr || track->clips.empty())
        return nullptr;
    return &track->clips.front();
}

std::size_t Project::totalNoteCount() const noexcept
{
    std::size_t count = 0;
    for (const auto& track : tracks_)
        for (const auto& clip : track.clips)
            count += clip.notes.size();
    return count;
}

std::size_t Project::totalDrumHitCount() const noexcept
{
    std::size_t count = 0;
    for (const auto& track : tracks_)
        for (const auto& clip : track.drumClips)
            count += clip.pattern.hits.size();
    return count;
}

} // namespace sensei::core
