#pragma once

#include "sensei/core/Transport.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class TransportBar final : public juce::Component
{
public:
    TransportBar();

    void setTransport(sensei::core::Transport* transport);
    void setPositionBeats(double beats, double songLengthBeats, bool loopEnabled);
    void setAudioDeviceAvailable(bool available);
    void refreshFromTransport();
    void applyThemeColours();

    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void(double)> onBpmChanged;
    std::function<void()> onToggleLoop;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    sensei::core::Transport* transport_ = nullptr;
    juce::TextButton playButton_ { "Play" };
    juce::TextButton stopButton_ { "Stop" };
    juce::TextButton loopButton_ { "Loop" };
    juce::Label bpmLabel_ { {}, "BPM" };
    juce::Label bpmEditor_;
    juce::Label positionLabel_;
    juce::Label modeLabel_;
    juce::Label audioStatusLabel_;
    double positionBeats_ = 0.0;
    double songLengthBeats_ = 16.0;
    bool loopEnabled_ = true;
    bool audioDeviceAvailable_ = true;
};
