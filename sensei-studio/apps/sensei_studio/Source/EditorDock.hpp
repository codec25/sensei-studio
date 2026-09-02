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

// Context-sensitive bottom editor: the selected musical object decides what the
// producer sees. Supporting tools must never steal space from the actual editor.
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
        addAndMakeVisible(contextLabel_);
        title_.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
        contextLabel_.setFont(juce::FontOptions(12.0f));
        contextLabel_.setJustificationType(juce::Justification::centredRight);

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
            contextLabel_.setText(drums ? "Beat editor"
                                        : (chords ? "Chords + notes" : "Note editor"),
                                  juce::dontSendNotification);
        }
        else
        {
            title_.setText("Editor", juce::dontSendNotification);
            contextLabel_.setText("Select a track to edit", juce::dontSendNotification);
        }
        contextLabel_.setColour(juce::Label::textColourId, studioPalette().textMuted);
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        g.fillAll(p.bg1);
        g.setColour(p.borderSoft.withAlpha(0.55f));
        g.drawLine(0.0f, 0.5f, (float) getWidth(), 0.5f);

        // A thin role-neutral focus line ties the editor to the active context
        // without turning the whole dock into another card.
        g.setColour(p.accent.withAlpha(0.55f));
        g.fillRect(0, 0, 2, getHeight());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12, 8);
        auto heading = area.removeFromTop(24);
        contextLabel_.setBounds(heading.removeFromRight(juce::jmin(150, heading.getWidth() / 3)));
        title_.setBounds(heading);
        area.removeFromTop(6);

        // Keep the playable keyboard available when there is room, but let the
        // actual note/beat editor win on shorter docks.
        const bool roomy = area.getHeight() >= 190;
        const int stripH = roomy ? 64 : 48;
        auto strip = area.removeFromTop(juce::jmin(stripH, area.getHeight()));
        instrumentPicker_.setBounds(strip.removeFromLeft(juce::jmin(260, juce::jmax(180, strip.getWidth() / 3))));
        if (keyboard_.isVisible())
        {
            strip.removeFromLeft(8);
            keyboard_.setBounds(strip);
        }

        area.removeFromTop(roomy ? 8 : 4);
        if (chordHelper_.isVisible())
        {
            // Chord guidance supports the note editor; it no longer consumes a
            // third of the dock by default.
            const int helperH = juce::jmin(roomy ? 96 : 72, juce::jmax(0, area.getHeight() / 3));
            chordHelper_.setBounds(area.removeFromTop(helperH));
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
    juce::Label contextLabel_;
    InstrumentPicker instrumentPicker_;
    ChordHelperPanel chordHelper_;
    PianoRoll pianoRoll_;
    DrumGrid drumGrid_;
    NoteKeyboard keyboard_;
};
