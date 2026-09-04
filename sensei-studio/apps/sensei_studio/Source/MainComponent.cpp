#include "MainComponent.hpp"

#include "sensei/core/commands/ArrangementCommands.hpp"

MainComponent::MainComponent()
{
    setSize(1480, 920);
    setOpaque(true);
    setWantsKeyboardFocus(true);

    brandLabel_.setText("Sensei Studio", juce::dontSendNotification);
    brandLabel_.setFont(juce::FontOptions(22.0f).withStyle("Bold"));

    for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i)
    {
        const auto id = static_cast<ThemeId>(i);
        themeBox_.addItem(themeDisplayName(id), i + 1);
    }
    themeBox_.setSelectedId(static_cast<int>(themeController_.themeId()) + 1, juce::dontSendNotification);
    themeBox_.onChange = [this] {
        const int id = themeBox_.getSelectedId();
        if (id <= 0)
            return;
        themeController_.setTheme(static_cast<ThemeId>(id - 1));
    };

    themeController_.onThemeChanged = [this] {
        applyThemeToChrome();
        refreshAll();
    };

    addAndMakeVisible(brandLabel_);
    addAndMakeVisible(themeBox_);
    addAndMakeVisible(transportBar_);
    addAndMakeVisible(browserPanel_);
    addAndMakeVisible(arrangementView_);
    addAndMakeVisible(splitter_);
    addAndMakeVisible(editorDock_);
    addAndMakeVisible(senseiPanel_);
    addAndMakeVisible(viewControlBar_);

    audioEngine_.setTransport(&document_.transport());
    audioEngine_.setSnapshotPublisher(&document_.snapshots());
    const bool audioOk = audioEngine_.initialise();
    transportBar_.setAudioDeviceAvailable(audioOk);

    transportBar_.setTransport(&document_.transport());
    transportBar_.onPlay = [this] {
        document_.transport().play();
        transportBar_.refreshFromTransport();
    };
    transportBar_.onStop = [this] {
        document_.transport().stop();
        audioEngine_.allNotesOff();
        transportBar_.refreshFromTransport();
        editorDock_.setPlayheadBeats(0.0);
        arrangementView_.setPlayheadBeats(0.0);
    };
    transportBar_.onBpmChanged = [this](double bpm) {
        document_.setBpm(bpm);
        transportBar_.refreshFromTransport();
    };
    transportBar_.onToggleLoop = [this] { toggleLoop(); };

    browserPanel_.setDocument(&document_);
    browserPanel_.setCollapsed(themeController_.browserCollapsed());
    browserPanel_.onChanged = [this] {
        audioEngine_.allNotesOff();
        handleProjectEdited();
    };
    browserPanel_.onCollapseToggle = [this] { toggleCreateView(); };

    arrangementView_.setDocument(&document_);
    arrangementView_.onEdited = [this] { handleProjectEdited(); };
    arrangementView_.onSelectionChanged = [this] {
        // Selection drives the contextual editor. If the user explicitly closed
        // Editor we respect that choice; reopening follows the current clip/track.
        refreshAll();
    };

    editorDock_.setDocument(&document_);
    editorDock_.onEdited = [this] {
        audioEngine_.allNotesOff();
        handleProjectEdited();
    };
    editorDock_.onAuditionNoteOn = [this](int midi, float vel) {
        audioEngine_.noteOn(auditionInstrument(), midi, vel);
    };
    editorDock_.onAuditionNoteOff = [this](int midi) {
        audioEngine_.noteOff(auditionInstrument(), midi);
    };

    senseiPanel_.setDocument(&document_);
    senseiPanel_.setCollapsed(themeController_.senseiCollapsed());
    senseiPanel_.onChanged = [this] { refreshAll(); };
    senseiPanel_.onCollapseToggle = [this] { toggleSenseiView(); };

    viewControlBar_.onCreate = [this] { toggleCreateView(); };
    viewControlBar_.onEdit = [this] { toggleEditView(); };
    viewControlBar_.onMixer = [] {
        // Deliberately unavailable until the real mixer exists. Never expose a
        // dead production surface simply to make the interface look complete.
    };
    viewControlBar_.onSensei = [this] { toggleSenseiView(); };
    viewControlBar_.onFocusArrangement = [this] { focusArrangement(); };
    viewControlBar_.setMixerAvailable(false);

    splitter_.onDragDelta = [this](int deltaY) {
        if (! editorOpen_)
            return;
        const int workspaceH = juce::jmax(1, getHeight() - 140);
        const float current = themeController_.editorHeightFraction();
        const float editorH = current * (float) workspaceH;
        const float next = (editorH - (float) deltaY) / (float) workspaceH;
        themeController_.setEditorHeightFraction(next);
        resized();
    };

    applyThemeToChrome();
    refreshAll();
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioEngine_.shutdown();
}

