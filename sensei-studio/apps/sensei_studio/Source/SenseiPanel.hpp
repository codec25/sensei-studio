#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Static placeholder only. No AI / mentoring logic in Milestone A.
class SenseiPanel final : public juce::Component
{
public:
    SenseiPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title_;
    juce::Label body_;
};
