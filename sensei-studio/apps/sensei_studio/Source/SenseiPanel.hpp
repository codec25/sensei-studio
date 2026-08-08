#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/sensei/Observation.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class SenseiPanel final : public juce::Component
{
public:
    std::function<void()> onChanged;

    SenseiPanel();

    void setDocument(sensei::core::Document* document);
    void refresh(bool force);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void bindChoices();

    sensei::core::Document* document_ = nullptr;
    juce::Label title_;
    juce::Label fact_;
    juce::Label advice_;
    juce::Label modeLabel_;
    juce::TextButton likeBtn_ { "I like it like this" };
    juce::TextButton doBtn_ { "Let’s do something" };
    juce::TextButton whyBtn_ { "Why?" };
    juce::TextButton laterBtn_ { "Later" };
    juce::TextButton drumsBtn_ { "Add starter drums" };
    juce::TextButton bassBtn_ { "Add root-note bass" };
    juce::TextButton songBtn_ { "Turn this loop into a song" };
    juce::TextButton variationBtn_ { "Thin variation drums" };
    juce::TextButton introBtn_ { "Thin the intro" };
};
