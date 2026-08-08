#include "PianoRoll.hpp"

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

void PianoRoll::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff11151a));

    const float contentH = kNumRows * kRowH;
    const double loopBeats = document_ != nullptr ? document_->project().loop().lengthBeats : 16.0;
    const float contentW = kKeyW + static_cast<float>(loopBeats) * kBeatW;

    // Rows + keys
    for (int row = 0; row < kNumRows; ++row)
    {
        const int pitch = kTopMidi - row;
        const float y = static_cast<float>(row) * kRowH;
        g.setColour(isBlackKey(pitch) ? juce::Colour(0xff171b21) : juce::Colour(0xff1e222a));
        g.fillRect(kKeyW, y, contentW - kKeyW, kRowH);

        g.setColour(juce::Colour(0xff0d1014));
        g.fillRect(0.0f, y, kKeyW, kRowH);
        g.setColour(juce::Colour(0xff778190));
        g.setFont(11.0f);
        g.drawText(noteName(pitch), juce::Rectangle<float>(4.0f, y, kKeyW - 8.0f, kRowH),
                   juce::Justification::centredLeft, false);

        g.setColour(juce::Colour(0xff222832));
        g.drawHorizontalLine(static_cast<int>(y + kRowH), kKeyW, contentW);
    }

    // Vertical grid (1/16)
    const int steps = static_cast<int>(std::lround(loopBeats / sensei::core::kDefaultGridBeats));
    for (int s = 0; s <= steps; ++s)
    {
        const float x = kKeyW + static_cast<float>(s) * (kBeatW * static_cast<float>(sensei::core::kDefaultGridBeats));
        const bool beatLine = (s % 4) == 0;
        g.setColour(beatLine ? juce::Colour(0xff3a4452) : juce::Colour(0xff272d35));
        g.drawVerticalLine(static_cast<int>(x), 0.0f, contentH);
    }

    // Notes
    if (document_ != nullptr)
    {
        if (const auto* clip = document_->project().primaryClip())
        {
            for (const auto& note : clip->notes)
            {
                const float x = xForBeat(note.startBeat);
                const float y = yForPitch(note.pitch);
                const float w = juce::jmax(6.0f, static_cast<float>(note.lengthBeats) * kBeatW);
                const bool selected = note.id == document_->selectedNoteId();

                g.setColour(selected ? juce::Colour(0xffeaff9a) : juce::Colour(0xffd5ff5c));
                g.setOpacity(0.45f + note.velocity * 0.55f);
                g.fillRoundedRectangle(x, y + 1.0f, w, kRowH - 2.0f, 4.0f);
                g.setOpacity(1.0f);
                g.setColour(selected ? juce::Colour(0xff9cf0ff) : juce::Colour(0xff2a303a));
                g.drawRoundedRectangle(x, y + 1.0f, w, kRowH - 2.0f, 4.0f, selected ? 2.0f : 1.0f);
            }
        }
    }

    // Playhead
    const float px = xForBeat(playheadBeats_);
    g.setColour(juce::Colour(0xff9cf0ff));
    g.drawLine(px, 0.0f, px, contentH, 2.0f);

    g.setColour(juce::Colour(0xff2a303a));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);
}

void PianoRoll::resized() {}

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

