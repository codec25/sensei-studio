#include "sensei/core/Project.hpp"

namespace sensei::core {

Project Project::createStarter(std::string name)
{
    Project project;
    project.id_ = project.generateId();
    project.name_ = std::move(name);
    project.loop_ = LoopRegion { 0.0, kDefaultLoopBeats, true };

    Track track;
    track.id = project.generateId();
    track.name = "Sensei Synth";
    track.type = TrackType::Midi;

    MidiClip clip;
    clip.id = project.generateId();
    clip.name = "Idea";
    clip.startBeat = 0.0;
    clip.lengthBeats = kDefaultLoopBeats;
    track.clips.push_back(std::move(clip));

    project.tracks_.push_back(std::move(track));
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
    return tracks_.empty() ? nullptr : &tracks_.front();
}

const Track* Project::primaryMidiTrack() const noexcept
{
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

} // namespace sensei::core