void MainComponent::applyThemeToChrome()
{
    const auto& p = themeController_.palette();
    brandLabel_.setColour(juce::Label::textColourId, p.textPrimary);
    themeBox_.setSelectedId(static_cast<int>(themeController_.themeId()) + 1, juce::dontSendNotification);
    transportBar_.applyThemeColours();
    repaint();
}

void MainComponent::toggleLoop()
{
    const auto& loop = document_.project().loop();
    document_.execute(std::make_unique<sensei::core::SetLoopRegionCommand>(
        loop.startBeat, loop.lengthBeats, ! loop.enabled));
    handleProjectEdited();
}

void MainComponent::toggleCreateView()
{
    const bool nextCollapsed = ! browserPanel_.isCollapsed();
    themeController_.setBrowserCollapsed(nextCollapsed);
    browserPanel_.setCollapsed(nextCollapsed);
    resized();
}

void MainComponent::toggleEditView()
{
    editorOpen_ = ! editorOpen_;
    resized();
}

void MainComponent::toggleSenseiView()
{
    const bool nextCollapsed = ! senseiPanel_.isCollapsed();
    themeController_.setSenseiCollapsed(nextCollapsed);
    senseiPanel_.setCollapsed(nextCollapsed);
    resized();
}

void MainComponent::focusArrangement()
{
    // Logic/Ableton-style focus action: collapse supporting working areas without
    // changing the project. The song immediately reclaims the whole workspace.
    themeController_.setBrowserCollapsed(true);
    themeController_.setSenseiCollapsed(true);
    browserPanel_.setCollapsed(true);
    senseiPanel_.setCollapsed(true);
    editorOpen_ = false;
    resized();
}

void MainComponent::syncViewControls()
{
    viewControlBar_.setStates(browserPanel_.isVisible() && ! browserPanel_.isCollapsed(),
                              editorOpen_, false,
                              senseiPanel_.isVisible() && ! senseiPanel_.isCollapsed());
}

sensei::core::InstrumentId MainComponent::auditionInstrument() const
{
    if (const auto* t = document_.project().findTrack(document_.selectedTrackId()))
    {
        if (t->type == sensei::core::TrackType::Drums)
            return sensei::core::InstrumentId::StudioKitBasic;
        if (sensei::core::isValidInstrumentId(t->instrumentId)
            && ! sensei::core::instrumentInfo(t->instrumentId).isDrumKit)
            return t->instrumentId;
        return sensei::core::defaultInstrumentForRole(t->role);
    }
    return sensei::core::InstrumentId::WarmKeys;
}

void MainComponent::refreshAll()
{
    browserPanel_.setDocument(&document_);
    editorDock_.refresh();
    arrangementView_.repaint();
    senseiPanel_.refresh(true);
    transportBar_.refreshFromTransport();
    resized();
}

void MainComponent::handleProjectEdited()
{
    document_.publishSnapshot();
    refreshAll();
}

