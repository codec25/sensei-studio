#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/commands/TrackContentCommands.hpp"

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
        g.fillAll(juce::Colour(0xff11151a));
        g.setColour(juce::Colour(0xff9ca6b5));
        g.drawText("DRUM GRID · selected clip · click to toggle",
                   getLocalBounds().removeFromTop(20).reduced(6, 0),
                   juce::Justification::centredLeft, false);

        const auto* clip = activeClip();
        if (clip == nullptr)
            return;

        static constexpr const char* names[] { "Kick", "Snare", "Hat" };
        const int steps = clip->pattern.stepCount > 0 ? clip->pattern.stepCount
                                                     : sensei::core::kDefaultDrumSteps;
        const float cellW = (float) juce::jmax(1, (getWidth() - 56) / juce::jmin(steps, 64));
        const float cellH = 28.0f;

        for (int lane = 0; lane < sensei::core::kDrumLaneCount; ++lane)
        {
            const float y = 28.0f + lane * (cellH + 6.0f);
            g.setColour(juce::Colour(0xffdfe4eb));
            g.drawText(names[lane], juce::Rectangle<float>(4, y, 48, cellH),
                       juce::Justification::centredLeft, false);

            for (int step = 0; step < steps; ++step)
            {
                const float x = 56.0f + step * cellW;
                const bool on = clip->pattern.hasHit(step, static_cast<sensei::core::DrumLane>(lane));
                const bool beat = (step % 4) == 0;
                g.setColour(on ? juce::Colour(0xffd5ff5c)
                               : (beat ? juce::Colour(0xff2a303a) : juce::Colour(0xff1e222a)));
                g.fillRoundedRectangle(x + 1, y + 2, cellW - 2, cellH - 4, 3.0f);
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
        const float cellW = (float) juce::jmax(1, (getWidth() - 56) / juce::jmin(steps, 64));
        const float cellH = 28.0f;
        if (event.x < 56)
            return;

        const int step = juce::jlimit(0, steps - 1, (int) ((event.x - 56) / cellW));
        const int lane = juce::jlimit(0, sensei::core::kDrumLaneCount - 1,
                                      (int) ((event.y - 28) / (cellH + 6.0f)));
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
