#pragma once

#include "ArrangementView.hpp"
#include "ChordHelperPanel.hpp"
#include "DrumGrid.hpp"
#include "InstrumentPicker.hpp"
#include "PianoRoll.hpp"
#include "SenseiPanel.hpp"
#include "TrackList.hpp"
#include "TransportBar.hpp"

#include "sensei/core/Document.hpp"
#include "sensei/engine/AudioEngine.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void refreshAll();
    void handleProjectEdited();
    sensei::core::InstrumentId auditionInstrument() const;

    sensei::core::Document document_;
    sensei::engine::AudioEngine audioEngine_;

    juce::Label brandLabel_;
    juce::Label subtitleLabel_;
    juce::Label helpLabel_;
    juce::Label positionLabel_;
    juce::TextButton arrangeBtn_ { "Arrange" };
    juce::TextButton editBtn_ { "Edit clip" };
    bool showArrange_ = true;

    TransportBar transportBar_;
    TrackList trackList_;
    InstrumentPicker instrumentPicker_;
    ChordHelperPanel chordHelper_;
    ArrangementView arrangementView_;
    DrumGrid drumGrid_;
    PianoRoll pianoRoll_;
    SenseiPanel senseiPanel_;
};
