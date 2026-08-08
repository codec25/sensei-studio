#pragma once

#include "sensei/core/Transport.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class TransportBar final : public juce::Component
{
public:
    TransportBar();

    void setTransport(sensei::core::Transport* transport);
    void refreshFromTransport();

    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void(double)> onBpmChanged;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    sensei::core::Transport* transport_ = nullptr;
    juce::TextButton playButton_ { "Play" };
    juce::TextButton stopButton_ { "Stop" };
    juce::Label bpmLabel_ { {}, "BPM" };
    juce::Label bpmEditor_;
    juce::Label statusLabel_;
};
