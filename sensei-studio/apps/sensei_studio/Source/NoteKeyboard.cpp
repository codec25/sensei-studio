#include "NoteKeyboard.hpp"

#include "ui/Theme.hpp"

namespace {
constexpr const char* kKeyNames[12] {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
}

NoteKeyboard::NoteKeyboard()
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void NoteKeyboard::paint(juce::Graphics& g)
{
    const auto& p = studioPalette();
    g.setColour(p.textMuted);
    g.setFont(12.5f);
    g.drawText("Audition",
               getLocalBounds().removeFromTop(18),
               juce::Justification::centredLeft,
               false);

    for (int i = 0; i < kNumKeys; ++i)
    {
        const auto bounds = keyBounds_[static_cast<size_t>(i)];
        if (bounds.isEmpty())
            continue;

        const bool black = juce::String(kKeyNames[i]).containsChar('#');
        const bool active = currentMidi_ == kBaseMidi + i;

        g.setColour(active ? p.accent : (black ? p.bg1 : p.bg2));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
        g.setColour(p.borderSoft.withAlpha(0.7f));
        g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);
        g.setColour(active ? p.clipText : p.textPrimary);
        g.setFont(11.5f);
        g.drawFittedText(kKeyNames[i], bounds, juce::Justification::centred, 1);
    }
}

void NoteKeyboard::resized()
{
    auto area = getLocalBounds().withTrimmedTop(20).reduced(0, 2);
    const int keyWidth = juce::jmax(1, area.getWidth() / kNumKeys);

    for (int i = 0; i < kNumKeys; ++i)
    {
        keyBounds_[static_cast<size_t>(i)] = {
            area.getX() + i * keyWidth + 1,
            area.getY(),
            keyWidth - 2,
            area.getHeight()
        };
    }
}

int NoteKeyboard::midiForPosition(juce::Point<int> pos) const noexcept
{
    for (int i = 0; i < kNumKeys; ++i)
    {
        if (keyBounds_[static_cast<size_t>(i)].contains(pos))
            return kBaseMidi + i;
    }
    return -1;
}

void NoteKeyboard::triggerNote(int midiNote)
{
    if (midiNote < 0 || midiNote == currentMidi_)
        return;

    releaseCurrentNote();
    currentMidi_ = midiNote;
    if (onNoteOn)
        onNoteOn(midiNote);
    repaint();
}

void NoteKeyboard::releaseCurrentNote()
{
    if (currentMidi_ < 0)
        return;

    if (onNoteOff)
        onNoteOff(currentMidi_);
    currentMidi_ = -1;
    repaint();
}

void NoteKeyboard::mouseDown(const juce::MouseEvent& event)
{
    triggerNote(midiForPosition(event.getPosition()));
}

void NoteKeyboard::mouseUp(const juce::MouseEvent&)
{
    releaseCurrentNote();
}

void NoteKeyboard::mouseDrag(const juce::MouseEvent& event)
{
    const int midi = midiForPosition(event.getPosition());
    if (midi >= 0)
        triggerNote(midi);
}
