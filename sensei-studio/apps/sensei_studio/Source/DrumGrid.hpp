#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/commands/TrackContentCommands.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class DrumGrid final : public juce::Component
{
public:
    std::function<void()> onEdited;

    void setDocument(sensei::core::Document* document)
    {
        document_ = document;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        g.fillAll(p.bg0);
        g.setColour(p.textMuted);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("Drum grid · selected clip · click to toggle",
                   getLocalBounds().removeFromTop(22).reduced(6, 0),
                   juce::Justification::centredLeft, false);

        const auto* clip = activeClip();
        if (clip == nullptr)
            return;

        static constexpr const char* names[] { "Kick", "Snare", "Hat" };
        const int steps = clip->pattern.stepCount > 0 ? clip->pattern.stepCount
                                                     : sensei::core::kDefaultDrumSteps;
        const float cellW = (float) juce::jmax(1, (getWidth() - 64) / juce::jmin(steps, 64));
        const float cellH = 32.0f;

        for (int lane = 0; lane < sensei::core::kDrumLaneCount; ++lane)
        {
            const float y = 30.0f + lane * (cellH + 8.0f);
            g.setColour(p.textSecondary);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText(names[lane], juce::Rectangle<float>(4, y, 56, cellH),
                       juce::Justification::centredLeft, false);

            for (int step = 0; step < steps; ++step)
            {
                const float x = 64.0f + step * cellW;
                const bool on = clip->pattern.hasHit(step, static_cast<sensei::core::DrumLane>(lane));
                const bool beat = (step % 4) == 0;
                g.setColour(on ? p.roleDrums : (beat ? p.panelSoft : p.bg2));
                g.fillRoundedRectangle(x + 1, y + 3, cellW - 2, cellH - 6, 4.0f);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (document_ == nullptr)
            return;
        auto* track = document_->project().findTrackByRole(sensei::core::TrackRole::Drums);
        auto* clip = activeClip();
        if (track == nullptr || clip == nullptr)
            return;

        const int steps = clip->pattern.stepCount > 0 ? clip->pattern.stepCount
                                                     : sensei::core::kDefaultDrumSteps;
        const float cellW = (float) juce::jmax(1, (getWidth() - 64) / juce::jmin(steps, 64));
        const float cellH = 32.0f;
        if (event.x < 64)
            return;

        const int step = juce::jlimit(0, steps - 1, (int) ((event.x - 64) / cellW));
        const int lane = juce::jlimit(0, sensei::core::kDrumLaneCount - 1,
                                      (int) ((event.y - 30) / (cellH + 8.0f)));
        document_->setSelectedClipId(track->id, clip->id);
        document_->execute(std::make_unique<sensei::core::ToggleDrumHitCommand>(
            track->id, clip->id, step, static_cast<sensei::core::DrumLane>(lane), 0.85f));
        repaint();
        if (onEdited)
            onEdited();
    }

private:
    [[nodiscard]] sensei::core::DrumClip* activeClip() noexcept
    {
        if (document_ == nullptr)
            return nullptr;
        auto* track = document_->project().findTrackByRole(sensei::core::TrackRole::Drums);
        if (track == nullptr || track->drumClips.empty())
            return nullptr;
        if (auto* clip = document_->project().findDrumClip(track->id, document_->selectedClipId()))
            return clip;
        return &track->drumClips.front();
    }

    [[nodiscard]] const sensei::core::DrumClip* activeClip() const noexcept
    {
        if (document_ == nullptr)
            return nullptr;
        const auto* track = document_->project().findTrackByRole(sensei::core::TrackRole::Drums);
        if (track == nullptr || track->drumClips.empty())
            return nullptr;
        if (const auto* clip = document_->project().findDrumClip(track->id, document_->selectedClipId()))
            return clip;
        return &track->drumClips.front();
    }

    sensei::core::Document* document_ = nullptr;
};
