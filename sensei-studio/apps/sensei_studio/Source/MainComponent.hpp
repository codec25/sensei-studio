#pragma once

#include "NoteKeyboard.hpp"
#include "SenseiPanel.hpp"
#include "TransportBar.hpp"

#include "sensei/core/Transport.hpp"
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

private:
    void timerCallback() override;

    sensei::core::Transport transport_;
    sensei::engine::AudioEngine audioEngine_;

    juce::Label brandLabel_;
    juce::Label subtitleLabel_;
    TransportBar transportBar_;
    NoteKeyboard keyboard_;
    SenseiPanel senseiPanel_;
    juce::Label positionLabel_;
};
