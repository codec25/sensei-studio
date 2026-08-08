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

    // Transient editor preview — never mutates canonical Project notes.
    struct Preview
    {
        bool active = false;
        DragMode mode = DragMode::None;
        sensei::core::Id noteId = sensei::core::kInvalidId;
        double startBeat = 0.0;
        double lengthBeats = 1.0;
        int pitch = 60;
        float velocity = 0.8f;
        double originStartBeat = 0.0;
        double originLengthBeats = 1.0;
        int originPitch = 60;
        double grabOffsetBeats = 0.0;
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
    [[nodiscard]] const sensei::core::MidiNote* hitTestNote(juce::Point<float> pos, bool& nearRightEdge) const;
    void drawNoteRect(juce::Graphics& g, double startBeat, int pitch, double lengthBeats,
                      float velocity, bool selected) const;
    void deleteSelected();
    void notifyEdited();
    void clearPreview() noexcept;
    void commitPreview();

    sensei::core::Document* document_ = nullptr;
    double playheadBeats_ = 0.0;
    Preview preview_ {};
    int auditionPitch_ = -1;
};
