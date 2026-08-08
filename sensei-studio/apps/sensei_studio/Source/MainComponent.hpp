#pragma once

#include "PianoRoll.hpp"
#include "SenseiPanel.hpp"
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
    void refreshSensei(bool force);
    void handleProjectEdited();

    sensei::core::Document document_;
    sensei::engine::AudioEngine audioEngine_;

    juce::Label brandLabel_;
    juce::Label subtitleLabel_;
    juce::Label trackLabel_;
    juce::Label helpLabel_;
    TransportBar transportBar_;
    PianoRoll pianoRoll_;
    SenseiPanel senseiPanel_;
    juce::Label positionLabel_;
};
