#include "NoteKeyboard.hpp"

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
    g.setColour(juce::Colour(0xff9ca6b5));
    g.setFont(14.0f);
    g.drawText("Click a key to audition (works while stopped)",
               getLocalBounds().removeFromTop(22),
               juce::Justification::centredLeft,
               false);

    for (int i = 0; i < kNumKeys; ++i)
    {
        const auto bounds = keyBounds_[static_cast<size_t>(i)];
        if (bounds.isEmpty())
            continue;

        const bool black = juce::String(kKeyNames[i]).containsChar('#');
        const bool active = currentMidi_ == kBaseMidi + i;

        g.setColour(active ? juce::Colour(0xffd5ff5c)
                           : (black ? juce::Colour(0xff171b21) : juce::Colour(0xff1e222a)));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(juce::Colour(0xff2a303a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
        g.setColour(active ? juce::Colours::black : juce::Colour(0xfff4f5f7));
        g.drawFittedText(kKeyNames[i], bounds, juce::Justification::centred, 1);
    }
}

void NoteKeyboard::resized()
{
    auto area = getLocalBounds().withTrimmedTop(28).reduced(0, 4);
    const int keyWidth = juce::jmax(1, area.getWidth() / kNumKeys);

    for (int i = 0; i < kNumKeys; ++i)
    {
        keyBounds_[static_cast<size_t>(i)] = {
            area.getX() + i * keyWidth + 2,
            area.getY(),
            keyWidth - 4,
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
