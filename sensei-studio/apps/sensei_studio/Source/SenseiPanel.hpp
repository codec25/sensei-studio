#pragma once

#include "sensei/core/sensei/Observation.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

// Deterministic Sensei observations only. No AI / networking.
class SenseiPanel final : public juce::Component
{
public:
    SenseiPanel();

    void setObservation(const sensei::core::Observation& observation, bool forceShow);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title_;
    juce::Label fact_;
    juce::Label advice_;
    juce::Label modeLabel_;
    bool visibleObservation_ = true;
};
