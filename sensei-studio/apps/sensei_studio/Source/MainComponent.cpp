#include "MainComponent.hpp"

MainComponent::MainComponent()
{
    setSize(1280, 760);
    setOpaque(true);
    setWantsKeyboardFocus(true);

    brandLabel_.setText("Sensei Studio", juce::dontSendNotification);
    brandLabel_.setFont(juce::FontOptions(28.0f).withStyle("Bold"));
    brandLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f5f7));

    subtitleLabel_.setText("Milestone B — first musical loop", juce::dontSendNotification);
    subtitleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));

    trackLabel_.setText("Track: Sensei Synth · 4 bars · 1/16 grid", juce::dontSendNotification);
    trackLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffdfe4eb));

    helpLabel_.setText("Click=add · drag=move · edge=resize · right-click/Del=delete · Ctrl/Cmd+Z undo",
                       juce::dontSendNotification);
    helpLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));

    positionLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    positionLabel_.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(brandLabel_);
    addAndMakeVisible(subtitleLabel_);
    addAndMakeVisible(trackLabel_);
    addAndMakeVisible(helpLabel_);
    addAndMakeVisible(transportBar_);
    addAndMakeVisible(pianoRoll_);
    addAndMakeVisible(senseiPanel_);
    addAndMakeVisible(positionLabel_);

    audioEngine_.setTransport(&document_.transport());
    audioEngine_.setSnapshotPublisher(&document_.snapshots());
    if (! audioEngine_.initialise())
    {
        subtitleLabel_.setText("Milestone B — audio device init failed (UI still available)",
                               juce::dontSendNotification);
    }

    transportBar_.setTransport(&document_.transport());
    transportBar_.onPlay = [this] {
        document_.transport().play();
        transportBar_.refreshFromTransport();
    };
    transportBar_.onStop = [this] {
        document_.transport().stop();
        audioEngine_.allNotesOff();
        transportBar_.refreshFromTransport();
        pianoRoll_.setPlayheadBeats(0.0);
    };
    transportBar_.onBpmChanged = [this](double bpm) {
        document_.setBpm(bpm);
        transportBar_.refreshFromTransport();
    };

    pianoRoll_.setDocument(&document_);
    pianoRoll_.onAuditionNoteOn = [this](int midi, float vel) { audioEngine_.noteOn(midi, vel); };
    pianoRoll_.onAuditionNoteOff = [this](int midi) { audioEngine_.noteOff(midi); };
    pianoRoll_.onProjectEdited = [this] { handleProjectEdited(); };

    refreshSensei(true);
    startTimerHz(30);
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
    trackLabel_.setBounds(center.removeFromTop(22));
    helpLabel_.setBounds(center.removeFromTop(20));
    positionLabel_.setBounds(center.removeFromTop(22));
    center.removeFromTop(8);
    pianoRoll_.setBounds(center);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown())
    {
        if (key.getKeyCode() == 'z' || key.getKeyCode() == 'Z')
        {
            if (mods.isShiftDown())
                document_.redo();
            else
                document_.undo();
            handleProjectEdited();
            return true;
        }
        if (key.getKeyCode() == 'y' || key.getKeyCode() == 'Y')
        {
            document_.redo();
            handleProjectEdited();
            return true;
        }
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        return pianoRoll_.keyPressed(key);

    return false;
}

void MainComponent::timerCallback()
{
    transportBar_.refreshFromTransport();
    const auto beats = document_.transport().positionBeats();
    positionLabel_.setText("Position: " + juce::String(beats, 2) + " beats · "
                               + juce::String(document_.project().totalNoteCount()) + " notes",
                           juce::dontSendNotification);
    pianoRoll_.setPlayheadBeats(beats);
}

void MainComponent::refreshSensei(bool force)
{
    senseiPanel_.setObservation(document_.analyze(), force);
}

void MainComponent::handleProjectEdited()
{
    pianoRoll_.repaint();
    refreshSensei(false);
    // High-signal: always show first note / empty / outside-loop style facts.
    const auto obs = document_.analyze();
    if (obs.kind == sensei::core::ObservationKind::NoNotes
        || obs.kind == sensei::core::ObservationKind::FirstIdea
        || obs.kind == sensei::core::ObservationKind::NotesOutsideLoop)
    {
        senseiPanel_.setObservation(obs, true);
    }
}