void MainComponent::paint(juce::Graphics& g)
{
    const auto& p = themeController_.palette();
    g.fillAll(p.bg0);

    juce::ColourGradient header(p.accentSoft, 0.0f, 0.0f,
                                juce::Colours::transparentBlack, 0.0f, 58.0f, false);
    g.setGradientFill(header);
    g.fillRect(0, 0, getWidth(), 58);

    drawSenseiOrb(g, { 13.0f, 10.0f, 26.0f, 26.0f }, p, 0.75f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Professional DAW hierarchy: a thin transport strip, then the song.
    auto top = area.removeFromTop(46).reduced(46, 5);
    brandLabel_.setBounds(top.removeFromLeft(195));
    themeBox_.setBounds(top.removeFromRight(138).reduced(0, 2));
    top.removeFromRight(8);
    transportBar_.setBounds(top);

    // A slim edge strip controls supporting work areas. Unlike the earlier
    // full-width footer it costs very little song height and reads as utility.
    constexpr int viewBarH = 36;
    auto viewStrip = area.removeFromBottom(viewBarH);
    constexpr int viewBarW = 346;
    viewControlBar_.setBounds(viewStrip.removeFromRight(viewBarW).reduced(4, 2));

    const bool browserOpen = ! browserPanel_.isCollapsed();
    const bool senseiOpen = ! senseiPanel_.isCollapsed();

    browserPanel_.setVisible(browserOpen);
    senseiPanel_.setVisible(senseiOpen);

    // Folded panels consume zero pixels. The arrangement gets every pixel that
    // supporting tools are not actively using.
    if (browserOpen)
    {
        const int browserW = juce::jlimit(196, 232, getWidth() / 7);
        browserPanel_.setBounds(area.removeFromLeft(browserW));
    }
    else
    {
        browserPanel_.setBounds({});
    }

    if (senseiOpen)
    {
        const int senseiW = juce::jlimit(266, 314, getWidth() / 5);
        senseiPanel_.setBounds(area.removeFromRight(senseiW));
    }
    else
    {
        senseiPanel_.setBounds({});
    }

    editorDock_.setVisible(editorOpen_);
    splitter_.setVisible(editorOpen_);

    if (! editorOpen_)
    {
        arrangementView_.setBounds(area);
        syncViewControls();
        return;
    }

    constexpr int splitterH = 7;
    const int workspaceH = juce::jmax(180, area.getHeight());
    int editorH = juce::roundToInt(themeController_.editorHeightFraction() * (float) workspaceH);
    editorH = juce::jlimit(120, workspaceH - 160, editorH);
    const int arrangeH = workspaceH - editorH - splitterH;

    arrangementView_.setBounds(area.removeFromTop(arrangeH));
    splitter_.setBounds(area.removeFromTop(splitterH));
    editorDock_.setBounds(area);
    syncViewControls();
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    const bool primary = mods.isCommandDown() || mods.isCtrlDown();

    if (primary)
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

        // Visible controls remain primary; shortcuts are accelerators only.
        if (mods.isAltDown())
        {
            if (key.getKeyCode() == 'b' || key.getKeyCode() == 'B')
            {
                toggleCreateView();
                return true;
            }
            if (key.getKeyCode() == 'e' || key.getKeyCode() == 'E')
            {
                toggleEditView();
                return true;
            }
            if (key.getKeyCode() == 's' || key.getKeyCode() == 'S')
            {
                toggleSenseiView();
                return true;
            }
            if (key.getKeyCode() == 'f' || key.getKeyCode() == 'F')
            {
                focusArrangement();
                return true;
            }
        }
    }

    if (arrangementView_.keyPressed(key))
        return true;
    if (editorOpen_)
        return editorDock_.keyPressed(key);
    return false;
}

void MainComponent::timerCallback()
{
    const auto beats = document_.transport().positionBeats();
    transportBar_.setPositionBeats(beats, document_.project().songLengthBeats(),
                                   document_.project().loop().enabled);
    transportBar_.refreshFromTransport();
    editorDock_.setPlayheadBeats(beats);
    arrangementView_.setPlayheadBeats(beats);
}
