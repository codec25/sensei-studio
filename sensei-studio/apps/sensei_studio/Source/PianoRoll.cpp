#include "PianoRoll.hpp"

#include "ui/Theme.hpp"

#include "sensei/core/Grid.hpp"

#include <cmath>

namespace {
bool isBlackKey(int midi)
{
    switch (midi % 12)
    {
        case 1: case 3: case 6: case 8: case 10: return true;
        default: return false;
    }
}

juce::String noteName(int midi)
{
    static constexpr const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int pc = ((midi % 12) + 12) % 12;
    const int octave = (midi / 12) - 1;
    return juce::String(names[pc]) + juce::String(octave);
}
} // namespace

PianoRoll::PianoRoll()
{
    setWantsKeyboardFocus(true);
}

void PianoRoll::setDocument(sensei::core::Document* document)
{
    document_ = document;
    clearPreview();
    repaint();
}

void PianoRoll::setPlayheadBeats(double beats)
{
    if (std::abs(playheadBeats_ - beats) > 1.0e-4)
    {
        playheadBeats_ = beats;
        repaint();
    }
}

void PianoRoll::clearPreview() noexcept
{
    preview_ = {};
}

void PianoRoll::drawNoteRect(juce::Graphics& g,
                             double startBeat,
                             int pitch,
                             double lengthBeats,
                             float velocity,
                             bool selected) const
{
    const float x = xForBeat(startBeat);
    const float y = yForPitch(pitch);
    const float w = juce::jmax(6.0f, static_cast<float>(lengthBeats) * kBeatW);

    const auto& p = studioPalette();
    g.setColour(selected ? p.accent.brighter(0.25f) : p.accent);
    g.setOpacity(0.45f + velocity * 0.55f);
    g.fillRoundedRectangle(x, y + 1.0f, w, kRowH - 2.0f, 4.0f);
    g.setOpacity(1.0f);
    g.setColour(selected ? p.selectedOutline : p.borderSoft);
    g.drawRoundedRectangle(x, y + 1.0f, w, kRowH - 2.0f, 4.0f, selected ? 2.0f : 1.0f);
}

void PianoRoll::paint(juce::Graphics& g)
{
    const auto& p = studioPalette();
    g.fillAll(p.bg0);

    const float contentH = kNumRows * kRowH;
    const double loopBeats = document_ != nullptr ? document_->project().loop().lengthBeats : 16.0;
    const float contentW = kKeyW + static_cast<float>(loopBeats) * kBeatW;

    for (int row = 0; row < kNumRows; ++row)
    {
        const int pitch = kTopMidi - row;
        const float y = static_cast<float>(row) * kRowH;
        g.setColour(isBlackKey(pitch) ? p.bg1 : p.bg2);
        g.fillRect(kKeyW, y, contentW - kKeyW, kRowH);

        g.setColour(p.bg0);
        g.fillRect(0.0f, y, kKeyW, kRowH);
        g.setColour(p.textMuted);
        g.setFont(12.0f);
        g.drawText(noteName(pitch), juce::Rectangle<float>(4.0f, y, kKeyW - 8.0f, kRowH),
                   juce::Justification::centredLeft, false);

        if ((row % 4) == 0)
        {
            g.setColour(p.gridMinor.withAlpha(0.45f));
            g.drawHorizontalLine(static_cast<int>(y + kRowH), kKeyW, contentW);
        }
    }

    const int steps = static_cast<int>(std::lround(loopBeats / sensei::core::kDefaultGridBeats));
    for (int s = 0; s <= steps; ++s)
    {
        const float x = kKeyW + static_cast<float>(s) * (kBeatW * static_cast<float>(sensei::core::kDefaultGridBeats));
        const bool beatLine = (s % 4) == 0;
        if (! beatLine && (s % 2) != 0)
            continue;
        g.setColour(beatLine ? p.gridMajor.withAlpha(0.55f) : p.gridMinor.withAlpha(0.35f));
        g.drawVerticalLine(static_cast<int>(x), 0.0f, contentH);
    }

    if (document_ != nullptr)
    {
        if (const auto* clip = activeClip())
        {
            for (const auto& note : clip->notes)
            {
                // While moving/resizing, hide the canonical note and show preview instead.
                if (preview_.active && preview_.noteId == note.id
                    && (preview_.mode == DragMode::Move || preview_.mode == DragMode::Resize))
                {
                    continue;
                }

                const bool selected = note.id == document_->selectedNoteId();
                drawNoteRect(g, note.startBeat, note.pitch, note.lengthBeats, note.velocity, selected);
            }
        }
    }

    if (preview_.active)
    {
        drawNoteRect(g,
                     preview_.startBeat,
                     preview_.pitch,
                     preview_.lengthBeats,
                     preview_.velocity,
                     true);
    }

    const float px = xForBeat(playheadBeats_);
    g.setColour(p.playhead);
    g.drawLine(px, 0.0f, px, contentH, 2.0f);
}

