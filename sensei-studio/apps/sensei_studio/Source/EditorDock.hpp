#pragma once

#include "ChordHelperPanel.hpp"
#include "DrumGrid.hpp"
#include "InstrumentPicker.hpp"
#include "NoteKeyboard.hpp"
#include "PianoRoll.hpp"
#include "ui/Theme.hpp"

#include "sensei/core/Document.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// Context-sensitive bottom editor: instrument strip + piano roll / drum grid.
class EditorDock final : public juce::Component
{
public:
    std::function<void()> onEdited;
    std::function<void(int, float)> onAuditionNoteOn;
    std::function<void(int)> onAuditionNoteOff;

    EditorDock()
    {
        addAndMakeVisible(instrumentPicker_);
        addAndMakeVisible(chordHelper_);
        addAndMakeVisible(pianoRoll_);
        addAndMakeVisible(drumGrid_);
        addAndMakeVisible(keyboard_);
        addAndMakeVisible(title_);
        title_.setFont(juce::FontOptions(15.0f).withStyle("Bold"));

        instrumentPicker_.onChanged = [this] {
            if (onEdited)
                onEdited();
        };
        chordHelper_.onApplied = [this] {
            if (onEdited)
                onEdited();
        };
        drumGrid_.onEdited = [this] {
            if (onEdited)
                onEdited();
        };
        pianoRoll_.onProjectEdited = [this] {
            if (onEdited)
                onEdited();
        };
        pianoRoll_.onAuditionNoteOn = [this](int m, float v) {
            if (onAuditionNoteOn)
                onAuditionNoteOn(m, v);
        };
        pianoRoll_.onAuditionNoteOff = [this](int m) {
            if (onAuditionNoteOff)
                onAuditionNoteOff(m);
        };
        keyboard_.onNoteOn = [this](int m) {
            if (onAuditionNoteOn)
                onAuditionNoteOn(m, 0.85f);
        };
        keyboard_.onNoteOff = [this](int m) {
            if (onAuditionNoteOff)
                onAuditionNoteOff(m);
        };
    }

    void setDocument(sensei::core::Document* document)
    {
        document_ = document;
        instrumentPicker_.setDocument(document);
        chordHelper_.setDocument(document);
        pianoRoll_.setDocument(document);
        drumGrid_.setDocument(document);
        refresh();
    }

    void setPlayheadBeats(double beats) { pianoRoll_.setPlayheadBeats(beats); }

    bool keyPressed(const juce::KeyPress& key) override
    {
        return pianoRoll_.keyPressed(key);
    }

    void refresh()
    {
        instrumentPicker_.refresh();
        pianoRoll_.repaint();
        drumGrid_.repaint();
        chordHelper_.repaint();

        const auto* track = document_ != nullptr
                                ? document_->project().findTrack(document_->selectedTrackId())
                                : nullptr;
        const bool drums = track != nullptr && track->type == sensei::core::TrackType::Drums;
        const bool chords = track != nullptr && track->role == sensei::core::TrackRole::Chords;

        drumGrid_.setVisible(drums);
        pianoRoll_.setVisible(! drums);
        keyboard_.setVisible(! drums);
        chordHelper_.setVisible(! drums && chords);

        if (track != nullptr)
        {
            const auto info = sensei::core::instrumentInfo(track->instrumentId);
            title_.setText(juce::String(track->name) + "  ·  " + info.displayName,
                           juce::dontSendNotification);
        }
        else
        {
            title_.setText("Editor", juce::dontSendNotification);
        }
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        g.fillAll(p.bg1);
        g.setColour(p.borderSoft.withAlpha(0.5f));
        g.drawLine(0.0f, 0.5f, (float) getWidth(), 0.5f);
        g.setColour(p.accentSoft);
        g.fillRect(0, 0, 3, getHeight());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12, 8);
        title_.setBounds(area.removeFromTop(22));
        area.removeFromTop(6);

        auto strip = area.removeFromTop(72);
        instrumentPicker_.setBounds(strip.removeFromLeft(juce::jmin(280, strip.getWidth() / 3)));
        if (keyboard_.isVisible())
        {
            strip.removeFromLeft(8);
            keyboard_.setBounds(strip);
        }

        area.removeFromTop(8);
        if (chordHelper_.isVisible())
        {
            chordHelper_.setBounds(area.removeFromTop(juce::jmin(140, area.getHeight() / 3)));
            area.removeFromTop(6);
        }

        if (drumGrid_.isVisible())
            drumGrid_.setBounds(area);
        else
            pianoRoll_.setBounds(area);
    }

private:
    sensei::core::Document* document_ = nullptr;
    juce::Label title_;
    InstrumentPicker instrumentPicker_;
    ChordHelperPanel chordHelper_;
    PianoRoll pianoRoll_;
    DrumGrid drumGrid_;
    NoteKeyboard keyboard_;
};