sensei::core::MidiNote* PianoRoll::hitTestNote(juce::Point<float> pos, bool& nearRightEdge)
{
    nearRightEdge = false;
    if (document_ == nullptr)
        return nullptr;

    auto* clip = document_->project().primaryClip();
    if (clip == nullptr)
        return nullptr;

    // Topmost (last drawn) wins — iterate reverse.
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

    auto* track = document_->project().primaryMidiTrack();
    auto* clip = document_->project().primaryClip();
    if (track == nullptr || clip == nullptr)
        return;

    if (event.mods.isPopupMenu())
    {
        bool edge = false;
        if (auto* note = hitTestNote(event.position, edge))
        {
            document_->setSelectedNoteId(note->id);
            document_->execute(std::make_unique<sensei::core::DeleteNoteCommand>(track->id, clip->id, note->id));
            document_->setSelectedNoteId(sensei::core::kInvalidId);
            notifyEdited();
        }
        return;
    }

    bool edge = false;
    if (auto* note = hitTestNote(event.position, edge))
    {
        document_->setSelectedNoteId(note->id);
        dragNoteId_ = note->id;
        dragOriginBeat_ = note->startBeat;
        dragOriginPitch_ = note->pitch;
        dragOriginLength_ = note->lengthBeats;
        dragGrabOffsetBeats_ = beatForX(event.position.x) - note->startBeat;
        dragMode_ = edge ? DragMode::Resize : DragMode::Move;

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

    const double start = sensei::core::snapBeat(beatForX(event.position.x));
    const int pitch = pitchForY(event.position.y);
    auto cmd = std::make_unique<sensei::core::AddNoteCommand>(track->id, clip->id, pitch, start, 1.0, 0.8f);
    auto* raw = cmd.get();
    if (document_->execute(std::move(cmd)))
    {
        document_->setSelectedNoteId(raw->createdNoteId());
        dragMode_ = DragMode::Create;
        dragNoteId_ = raw->createdNoteId();
        dragOriginBeat_ = start;
        dragOriginPitch_ = pitch;
        dragOriginLength_ = 1.0;
        if (onAuditionNoteOn)
        {
            auditionPitch_ = pitch;
            onAuditionNoteOn(pitch, 0.8f);
        }
        notifyEdited();
    }
}

void PianoRoll::mouseDrag(const juce::MouseEvent& event)
{
    if (document_ == nullptr || dragMode_ == DragMode::None || dragNoteId_ == sensei::core::kInvalidId)
        return;

    auto* track = document_->project().primaryMidiTrack();
    auto* clip = document_->project().primaryClip();
    auto* note = document_->project().findNote(track->id, clip->id, dragNoteId_);
    if (note == nullptr)
        return;

    if (dragMode_ == DragMode::Move || dragMode_ == DragMode::Create)
    {
        const double newStart = sensei::core::snapBeat(beatForX(event.position.x) - dragGrabOffsetBeats_);
        const int newPitch = pitchForY(event.position.y);
        // Live preview mutate then publish — finalized as command on mouseUp for move.
        // For Create, note already added; preview move/pitch directly then commit Move on up if changed.
        note->startBeat = sensei::core::clampNonNegativeBeat(newStart);
        note->pitch = sensei::core::clampMidiNote(newPitch);
        document_->publishSnapshot();
        if (auditionPitch_ != note->pitch)
        {
            if (onAuditionNoteOff && auditionPitch_ >= 0)
                onAuditionNoteOff(auditionPitch_);
            auditionPitch_ = note->pitch;
            if (onAuditionNoteOn)
                onAuditionNoteOn(note->pitch, note->velocity);
        }
        repaint();
    }
    else if (dragMode_ == DragMode::Resize)
    {
        const double endBeat = sensei::core::snapBeat(beatForX(event.position.x));
        const double length = sensei::core::snapLength(endBeat - note->startBeat);
        note->lengthBeats = length;
        document_->publishSnapshot();
        repaint();
    }
}

void PianoRoll::mouseUp(const juce::MouseEvent&)
{
    if (onAuditionNoteOff && auditionPitch_ >= 0)
        onAuditionNoteOff(auditionPitch_);
    auditionPitch_ = -1;

    if (document_ == nullptr)
    {
        dragMode_ = DragMode::None;
        return;
    }

    auto* track = document_->project().primaryMidiTrack();
    auto* clip = document_->project().primaryClip();
    auto* note = (track && clip) ? document_->project().findNote(track->id, clip->id, dragNoteId_) : nullptr;

    if (note != nullptr && (dragMode_ == DragMode::Move || dragMode_ == DragMode::Create))
    {
        const double finalStart = note->startBeat;
        const int finalPitch = note->pitch;
        // Restore origin then apply command so undo works.
        note->startBeat = dragOriginBeat_;
        note->pitch = dragOriginPitch_;
        if (std::abs(finalStart - dragOriginBeat_) > 1.0e-9 || finalPitch != dragOriginPitch_)
        {
            document_->execute(std::make_unique<sensei::core::MoveNoteCommand>(
                track->id, clip->id, dragNoteId_, finalStart, finalPitch));
            notifyEdited();
        }
        else
        {
            document_->publishSnapshot();
        }
    }
    else if (note != nullptr && dragMode_ == DragMode::Resize)
    {
        const double finalLen = note->lengthBeats;
        note->lengthBeats = dragOriginLength_;
        if (std::abs(finalLen - dragOriginLength_) > 1.0e-9)
        {
            document_->execute(std::make_unique<sensei::core::ResizeNoteCommand>(
                track->id, clip->id, dragNoteId_, finalLen));
            notifyEdited();
        }
        else
        {
            document_->publishSnapshot();
        }
    }

    dragMode_ = DragMode::None;
    dragNoteId_ = sensei::core::kInvalidId;
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

    const auto id = document_->selectedNoteId();
    if (id == sensei::core::kInvalidId)
        return;

    auto* track = document_->project().primaryMidiTrack();
    auto* clip = document_->project().primaryClip();
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