void PianoRoll::resized() {}

sensei::core::Track* PianoRoll::activeTrack() noexcept
{
    if (document_ == nullptr)
        return nullptr;
    if (auto* t = document_->project().findTrack(document_->selectedTrackId()))
        if (t->type == sensei::core::TrackType::Midi)
            return t;
    return document_->project().primaryMidiTrack();
}

sensei::core::MidiClip* PianoRoll::activeClip() noexcept
{
    auto* track = activeTrack();
    if (track == nullptr || track->clips.empty() || document_ == nullptr)
        return nullptr;
    if (auto* clip = document_->project().findClip(track->id, document_->selectedClipId()))
        return clip;
    return &track->clips.front();
}

const sensei::core::MidiClip* PianoRoll::activeClip() const noexcept
{
    if (document_ == nullptr)
        return nullptr;
    const auto* track = document_->project().findTrack(document_->selectedTrackId());
    if (track == nullptr || track->type != sensei::core::TrackType::Midi || track->clips.empty())
        return document_->project().primaryClip();
    if (const auto* clip = document_->project().findClip(track->id, document_->selectedClipId()))
        return clip;
    return &track->clips.front();
}

int PianoRoll::pitchForY(float y) const noexcept
{
    const int row = juce::jlimit(0, kNumRows - 1, static_cast<int>(y / kRowH));
    return kTopMidi - row;
}

double PianoRoll::beatForX(float x) const noexcept
{
    const float rel = juce::jmax(0.0f, x - kKeyW);
    return static_cast<double>(rel / kBeatW);
}

float PianoRoll::yForPitch(int pitch) const noexcept
{
    const int row = juce::jlimit(0, kNumRows - 1, kTopMidi - pitch);
    return static_cast<float>(row) * kRowH;
}

float PianoRoll::xForBeat(double beat) const noexcept
{
    return kKeyW + static_cast<float>(beat) * kBeatW;
}

const sensei::core::MidiNote* PianoRoll::hitTestNote(juce::Point<float> pos, bool& nearRightEdge) const
{
    nearRightEdge = false;
    if (document_ == nullptr)
        return nullptr;

    const auto* clip = activeClip();
    if (clip == nullptr)
        return nullptr;

    for (auto it = clip->notes.rbegin(); it != clip->notes.rend(); ++it)
    {
        const float x = xForBeat(it->startBeat);
        const float y = yForPitch(it->pitch);
        const float w = juce::jmax(6.0f, static_cast<float>(it->lengthBeats) * kBeatW);
        const juce::Rectangle<float> bounds(x, y + 1.0f, w, kRowH - 2.0f);
        if (bounds.contains(pos))
        {
            nearRightEdge = (pos.x >= bounds.getRight() - kResizeHandle);
            return &(*it);
        }
    }
    return nullptr;
}

void PianoRoll::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    if (document_ == nullptr)
        return;

    auto* track = activeTrack();
    auto* clip = activeClip();
    if (track == nullptr || clip == nullptr)
        return;

    if (event.mods.isPopupMenu())
    {
        bool edge = false;
        if (const auto* note = hitTestNote(event.position, edge))
        {
            document_->setSelectedNoteId(note->id);
            document_->execute(std::make_unique<sensei::core::DeleteNoteCommand>(track->id, clip->id, note->id));
            document_->setSelectedNoteId(sensei::core::kInvalidId);
            clearPreview();
            notifyEdited();
        }
        return;
    }

    bool edge = false;
    if (const auto* note = hitTestNote(event.position, edge))
    {
        document_->setSelectedNoteId(note->id);
        preview_.active = true;
        preview_.mode = edge ? DragMode::Resize : DragMode::Move;
        preview_.noteId = note->id;
        preview_.startBeat = note->startBeat;
        preview_.lengthBeats = note->lengthBeats;
        preview_.pitch = note->pitch;
        preview_.velocity = note->velocity;
        preview_.originStartBeat = note->startBeat;
        preview_.originLengthBeats = note->lengthBeats;
        preview_.originPitch = note->pitch;
        preview_.grabOffsetBeats = beatForX(event.position.x) - note->startBeat;

        if (onAuditionNoteOn)
        {
            auditionPitch_ = note->pitch;
            onAuditionNoteOn(note->pitch, note->velocity);
        }
        repaint();
        return;
    }

    if (event.position.x < kKeyW)
    {
        const int pitch = pitchForY(event.position.y);
        if (onAuditionNoteOn)
        {
            auditionPitch_ = pitch;
            onAuditionNoteOn(pitch, 0.85f);
        }
        return;
    }

    // Create: preview only until mouseUp — no canonical mutation yet.
    const double start = sensei::core::snapBeat(beatForX(event.position.x));
    const int pitch = pitchForY(event.position.y);
    preview_.active = true;
    preview_.mode = DragMode::Create;
    preview_.noteId = sensei::core::kInvalidId;
    preview_.startBeat = start;
    preview_.lengthBeats = 1.0;
    preview_.pitch = pitch;
    preview_.velocity = 0.8f;
    preview_.originStartBeat = start;
    preview_.originLengthBeats = 1.0;
    preview_.originPitch = pitch;
    preview_.grabOffsetBeats = 0.0;

    if (onAuditionNoteOn)
    {
        auditionPitch_ = pitch;
        onAuditionNoteOn(pitch, 0.8f);
    }
    repaint();
}

