#pragma once

#include "sensei/core/Document.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class PianoRoll final : public juce::Component
{
public:
    PianoRoll();

    void setDocument(sensei::core::Document* document);
    void setPlayheadBeats(double beats);

    std::function<void(int midiNote, float velocity)> onAuditionNoteOn;
    std::function<void(int midiNote)> onAuditionNoteOff;
    std::function<void()> onProjectEdited;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    enum class DragMode
    {
        None,
        Create,
        Move,
        Resize
    };

    static constexpr int kTopMidi = 72; // C5
    static constexpr int kBottomMidi = 48; // C3
    static constexpr int kNumRows = kTopMidi - kBottomMidi + 1;
    static constexpr float kRowH = 18.0f;
    static constexpr float kBeatW = 48.0f;
    static constexpr float kKeyW = 52.0f;
    static constexpr float kResizeHandle = 8.0f;

    [[nodiscard]] int pitchForY(float y) const noexcept;
    [[nodiscard]] double beatForX(float x) const noexcept;
    [[nodiscard]] float yForPitch(int pitch) const noexcept;
    [[nodiscard]] float xForBeat(double beat) const noexcept;
    [[nodiscard]] sensei::core::MidiNote* hitTestNote(juce::Point<float> pos, bool& nearRightEdge);
    void deleteSelected();
    void notifyEdited();

    sensei::core::Document* document_ = nullptr;
    double playheadBeats_ = 0.0;

    DragMode dragMode_ = DragMode::None;
    sensei::core::Id dragNoteId_ = sensei::core::kInvalidId;
    double dragOriginBeat_ = 0.0;
    int dragOriginPitch_ = 0;
    double dragOriginLength_ = 0.0;
    double dragGrabOffsetBeats_ = 0.0;
    int auditionPitch_ = -1;
};
