#include "SenseiPanel.hpp"

SenseiPanel::SenseiPanel()
{
    title_.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    title_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f5f7));
    fact_.setColour(juce::Label::textColourId, juce::Colour(0xffb7c0cc));
    fact_.setJustificationType(juce::Justification::topLeft);
    advice_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));
    advice_.setJustificationType(juce::Justification::topLeft);
    modeLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff9ca6b5));

    for (auto* b : { &likeBtn_, &doBtn_, &whyBtn_, &laterBtn_, &drumsBtn_, &bassBtn_ })
        addAndMakeVisible(*b);

    addAndMakeVisible(title_);
    addAndMakeVisible(fact_);
    addAndMakeVisible(advice_);
    addAndMakeVisible(modeLabel_);
    bindChoices();
    refresh(true);
}

void SenseiPanel::setDocument(sensei::core::Document* document)
{
    document_ = document;
    refresh(true);
}

void SenseiPanel::bindChoices()
{
    likeBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->handleChoice(sensei::core::UserChoice::LikeIt);
        refresh(true);
        if (onChanged) onChanged();
    };
    doBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->handleChoice(sensei::core::UserChoice::DoSomething);
        refresh(true);
        if (onChanged) onChanged();
    };
    whyBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->handleChoice(sensei::core::UserChoice::Why);
        advice_.setText(document_->lesson().lastWhy, juce::dontSendNotification);
        if (onChanged) onChanged();
    };
    laterBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->handleChoice(sensei::core::UserChoice::Later);
        refresh(true);
        if (onChanged) onChanged();
    };
    drumsBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->applyStarterDrums("basic-rock");
        refresh(true);
        if (onChanged) onChanged();
    };
    bassBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->applyRootBass();
        refresh(true);
        if (onChanged) onChanged();
    };
}

void SenseiPanel::refresh(bool force)
{
    juce::ignoreUnused(force);
    if (document_ == nullptr)
    {
        title_.setText("Sensei", juce::dontSendNotification);
        fact_.setText("Producer mentor", juce::dontSendNotification);
        return;
    }

    const auto obs = document_->analyze();
    title_.setText(obs.title, juce::dontSendNotification);
    fact_.setText(obs.fact, juce::dontSendNotification);
    advice_.setText(obs.advice.empty() ? "Your choice is final." : obs.advice,
                    juce::dontSendNotification);

    const auto& lesson = document_->lesson();
    modeLabel_.setText(lesson.quiet ? "Create mode · quiet" : "Guided first loop",
                       juce::dontSendNotification);

    drumsBtn_.setVisible(lesson.chordsAccepted && ! lesson.drumsAccepted && ! lesson.quiet);
    bassBtn_.setVisible(lesson.drumsAccepted && ! lesson.bassAccepted && ! lesson.quiet
                        && ! document_->project().harmony().chords.empty());
    resized();
}

void SenseiPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff171a20));
    g.setColour(juce::Colour(0xff2a303a));
    g.drawLine(0.0f, 0.0f, 0.0f, (float) getHeight(), 1.0f);
    g.setColour(juce::Colour(0xffd5ff5c));
    g.fillEllipse(16.0f, 16.0f, 28.0f, 28.0f);
}

void SenseiPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    modeLabel_.setBounds(area.removeFromTop(22).withTrimmedLeft(40));
    area.removeFromTop(36);
    title_.setBounds(area.removeFromTop(28));
    area.removeFromTop(4);
    fact_.setBounds(area.removeFromTop(54));
    area.removeFromTop(4);
    advice_.setBounds(area.removeFromTop(54));
    area.removeFromTop(8);

    auto row = area.removeFromTop(28);
    likeBtn_.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
    doBtn_.setBounds(row.reduced(2));
    area.removeFromTop(6);
    row = area.removeFromTop(28);
    whyBtn_.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
    laterBtn_.setBounds(row.reduced(2));
    area.removeFromTop(10);
    if (drumsBtn_.isVisible())
    {
        drumsBtn_.setBounds(area.removeFromTop(30));
        area.removeFromTop(6);
    }
    if (bassBtn_.isVisible())
        bassBtn_.setBounds(area.removeFromTop(30));
}
