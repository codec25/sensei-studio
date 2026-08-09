#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/harmony/Progressions.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class ChordHelperPanel final : public juce::Component
{
public:
    std::function<void()> onApplied;

    ChordHelperPanel()
    {
        keyBox_.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
        keyBox_.setSelectedItemIndex(0);
        modeBox_.addItemList({ "Major", "Minor" }, 1);
        modeBox_.setSelectedItemIndex(0);

        for (int i = 0; i < sensei::core::kProgressionCount; ++i)
            progBox_.addItem(sensei::core::kProgressions[i].displayName, i + 1);
        progBox_.setSelectedItemIndex(0);

        applyBtn_.setButtonText("Insert progression");
        applyBtn_.onClick = [this] { apply(); };

        explain_.setJustificationType(juce::Justification::topLeft);
        updateExplain();

        progBox_.onChange = [this] { updateExplain(); };

        addAndMakeVisible(keyBox_);
        addAndMakeVisible(modeBox_);
        addAndMakeVisible(progBox_);
        addAndMakeVisible(applyBtn_);
        addAndMakeVisible(explain_);
        addAndMakeVisible(title_);
        title_.setText("Chord helper", juce::dontSendNotification);
        title_.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    }

    void setDocument(sensei::core::Document* document) { document_ = document; }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        explain_.setColour(juce::Label::textColourId, p.textSecondary);
        title_.setColour(juce::Label::textColourId, p.textPrimary);
        g.setColour(p.bg2);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    }

    void resized() override
    {
        auto a = getLocalBounds().reduced(12, 10);
        title_.setBounds(a.removeFromTop(22));
        a.removeFromTop(6);
        auto row = a.removeFromTop(28);
        keyBox_.setBounds(row.removeFromLeft(70));
        row.removeFromLeft(6);
        modeBox_.setBounds(row.removeFromLeft(90));
        a.removeFromTop(8);
        progBox_.setBounds(a.removeFromTop(28));
        a.removeFromTop(8);
        applyBtn_.setBounds(a.removeFromTop(32).removeFromLeft(180));
        a.removeFromTop(8);
        explain_.setBounds(a);
    }

private:
    void updateExplain()
    {
        const int idx = juce::jlimit(0, sensei::core::kProgressionCount - 1, progBox_.getSelectedItemIndex());
        const auto& p = sensei::core::kProgressions[idx];
        explain_.setText(juce::String(p.explanation) + "\nRomans: " + p.displayName,
                         juce::dontSendNotification);
    }

    void apply()
    {
        if (document_ == nullptr)
            return;
        const int root = keyBox_.getSelectedItemIndex();
        const auto mode = modeBox_.getSelectedItemIndex() == 0 ? sensei::core::ScaleMode::Major
                                                               : sensei::core::ScaleMode::Minor;
        const int idx = juce::jlimit(0, sensei::core::kProgressionCount - 1, progBox_.getSelectedItemIndex());
        if (document_->applyProgression(root, mode, sensei::core::kProgressions[idx].id))
        {
            if (onApplied)
                onApplied();
        }
    }

    sensei::core::Document* document_ = nullptr;
    juce::Label title_;
    juce::ComboBox keyBox_, modeBox_, progBox_;
    juce::TextButton applyBtn_;
    juce::Label explain_;
};
