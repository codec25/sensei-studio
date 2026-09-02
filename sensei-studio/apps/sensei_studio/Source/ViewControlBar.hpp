#pragma once

#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// F.1 workspace navigation: visible, touch-safe view toggles with keyboard
// accelerators layered on top. Arrangement remains the primary workspace.
class ViewControlBar final : public juce::Component
{
public:
    std::function<void()> onCreate;
    std::function<void()> onEdit;
    std::function<void()> onMixer;
    std::function<void()> onSensei;

    ViewControlBar()
    {
        configure(create_, "Create", "Show or hide Create");
        configure(edit_, "Edit", "Show or hide the selected clip editor");
        configure(mixer_, "Mixer", "Mixer workspace");
        configure(sensei_, "Sensei", "Show or hide Sensei");

        create_.onClick = [this] { if (onCreate) onCreate(); };
        edit_.onClick = [this] { if (onEdit) onEdit(); };
        mixer_.onClick = [this] { if (onMixer) onMixer(); };
        sensei_.onClick = [this] { if (onSensei) onSensei(); };

        addAndMakeVisible(create_);
        addAndMakeVisible(edit_);
        addAndMakeVisible(mixer_);
        addAndMakeVisible(sensei_);
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
        mixer_.setTooltip(available ? "Show or hide Mixer" : "Mixer is coming in the next production pass");
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
        const int count = 4;
        const int buttonW = juce::jmin(112, (area.getWidth() - gap * (count - 1)) / count);
        const int totalW = buttonW * count + gap * (count - 1);
        area = area.withSizeKeepingCentre(totalW, area.getHeight());

        for (auto* button : { &create_, &edit_, &mixer_, &sensei_ })
        {
            button->setBounds(area.removeFromLeft(buttonW));
            area.removeFromLeft(gap);
        }
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
};
