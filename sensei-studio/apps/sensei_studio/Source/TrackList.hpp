#pragma once

#include "sensei/core/Document.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class TrackList final : public juce::Component
{
public:
    std::function<void()> onSelectionChanged;

    void setDocument(sensei::core::Document* document)
    {
        document_ = document;
        rebuild();
    }

    void rebuild()
    {
        buttons_.clear();
        if (document_ == nullptr)
            return;

        for (const auto& track : document_->project().tracks())
        {
            auto* b = buttons_.add(new juce::TextButton(track.name));
            b->setClickingTogglesState(true);
            const Id id = track.id;
            b->onClick = [this, id] {
                if (document_ == nullptr)
                    return;
                document_->setSelectedTrackId(id);
                refreshToggleState();
                if (onSelectionChanged)
                    onSelectionChanged();
            };
            addAndMakeVisible(b);
        }
        refreshToggleState();
        resized();
    }

    void refreshToggleState()
    {
        if (document_ == nullptr)
            return;
        int i = 0;
        for (const auto& track : document_->project().tracks())
        {
            if (auto* b = buttons_[i++])
                b->setToggleState(track.id == document_->selectedTrackId(), juce::dontSendNotification);
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff171a20));
        g.setColour(juce::Colour(0xff9ca6b5));
        g.setFont(12.0f);
        g.drawText("TRACKS", getLocalBounds().removeFromTop(22).reduced(8, 0),
                   juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto area = getLocalBounds().withTrimmedTop(24).reduced(8);
        for (auto* b : buttons_)
        {
            b->setBounds(area.removeFromTop(34));
            area.removeFromTop(6);
        }
    }

private:
    using Id = sensei::core::Id;
    sensei::core::Document* document_ = nullptr;
    juce::OwnedArray<juce::TextButton> buttons_;
};
