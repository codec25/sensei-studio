#include "MainComponent.hpp"

MainComponent::MainComponent()
{
    setSize(1100, 640);
    setOpaque(true);

    brandLabel_.setText("Sensei Studio", juce::dontSendNotification);
    brandLabel_.setFont(juce::FontOptions(28.0f).withStyle("Bold"));
    brandLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f5f7));

    subtitleLabel_.setText("Milestone A — native heartbeat", juce::dontSendNotification);
    subtitleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));

    positionLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    positionLabel_.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(brandLabel_);
    addAndMakeVisible(subtitleLabel_);
    addAndMakeVisible(transportBar_);
    addAndMakeVisible(keyboard_);
    addAndMakeVisible(senseiPanel_);
    addAndMakeVisible(positionLabel_);

    audioEngine_.setTransport(&transport_);
    if (! audioEngine_.initialise())
    {
        subtitleLabel_.setText("Milestone A — audio device init failed (UI still available)",
                               juce::dontSendNotification);
    }

    transportBar_.setTransport(&transport_);
    transportBar_.onPlay = [this] {
        transport_.play();
        transportBar_.refreshFromTransport();
    };
    transportBar_.onStop = [this] {
        transport_.stop();
        audioEngine_.allNotesOff();
        transportBar_.refreshFromTransport();
    };
    transportBar_.onBpmChanged = [this](double bpm) {
        transport_.setBpm(bpm);
        transportBar_.refreshFromTransport();
    };

    keyboard_.onNoteOn = [this](int midi) { audioEngine_.noteOn(midi, 0.85f); };
    keyboard_.onNoteOff = [this](int midi) { audioEngine_.noteOff(midi); };

    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioEngine_.shutdown();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0f1115));

    auto header = getLocalBounds().removeFromTop(72);
    g.setColour(juce::Colour(0x14d5ff5c));
    g.fillRect(header);

    g.setColour(juce::Colour(0xffd5ff5c));
    g.fillRoundedRectangle(16.0f, 18.0f, 36.0f, 36.0f, 10.0f);
    g.setColour(juce::Colours::black);
    g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    g.drawText("S", juce::Rectangle<int>(16, 18, 36, 36), juce::Justification::centred);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(72).reduced(64, 12);
    brandLabel_.setBounds(top.removeFromTop(32));
    subtitleLabel_.setBounds(top);

    transportBar_.setBounds(area.removeFromBottom(58));

    auto right = area.removeFromRight(320);
    senseiPanel_.setBounds(right);

    auto center = area.reduced(16);
    positionLabel_.setBounds(center.removeFromTop(24));
    center.removeFromTop(8);
    keyboard_.setBounds(center.removeFromTop(180));
}

void MainComponent::timerCallback()
{
    transportBar_.refreshFromTransport();
    positionLabel_.setText("Position: " + juce::String(transport_.positionBeats(), 2) + " beats",
                           juce::dontSendNotification);
}
