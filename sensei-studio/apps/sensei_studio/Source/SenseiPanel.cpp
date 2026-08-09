#include "SenseiPanel.hpp"

SenseiPanel::SenseiPanel()
{
    title_.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    fact_.setFont(juce::FontOptions(15.0f));
    fact_.setJustificationType(juce::Justification::topLeft);
    advice_.setFont(juce::FontOptions(13.5f));
    advice_.setJustificationType(juce::Justification::topLeft);
    modeLabel_.setFont(juce::FontOptions(12.5f));

    collapseBtn_.setTooltip("Collapse Sensei");
    collapseBtn_.onClick = [this] {
        if (onCollapseToggle)
            onCollapseToggle();
    };
    expandBtn_.setTooltip("Expand Sensei");
    expandBtn_.onClick = [this] {
        if (onCollapseToggle)
            onCollapseToggle();
    };

    for (auto* b : { &likeBtn_, &doBtn_, &whyBtn_, &laterBtn_, &drumsBtn_, &bassBtn_,
                     &songBtn_, &variationBtn_, &introBtn_, &collapseBtn_, &expandBtn_ })
        addAndMakeVisible(*b);

    addAndMakeVisible(title_);
    addAndMakeVisible(fact_);
    addAndMakeVisible(advice_);
    addAndMakeVisible(modeLabel_);
    bindChoices();
    setCollapsed(false);
    refresh(true);
}

void SenseiPanel::setDocument(sensei::core::Document* document)
{
    document_ = document;
    refresh(true);
}

void SenseiPanel::setCollapsed(bool collapsed)
{
    collapsed_ = collapsed;
    updateVisibility();
    resized();
    repaint();
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
    songBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->applySongShape();
        refresh(true);
        if (onChanged) onChanged();
    };
    variationBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->applyVariationThinDrums();
        refresh(true);
        if (onChanged) onChanged();
    };
    introBtn_.onClick = [this] {
        if (document_ == nullptr) return;
        document_->applyIntroContrast();
        refresh(true);
        if (onChanged) onChanged();
    };
}

void SenseiPanel::updateVisibility()
{
    for (auto* c : getChildren())
        c->setVisible(! collapsed_);
    expandBtn_.setVisible(collapsed_);
    collapseBtn_.setVisible(! collapsed_);
}

void SenseiPanel::refresh(bool force)
{
    juce::ignoreUnused(force);
    const auto& p = studioPalette();
    title_.setColour(juce::Label::textColourId, p.textPrimary);
    fact_.setColour(juce::Label::textColourId, p.textSecondary);
    advice_.setColour(juce::Label::textColourId, p.textMuted);
    modeLabel_.setColour(juce::Label::textColourId, p.textMuted);

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
    modeLabel_.setText(lesson.quiet ? "Create mode · quiet" : "Guided song shape",
                       juce::dontSendNotification);

    updateVisibility();
    if (! collapsed_)
    {
        drumsBtn_.setVisible(lesson.chordsAccepted && ! lesson.drumsAccepted && ! lesson.quiet);
        bassBtn_.setVisible(lesson.drumsAccepted && ! lesson.bassAccepted && ! lesson.quiet
                            && ! document_->project().harmony().chords.empty());
        songBtn_.setVisible(lesson.celebratedCompleteLoop && ! lesson.songShapeAccepted && ! lesson.quiet);
        variationBtn_.setVisible(lesson.songShapeAccepted && ! lesson.variationAccepted && ! lesson.quiet);
        introBtn_.setVisible(lesson.songShapeAccepted && ! lesson.quiet);
    }
    resized();
}

void SenseiPanel::paint(juce::Graphics& g)
{
    const auto& p = studioPalette();
    g.fillAll(p.bg1);
    g.setColour(p.borderSoft.withAlpha(0.65f));
    g.drawLine(0.5f, 0.0f, 0.5f, (float) getHeight());

    if (collapsed_)
    {
        drawSenseiOrb(g, { 8.0f, 40.0f, 28.0f, 28.0f }, p, 0.7f);
        g.setColour(p.textMuted);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("S", getLocalBounds().withTrimmedTop(78), juce::Justification::centredTop, false);
        return;
    }

    juce::ColourGradient wash(p.accentSoft, 0.0f, 0.0f, juce::Colours::transparentBlack, 0.0f, 160.0f, false);
    g.setGradientFill(wash);
    g.fillRect(0, 0, getWidth(), 160);

    drawSenseiOrb(g, { 16.0f, 16.0f, 34.0f, 34.0f }, p, 0.9f);

    g.setColour(p.panelSoft.withAlpha(0.65f));
    g.fillRoundedRectangle(12.0f, 64.0f, (float) getWidth() - 24.0f, 150.0f, 12.0f);
}

void SenseiPanel::resized()
{
    if (collapsed_)
    {
        expandBtn_.setBounds(getLocalBounds().reduced(4).removeFromTop(28));
        return;
    }

    auto area = getLocalBounds().reduced(16);
    auto top = area.removeFromTop(34);
    top.removeFromLeft(44);
    modeLabel_.setBounds(top.removeFromLeft(top.getWidth() - 36));
    collapseBtn_.setBounds(top);

    area.removeFromTop(36);
    title_.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);
    fact_.setBounds(area.removeFromTop(48));
    area.removeFromTop(4);
    advice_.setBounds(area.removeFromTop(48));
    area.removeFromTop(12);

    auto row = area.removeFromTop(34);
    likeBtn_.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(3));
    doBtn_.setBounds(row.reduced(3));
    area.removeFromTop(8);
    row = area.removeFromTop(34);
    whyBtn_.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(3));
    laterBtn_.setBounds(row.reduced(3));
    area.removeFromTop(14);

    auto place = [&](juce::TextButton& b) {
        if (b.isVisible())
        {
            b.setBounds(area.removeFromTop(32));
            area.removeFromTop(6);
        }
    };
    place(drumsBtn_);
    place(bassBtn_);
    place(songBtn_);
    place(introBtn_);
    place(variationBtn_);
}
