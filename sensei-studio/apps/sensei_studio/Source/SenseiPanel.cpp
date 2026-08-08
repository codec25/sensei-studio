#include "SenseiPanel.hpp"

SenseiPanel::SenseiPanel()
{
    title_.setText("Sensei", juce::dontSendNotification);
    title_.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    title_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f5f7));

    body_.setText("Producer mentor placeholder.\n\n"
                  "Milestone A has no AI and no mentoring logic yet.\n"
                  "Create mode will stay quiet until Core lessons arrive.",
                  juce::dontSendNotification);
    body_.setColour(juce::Label::textColourId, juce::Colour(0xffb7c0cc));
    body_.setJustificationType(juce::Justification::topLeft);

    addAndMakeVisible(title_);
    addAndMakeVisible(body_);
}

void SenseiPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff171a20));
    g.setColour(juce::Colour(0xff2a303a));
    g.drawLine(0.0f, 0.0f, 0.0f, static_cast<float>(getHeight()), 1.0f);

    auto orb = juce::Rectangle<float>(16.0f, 16.0f, 28.0f, 28.0f);
    g.setColour(juce::Colour(0xffd5ff5c));
    g.fillEllipse(orb);
}

void SenseiPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(36); // room for painted orb/brand
    title_.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    body_.setBounds(area);
}
