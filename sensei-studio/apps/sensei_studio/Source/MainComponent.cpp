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
    browserPanel_.onCollapseToggle = [this] {
        themeController_.setBrowserCollapsed(! themeController_.browserCollapsed());
        browserPanel_.setCollapsed(themeController_.browserCollapsed());
        resized();
    };

    arrangementView_.setDocument(&document_);
    arrangementView_.onEdited = [this] { handleProjectEdited(); };
    arrangementView_.onSelectionChanged = [this] { refreshAll(); };

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
    senseiPanel_.onCollapseToggle = [this] {
        themeController_.setSenseiCollapsed(! themeController_.senseiCollapsed());
        senseiPanel_.setCollapsed(themeController_.senseiCollapsed());
        resized();
    };

    splitter_.onDragDelta = [this](int deltaY) {
        const int workspaceH = juce::jmax(1, getHeight() - 108);
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
                                juce::Colours::transparentBlack, 0.0f, 72.0f, false);
    g.setGradientFill(header);
    g.fillRect(0, 0, getWidth(), 72);

    drawSenseiOrb(g, { 16.0f, 14.0f, 32.0f, 32.0f }, p, 0.75f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(56).reduced(56, 8);
    brandLabel_.setBounds(top.removeFromLeft(220));
    themeBox_.setBounds(top.removeFromRight(160).reduced(0, 4));
    top.removeFromRight(12);
    transportBar_.setBounds(top);

    // F.1 productivity rule: the song owns the screen. Supporting panels collapse
    // automatically on tighter layouts, while the user's saved desktop choices
    // return as soon as enough horizontal space is available again.
    const bool compact = getWidth() < 1180;
    const bool focused = getWidth() < 1380;
    const bool browserCollapsed = compact || themeController_.browserCollapsed();
    const bool senseiCollapsed = focused || themeController_.senseiCollapsed();
    browserPanel_.setCollapsed(browserCollapsed);
    senseiPanel_.setCollapsed(senseiCollapsed);

    const int browserW = browserCollapsed ? 44 : juce::jlimit(200, 236, getWidth() / 7);
    const int senseiW = senseiCollapsed ? 44 : juce::jlimit(280, 320, getWidth() / 5);
    browserPanel_.setBounds(area.removeFromLeft(browserW));
    senseiPanel_.setBounds(area.removeFromRight(senseiW));

    constexpr int splitterH = 10;
    const int workspaceH = juce::jmax(180, area.getHeight());
    int editorH = juce::roundToInt(themeController_.editorHeightFraction() * (float) workspaceH);
    editorH = juce::jlimit(120, workspaceH - 160, editorH);
    const int arrangeH = workspaceH - editorH - splitterH;

    arrangementView_.setBounds(area.removeFromTop(arrangeH));
    splitter_.setBounds(area.removeFromTop(splitterH));
    editorDock_.setBounds(area);
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
    if (arrangementView_.keyPressed(key))
        return true;
    return editorDock_.keyPressed(key);
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
