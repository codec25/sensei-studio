#include "TransportBar.hpp"

TransportBar::TransportBar()
{
    addAndMakeVisible(playButton_);
    addAndMakeVisible(stopButton_);
    addAndMakeVisible(bpmLabel_);
    addAndMakeVisible(bpmEditor_);
    addAndMakeVisible(statusLabel_);

    bpmLabel_.setJustificationType(juce::Justification::centredRight);
    bpmEditor_.setEditable(true);
    bpmEditor_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff111419));
    bpmEditor_.setColour(juce::Label::outlineColourId, juce::Colour(0xff2a303a));
    bpmEditor_.setJustificationType(juce::Justification::centred);
    statusLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));

    playButton_.onClick = [this] {
        if (onPlay)
            onPlay();
    };
    stopButton_.onClick = [this] {
        if (onStop)
            onStop();
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

void TransportBar::refreshFromTransport()
{
    if (transport_ == nullptr)
        return;

    bpmEditor_.setText(juce::String(transport_->bpm(), 1), juce::dontSendNotification);
    statusLabel_.setText(transport_->isPlaying() ? "Playing" : "Ready",
                         juce::dontSendNotification);
    playButton_.setEnabled(! transport_->isPlaying());
}

void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12151a));
    g.setColour(juce::Colour(0xff2a303a));
    g.drawLine(0.0f, 0.0f, static_cast<float>(getWidth()), 0.0f, 1.0f);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced(12, 10);
    playButton_.setBounds(area.removeFromLeft(80));
    area.removeFromLeft(8);
    stopButton_.setBounds(area.removeFromLeft(80));
    area.removeFromLeft(16);
    bpmLabel_.setBounds(area.removeFromLeft(40));
    area.removeFromLeft(6);
    bpmEditor_.setBounds(area.removeFromLeft(64));
    statusLabel_.setBounds(area.removeFromRight(120));
}
