#pragma once

#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// F.2B workspace controls: these are compact show/hide affordances around the
// song, not five equal app destinations. Arrangement remains visually dominant.
class ViewControlBar final : public juce::Component
{
public:
    std::function<void()> onCreate;
    std::function<void()> onEdit;
    std::function<void()> onMixer;
    std::function<void()> onSensei;
    std::function<void()> onFocusArrangement;

    ViewControlBar()
    {
        configure(create_, "Create", "Show or hide the sound browser");
        configure(edit_, "Edit", "Show or hide the selected clip editor");
        configure(mixer_, "Mix", "Show or hide the mixer");
        configure(sensei_, "Sensei", "Show or hide Sensei guidance");
        configure(focus_, "Focus", "Fold supporting areas and give the song the full workspace");

        create_.onClick = [this] { if (onCreate) onCreate(); };
        edit_.onClick = [this] { if (onEdit) onEdit(); };
        mixer_.onClick = [this] { if (onMixer) onMixer(); };
        sensei_.onClick = [this] { if (onSensei) onSensei(); };
        focus_.onClick = [this] { if (onFocusArrangement) onFocusArrangement(); };

        addAndMakeVisible(create_);
        addAndMakeVisible(edit_);
        addAndMakeVisible(mixer_);
        addAndMakeVisible(sensei_);
        addAndMakeVisible(focus_);
    }

    void setStates(bool createOpen, bool editOpen, bool mixerOpen, bool senseiOpen)
    {
        create_.setToggleState(createOpen, juce::dontSendNotification);
        edit_.setToggleState(editOpen, juce::dontSendNotification);
        mixer_.setToggleState(mixerOpen, juce::dontSendNotification);
        sensei_.setToggleState(senseiOpen, juce::dontSendNotification);
    }

    void setMixerAvailable(bool available)
    {
        mixer_.setEnabled(available);
        mixer_.setTooltip(available ? "Show or hide Mixer"
                                    : "Mixer unlocks when its real audio controls are wired");
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(p.transportBg.withAlpha(0.96f));
        g.fillRoundedRectangle(r, 8.0f);
        g.setColour(p.borderSoft.withAlpha(0.72f));
        g.drawRoundedRectangle(r, 8.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6, 4);
        constexpr int gap = 4;
        constexpr int primaryW = 64;
        constexpr int senseiW = 72;
        constexpr int focusW = 58;

        for (auto* button : { &create_, &edit_, &mixer_ })
        {
            button->setBounds(area.removeFromLeft(primaryW));
            area.removeFromLeft(gap);
        }

        sensei_.setBounds(area.removeFromLeft(senseiW));
        area.removeFromLeft(gap + 4);
        focus_.setBounds(area.removeFromLeft(focusW));
    }

private:
    static void configure(juce::TextButton& button, const juce::String& text,
                          const juce::String& tooltip)
    {
        button.setButtonText(text);
        button.setTooltip(tooltip);
        button.setClickingTogglesState(false);
    }

    juce::TextButton create_;
    juce::TextButton edit_;
    juce::TextButton mixer_;
    juce::TextButton sensei_;
    juce::TextButton focus_;
};