void PianoRoll::mouseDrag(const juce::MouseEvent& event)
{
    if (! preview_.active)
        return;

    if (preview_.mode == DragMode::Move || preview_.mode == DragMode::Create)
    {
        const double newStart = sensei::core::snapBeat(beatForX(event.position.x) - preview_.grabOffsetBeats);
        const int newPitch = pitchForY(event.position.y);
        preview_.startBeat = sensei::core::clampNonNegativeBeat(newStart);
        preview_.pitch = sensei::core::clampMidiNote(newPitch);

        if (auditionPitch_ != preview_.pitch)
        {
            if (onAuditionNoteOff && auditionPitch_ >= 0)
                onAuditionNoteOff(auditionPitch_);
            auditionPitch_ = preview_.pitch;
            if (onAuditionNoteOn)
                onAuditionNoteOn(preview_.pitch, preview_.velocity);
        }
        repaint();
    }
    else if (preview_.mode == DragMode::Resize)
    {
        const double endBeat = sensei::core::snapBeat(beatForX(event.position.x));
        preview_.lengthBeats = sensei::core::snapLength(endBeat - preview_.startBeat);
        repaint();
    }
}

void PianoRoll::mouseUp(const juce::MouseEvent&)
{
    if (onAuditionNoteOff && auditionPitch_ >= 0)
        onAuditionNoteOff(auditionPitch_);
    auditionPitch_ = -1;

    commitPreview();
}

void PianoRoll::commitPreview()
{
    if (document_ == nullptr || ! preview_.active)
    {
        clearPreview();
        repaint();
        return;
    }

    auto* track = activeTrack();
    auto* clip = activeClip();
    if (track == nullptr || clip == nullptr)
    {
        clearPreview();
        repaint();
        return;
    }

    if (preview_.mode == DragMode::Create)
    {
        auto cmd = std::make_unique<sensei::core::AddNoteCommand>(
            track->id, clip->id, preview_.pitch, preview_.startBeat, preview_.lengthBeats, preview_.velocity);
        auto* raw = cmd.get();
        if (document_->execute(std::move(cmd)))
        {
            document_->setSelectedNoteId(raw->createdNoteId());
            notifyEdited();
        }
    }
    else if (preview_.mode == DragMode::Move)
    {
        if (std::abs(preview_.startBeat - preview_.originStartBeat) > 1.0e-9
            || preview_.pitch != preview_.originPitch)
        {
            document_->execute(std::make_unique<sensei::core::MoveNoteCommand>(
                track->id, clip->id, preview_.noteId, preview_.startBeat, preview_.pitch));
            notifyEdited();
        }
    }
    else if (preview_.mode == DragMode::Resize)
    {
        if (std::abs(preview_.lengthBeats - preview_.originLengthBeats) > 1.0e-9)
        {
            document_->execute(std::make_unique<sensei::core::ResizeNoteCommand>(
                track->id, clip->id, preview_.noteId, preview_.lengthBeats));
            notifyEdited();
        }
    }

    clearPreview();
    repaint();
}

bool PianoRoll::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelected();
        return true;
    }

    const auto mods = key.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown())
    {
        if (key.getKeyCode() == 'z' || key.getKeyCode() == 'Z')
        {
            if (document_ == nullptr)
                return true;

            clearPreview();
            if (mods.isShiftDown())
                document_->redo();
            else
                document_->undo();
            notifyEdited();
            return true;
        }
        if (key.getKeyCode() == 'y' || key.getKeyCode() == 'Y')
        {
            if (document_ != nullptr)
            {
                clearPreview();
                document_->redo();
                notifyEdited();
            }
            return true;
        }
    }
    return false;
}

void PianoRoll::deleteSelected()
{
    if (document_ == nullptr)
        return;

    clearPreview();
    const auto id = document_->selectedNoteId();
    if (id == sensei::core::kInvalidId)
        return;

    auto* track = activeTrack();
    auto* clip = activeClip();
    if (track == nullptr || clip == nullptr)
        return;

    if (document_->execute(std::make_unique<sensei::core::DeleteNoteCommand>(track->id, clip->id, id)))
    {
        document_->setSelectedNoteId(sensei::core::kInvalidId);
        notifyEdited();
    }
}

void PianoRoll::notifyEdited()
{
    repaint();
    if (onProjectEdited)
        onProjectEdited();
}
