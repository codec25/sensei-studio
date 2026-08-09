#pragma once

#include "ArrangementView.hpp"
#include "BrowserPanel.hpp"
#include "EditorDock.hpp"
#include "SenseiPanel.hpp"
#include "TransportBar.hpp"
#include "VerticalSplitter.hpp"
#include "ui/ThemeController.hpp"

#include "sensei/core/Document.hpp"
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
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void refreshAll();
    void handleProjectEdited();
    void applyThemeToChrome();
    void toggleLoop();
    sensei::core::InstrumentId auditionInstrument() const;

    ThemeController themeController_;
    sensei::core::Document document_;
    sensei::engine::AudioEngine audioEngine_;

    juce::Label brandLabel_;
    juce::ComboBox themeBox_;
    TransportBar transportBar_;
    BrowserPanel browserPanel_;
    ArrangementView arrangementView_;
    VerticalSplitter splitter_;
    EditorDock editorDock_;
    SenseiPanel senseiPanel_;
};
