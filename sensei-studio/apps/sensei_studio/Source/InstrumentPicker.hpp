#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/commands/InstrumentCommands.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class InstrumentPicker final : public juce::Component
{
public:
    std::function<void()> onChanged;

    InstrumentPicker()
    {
        label_.setText("Instrument", juce::dontSendNotification);
        label_.setFont(juce::FontOptions(12.5f));
        fact_.setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(label_);
        addAndMakeVisible(combo_);
        addAndMakeVisible(fact_);
        combo_.onChange = [this] { applySelection(); };
    }

    void setDocument(sensei::core::Document* document)
    {
        document_ = document;
        refresh();
    }

    void refresh()
    {
        updating_ = true;
        combo_.clear(juce::dontSendNotification);
        fact_.setText({}, juce::dontSendNotification);
        const auto& p = studioPalette();
        label_.setColour(juce::Label::textColourId, p.textMuted);
        fact_.setColour(juce::Label::textColourId, p.textSecondary);

        if (document_ == nullptr)
        {
            updating_ = false;
            return;
        }

        const auto* track = document_->project().findTrack(document_->selectedTrackId());
        if (track == nullptr)
        {
            updating_ = false;
            return;
        }

        if (track->type == sensei::core::TrackType::Drums)
            addItem(sensei::core::InstrumentId::StudioKitBasic);
        else
        {
            addItem(sensei::core::InstrumentId::WarmKeys);
            addItem(sensei::core::InstrumentId::DeepBass);
            addItem(sensei::core::InstrumentId::BrightPluck);
        }

        combo_.setSelectedId(static_cast<int>(track->instrumentId) + 1, juce::dontSendNotification);
        fact_.setText(sensei::core::instrumentInfo(track->instrumentId).shortFact,
                      juce::dontSendNotification);
        updating_ = false;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        g.setColour(p.bg2);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 8);
        label_.setBounds(area.removeFromTop(18));
        combo_.setBounds(area.removeFromTop(28));
        area.removeFromTop(4);
        fact_.setBounds(area.removeFromTop(28));
    }

private:
    void addItem(sensei::core::InstrumentId id)
    {
        const auto info = sensei::core::instrumentInfo(id);
        combo_.addItem(info.displayName, static_cast<int>(id) + 1);
    }

    void applySelection()
    {
        if (updating_ || document_ == nullptr)
            return;
        auto* track = document_->project().findTrack(document_->selectedTrackId());
        if (track == nullptr)
            return;
        const int selected = combo_.getSelectedId();
        if (selected <= 0)
            return;
        const auto id = static_cast<sensei::core::InstrumentId>(selected - 1);
        if (track->instrumentId == id)
            return;
        document_->execute(std::make_unique<sensei::core::SetTrackInstrumentCommand>(track->id, id));
        fact_.setText(sensei::core::instrumentInfo(id).shortFact, juce::dontSendNotification);
        if (onChanged)
            onChanged();
    }

    sensei::core::Document* document_ = nullptr;
    juce::Label label_;
    juce::ComboBox combo_;
    juce::Label fact_;
    bool updating_ = false;
};
