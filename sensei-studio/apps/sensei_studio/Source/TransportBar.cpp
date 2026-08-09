#include "TransportBar.hpp"

#include "sensei/core/Types.hpp"

#include <cmath>

TransportBar::TransportBar()
{
    addAndMakeVisible(playButton_);
    addAndMakeVisible(stopButton_);
    addAndMakeVisible(loopButton_);
    addAndMakeVisible(bpmLabel_);
    addAndMakeVisible(bpmEditor_);
    addAndMakeVisible(positionLabel_);
    addAndMakeVisible(modeLabel_);
    addAndMakeVisible(audioStatusLabel_);

    loopButton_.setClickingTogglesState(true);
    bpmLabel_.setJustificationType(juce::Justification::centredRight);
    bpmEditor_.setEditable(true);
    bpmEditor_.setJustificationType(juce::Justification::centred);
    positionLabel_.setJustificationType(juce::Justification::centredLeft);
    modeLabel_.setJustificationType(juce::Justification::centredRight);
    audioStatusLabel_.setJustificationType(juce::Justification::centredRight);
    audioStatusLabel_.setFont(juce::FontOptions(12.5f));
    positionLabel_.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    setAudioDeviceAvailable(true);
    applyThemeColours();

    playButton_.onClick = [this] {
        if (onPlay)
            onPlay();
    };
    stopButton_.onClick = [this] {
        if (onStop)
            onStop();
    };
    loopButton_.onClick = [this] {
        if (onToggleLoop)
            onToggleLoop();
    };
    bpmEditor_.onTextChange = [this] {
        if (onBpmChanged)
            onBpmChanged(bpmEditor_.getText().getDoubleValue());
    };
}

void TransportBar::setTransport(sensei::core::Transport* transport)
{
    transport_ = transport;
    refreshFromTransport();
}

void TransportBar::setPositionBeats(double beats, double songLengthBeats, bool loopEnabled)
{
    positionBeats_ = beats;
    songLengthBeats_ = songLengthBeats;
    loopEnabled_ = loopEnabled;

    const int bar = 1 + (int) std::floor(beats / sensei::core::kBeatsPerBar);
    const double beatInBar = std::fmod(beats, sensei::core::kBeatsPerBar) + 1.0;
    positionLabel_.setText(juce::String::formatted("%d : %.1f", bar, beatInBar),
                           juce::dontSendNotification);
    modeLabel_.setText(loopEnabled_ ? "Loop region" : "Whole song", juce::dontSendNotification);
    loopButton_.setToggleState(loopEnabled_, juce::dontSendNotification);
}

void TransportBar::setAudioDeviceAvailable(bool available)
{
    audioDeviceAvailable_ = available;
    if (audioDeviceAvailable_)
    {
        audioStatusLabel_.setText({}, juce::dontSendNotification);
        audioStatusLabel_.setVisible(false);
    }
    else
    {
        audioStatusLabel_.setText("Audio device unavailable", juce::dontSendNotification);
        audioStatusLabel_.setVisible(true);
    }
    applyThemeColours();
    resized();
}

void TransportBar::applyThemeColours()
{
    const auto& p = studioPalette();
    bpmLabel_.setColour(juce::Label::textColourId, p.textMuted);
    positionLabel_.setColour(juce::Label::textColourId, p.textPrimary);
    modeLabel_.setColour(juce::Label::textColourId, p.textMuted);
    audioStatusLabel_.setColour(juce::Label::textColourId, p.danger);
    bpmEditor_.setColour(juce::Label::backgroundColourId, p.bg2);
    bpmEditor_.setColour(juce::Label::outlineColourId, p.borderSoft);
    bpmEditor_.setColour(juce::Label::textColourId, p.textPrimary);
}

void TransportBar::refreshFromTransport()
{
    if (transport_ == nullptr)
        return;

    bpmEditor_.setText(juce::String(transport_->bpm(), 1), juce::dontSendNotification);
    playButton_.setEnabled(! transport_->isPlaying());
    playButton_.setButtonText(transport_->isPlaying() ? "Playing" : "Play");
    setPositionBeats(transport_->positionBeats(),
                     transport_->songLengthBeats() > 0.0 ? transport_->songLengthBeats()
                                                        : songLengthBeats_,
                     transport_->loopEnabled());
}

void TransportBar::paint(juce::Graphics& g)
{
    const auto& p = studioPalette();
    g.fillAll(p.transportBg);
    g.setColour(p.accentSoft);
    g.fillRect(0, getHeight() - 2, getWidth(), 2);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced(14, 8);
    playButton_.setBounds(area.removeFromLeft(88));
    area.removeFromLeft(8);
    stopButton_.setBounds(area.removeFromLeft(72));
    area.removeFromLeft(12);
    loopButton_.setBounds(area.removeFromLeft(72));
    area.removeFromLeft(18);
    bpmLabel_.setBounds(area.removeFromLeft(40));
    area.removeFromLeft(6);
    bpmEditor_.setBounds(area.removeFromLeft(64));
    area.removeFromLeft(18);
    positionLabel_.setBounds(area.removeFromLeft(100));
    if (audioStatusLabel_.isVisible())
        audioStatusLabel_.setBounds(area.removeFromRight(190));
    modeLabel_.setBounds(area.removeFromRight(120));
}
