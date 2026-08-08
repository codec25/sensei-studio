#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

class NoteKeyboard final : public juce::Component
{
public:
    NoteKeyboard();

    std::function<void(int midiNote)> onNoteOn;
    std::function<void(int midiNote)> onNoteOff;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    static constexpr int kNumKeys = 12;
    static constexpr int kBaseMidi = 60; // C4

    int midiForPosition(juce::Point<int> pos) const noexcept;
    void triggerNote(int midiNote);
    void releaseCurrentNote();

    int currentMidi_ = -1;
    std::array<juce::Rectangle<int>, kNumKeys> keyBounds_ {};
};
