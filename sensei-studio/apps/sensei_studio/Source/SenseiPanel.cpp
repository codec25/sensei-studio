#include "SenseiPanel.hpp"

SenseiPanel::SenseiPanel()
{
    title_.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    title_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f5f7));

    fact_.setColour(juce::Label::textColourId, juce::Colour(0xffb7c0cc));
    fact_.setJustificationType(juce::Justification::topLeft);

    advice_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    advice_.setJustificationType(juce::Justification::topLeft);

    modeLabel_.setText("Create mode · quiet", juce::dontSendNotification);
    modeLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));

    addAndMakeVisible(title_);
    addAndMakeVisible(fact_);
    addAndMakeVisible(advice_);
    addAndMakeVisible(modeLabel_);

    setObservation({ sensei::core::ObservationKind::NoNotes,
                     "No notes yet",
                     "The MIDI clip is empty.",
                     "Click in the piano roll to place your first note." },
                   true);
}

void SenseiPanel::setObservation(const sensei::core::Observation& observation, bool forceShow)
{
    // Create mode stays quiet: show NoNotes / FirstIdea / NotesOutsideLoop,
    // or any observation when forceShow is true (panel refresh / high-signal).
    const bool highSignal = observation.kind == sensei::core::ObservationKind::NoNotes
                            || observation.kind == sensei::core::ObservationKind::FirstIdea
                            || observation.kind == sensei::core::ObservationKind::NotesOutsideLoop;

    if (! forceShow && ! highSignal)
    {
        if (! visibleObservation_)
            return;
        // Keep last high-signal card rather than spamming low-priority updates.
        return;
    }

    visibleObservation_ = true;
    title_.setText(observation.title, juce::dontSendNotification);
    fact_.setText(observation.fact, juce::dontSendNotification);
    advice_.setText(observation.advice.empty() ? "Observation only — your choice is final."
                                               : observation.advice,
                    juce::dontSendNotification);
    repaint();
}

void SenseiPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff171a20));
    g.setColour(juce::Colour(0xff2a303a));
    g.drawLine(0.0f, 0.0f, 0.0f, static_cast<float>(getHeight()), 1.0f);

    g.setColour(juce::Colour(0xffd5ff5c));
    g.fillEllipse(16.0f, 16.0f, 28.0f, 28.0f);

    g.setColour(juce::Colour(0xff11151a));
    g.fillRoundedRectangle(12.0f, 56.0f, static_cast<float>(getWidth() - 24), 160.0f, 12.0f);
    g.setColour(juce::Colour(0x73d5ff5c));
    g.drawRoundedRectangle(12.0f, 56.0f, static_cast<float>(getWidth() - 24), 160.0f, 12.0f, 1.0f);
}

void SenseiPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    modeLabel_.setBounds(area.removeFromTop(24).withTrimmedLeft(40));
    area.removeFromTop(40);
    title_.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);
    fact_.setBounds(area.removeFromTop(56));
    area.removeFromTop(6);
    advice_.setBounds(area.removeFromTop(56));
}
