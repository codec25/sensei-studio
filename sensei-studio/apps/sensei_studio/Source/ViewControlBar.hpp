#pragma once

#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// F.2A workspace controls: working areas fold around the song instead of behaving
// like equal destinations. Arrangement remains the anchor at all times.
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
        configure(edit_, "Editor", "Show or hide the selected clip editor");
        configure(mixer_, "Mixer", "Show or hide the mixer");
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
        g.fillAll(p.transportBg);
        g.setColour(p.borderSoft.withAlpha(0.7f));
        g.drawHorizontalLine(0, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 5);
        constexpr int gap = 6;
        const int buttonW = juce::jlimit(72, 104, juce::jmax(72, area.getWidth() / 8));

        // Working-area toggles stay grouped together, like a DAW view selector.
        for (auto* button : { &create_, &edit_, &mixer_, &sensei_ })
        {
            button->setBounds(area.removeFromLeft(buttonW));
            area.removeFromLeft(gap);
        }

        // Focus is intentionally separated: it is a workspace command, not a view.
        focus_.setBounds(area.removeFromRight(buttonW));
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
