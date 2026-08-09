#include "MainComponent.hpp"

MainComponent::MainComponent()
{
    setSize(1440, 900);
    setOpaque(true);
    setWantsKeyboardFocus(true);

    brandLabel_.setText("Sensei Studio", juce::dontSendNotification);
    brandLabel_.setFont(juce::FontOptions(28.0f).withStyle("Bold"));
    brandLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f5f7));
    subtitleLabel_.setText("Milestone E — sound identity + instruments", juce::dontSendNotification);
    subtitleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    helpLabel_.setText("Loop → song shape → move/duplicate clips → Play whole song · Sensei never forces",
                       juce::dontSendNotification);
    helpLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    positionLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    positionLabel_.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(brandLabel_);
    addAndMakeVisible(subtitleLabel_);
    addAndMakeVisible(helpLabel_);
    addAndMakeVisible(positionLabel_);
    addAndMakeVisible(arrangeBtn_);
    addAndMakeVisible(editBtn_);
    addAndMakeVisible(transportBar_);
    addAndMakeVisible(trackList_);
    addAndMakeVisible(instrumentPicker_);
    addAndMakeVisible(chordHelper_);
    addAndMakeVisible(arrangementView_);
    addAndMakeVisible(drumGrid_);
    addAndMakeVisible(pianoRoll_);
    addAndMakeVisible(senseiPanel_);

    audioEngine_.setTransport(&document_.transport());
    audioEngine_.setSnapshotPublisher(&document_.snapshots());
    if (! audioEngine_.initialise())
        subtitleLabel_.setText("Milestone D — audio device init failed", juce::dontSendNotification);

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
        arrangementView_.setPlayheadBeats(0.0);
    };
    transportBar_.onBpmChanged = [this](double bpm) {
        document_.setBpm(bpm);
        transportBar_.refreshFromTransport();
    };

    trackList_.setDocument(&document_);
    trackList_.onSelectionChanged = [this] { refreshAll(); };

    instrumentPicker_.setDocument(&document_);
    instrumentPicker_.onChanged = [this] {
        audioEngine_.allNotesOff();
        handleProjectEdited();
    };

    chordHelper_.setDocument(&document_);
    chordHelper_.onApplied = [this] { handleProjectEdited(); };

    arrangementView_.setDocument(&document_);
    arrangementView_.onEdited = [this] { handleProjectEdited(); };
    arrangementView_.onSelectionChanged = [this] { refreshAll(); };

    drumGrid_.setDocument(&document_);
    drumGrid_.onEdited = [this] { handleProjectEdited(); };

    pianoRoll_.setDocument(&document_);
    pianoRoll_.onAuditionNoteOn = [this](int midi, float vel) {
        audioEngine_.noteOn(auditionInstrument(), midi, vel);
    };
    pianoRoll_.onAuditionNoteOff = [this](int midi) {
        audioEngine_.noteOff(auditionInstrument(), midi);
    };
    pianoRoll_.onProjectEdited = [this] { handleProjectEdited(); };

    senseiPanel_.setDocument(&document_);
    senseiPanel_.onChanged = [this] { refreshAll(); };

    arrangeBtn_.onClick = [this] {
        showArrange_ = true;
        refreshAll();
    };
    editBtn_.onClick = [this] {
        showArrange_ = false;
        refreshAll();
    };

    refreshAll();
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioEngine_.shutdown();
}

sensei::core::InstrumentId MainComponent::auditionInstrument() const
{
    if (const auto* t = document_.project().findTrack(document_.selectedTrackId()))
    {
        if (t->type == sensei::core::TrackType::Drums)
            return sensei::core::InstrumentId::WarmKeys; // pitched audition N/A on drums
        if (sensei::core::isValidInstrumentId(t->instrumentId)
            && ! sensei::core::instrumentInfo(t->instrumentId).isDrumKit)
            return t->instrumentId;
        return sensei::core::defaultInstrumentForRole(t->role);
    }
    return sensei::core::InstrumentId::WarmKeys;
}

void MainComponent::refreshAll()
{
    trackList_.rebuild();
    instrumentPicker_.refresh();
    pianoRoll_.repaint();
    drumGrid_.repaint();
    arrangementView_.repaint();
    senseiPanel_.refresh(true);

    const auto* track = document_.project().findTrack(document_.selectedTrackId());
    const bool drums = track != nullptr && track->type == sensei::core::TrackType::Drums;
    arrangementView_.setVisible(showArrange_);
    chordHelper_.setVisible(! showArrange_ && ! drums);
    pianoRoll_.setVisible(! showArrange_ && ! drums);
    drumGrid_.setVisible(! showArrange_ && drums);
    resized();
}

void MainComponent::handleProjectEdited()
{
    document_.publishSnapshot();
    refreshAll();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0f1115));
    g.setColour(juce::Colour(0x14d5ff5c));
    g.fillRect(getLocalBounds().removeFromTop(72));
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
    senseiPanel_.setBounds(area.removeFromRight(340));
    auto left = area.removeFromLeft(160);
    trackList_.setBounds(left.removeFromTop(left.getHeight() - 110));
    instrumentPicker_.setBounds(left);

    auto center = area.reduced(10);
    helpLabel_.setBounds(center.removeFromTop(20));
    auto meta = center.removeFromTop(24);
    positionLabel_.setBounds(meta.removeFromRight(meta.getWidth() / 2));
    arrangeBtn_.setBounds(meta.removeFromLeft(90).reduced(2));
    editBtn_.setBounds(meta.removeFromLeft(90).reduced(2));
    center.removeFromTop(6);

    if (showArrange_)
    {
        arrangementView_.setBounds(center);
    }
    else
    {
        if (chordHelper_.isVisible())
        {
            chordHelper_.setBounds(center.removeFromTop(160));
            center.removeFromTop(8);
        }
        if (drumGrid_.isVisible())
            drumGrid_.setBounds(center);
        else
            pianoRoll_.setBounds(center);
    }
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
    if (showArrange_ && arrangementView_.keyPressed(key))
        return true;
    return pianoRoll_.keyPressed(key);
}

void MainComponent::timerCallback()
{
    transportBar_.refreshFromTransport();
    const auto beats = document_.transport().positionBeats();
    positionLabel_.setText("Pos " + juce::String(beats, 2) + " / "
                               + juce::String(document_.project().songLengthBeats(), 0)
                               + " · notes " + juce::String((int) document_.project().totalNoteCount())
                               + " · drums " + juce::String((int) document_.project().totalDrumHitCount()),
                           juce::dontSendNotification);
    pianoRoll_.setPlayheadBeats(beats);
    arrangementView_.setPlayheadBeats(beats);
}
