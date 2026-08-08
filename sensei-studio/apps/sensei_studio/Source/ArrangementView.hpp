#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/commands/ArrangementCommands.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <functional>

// Simple horizontal arrangement timeline — not an Ableton/Logic clone.
class ArrangementView final : public juce::Component
{
public:
    std::function<void()> onEdited;
    std::function<void()> onSelectionChanged;

    void setDocument(sensei::core::Document* document)
    {
        document_ = document;
        repaint();
    }

    void setPlayheadBeats(double beats)
    {
        playheadBeats_ = beats;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff11151a));
        if (document_ == nullptr)
            return;

        const auto& project = document_->project();
        const double songLen = juce::jmax(project.songLengthBeats(), sensei::core::kDefaultLoopBeats);
        const int bars = juce::jmax(1, (int) std::ceil(songLen / sensei::core::kBeatsPerBar));
        const float laneH = 36.0f;
        const float headerH = 28.0f;
        const float labelW = 72.0f;
        const float contentW = juce::jmax(1.0f, (float) getWidth() - labelW - 8.0f);
        const float beatW = contentW / (float) songLen;

        // Section band
        for (const auto& section : project.sections())
        {
            const float x = labelW + (float) section.startBeat * beatW;
            const float w = (float) section.lengthBeats * beatW;
            g.setColour(juce::Colour(0xff1c2430));
            g.fillRect(x, 0.0f, w, headerH);
            g.setColour(juce::Colour(0xffd5ff5c));
            g.drawRect(x, 0.0f, w, headerH, 1.0f);
            g.setColour(juce::Colour(0xfff4f5f7));
            g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
            g.drawText(section.name, juce::Rectangle<float>(x + 4, 2, w - 8, headerH - 4),
                       juce::Justification::centredLeft, true);
        }

        // Bar numbers
        g.setFont(juce::FontOptions(10.0f));
        g.setColour(juce::Colour(0xff6f7a88));
        for (int bar = 0; bar < bars; ++bar)
        {
            const float x = labelW + (float) (bar * sensei::core::kBeatsPerBar) * beatW;
            g.drawVerticalLine((int) x, headerH, (float) getHeight());
            g.drawText(juce::String(bar + 1), juce::Rectangle<float>(x + 2, headerH - 14, 24, 12),
                       juce::Justification::centredLeft, false);
        }

        static constexpr const char* roles[] { "Chords", "Bass", "Drums", "Melody" };
        static constexpr sensei::core::TrackRole roleIds[] {
            sensei::core::TrackRole::Chords, sensei::core::TrackRole::Bass,
            sensei::core::TrackRole::Drums, sensei::core::TrackRole::Melody
        };
        static constexpr juce::uint32 colours[] { 0xff6ea8fe, 0xfff0a35e, 0xffd5ff5c, 0xffc792ea };

        for (int i = 0; i < 4; ++i)
        {
            const float y = headerH + 4.0f + i * (laneH + 6.0f);
            g.setColour(juce::Colour(0xffdfe4eb));
            g.drawText(roles[i], juce::Rectangle<float>(4, y, labelW - 8, laneH),
                       juce::Justification::centredLeft, false);
            g.setColour(juce::Colour(0xff1a1f27));
            g.fillRect(labelW, y, contentW, laneH);

            const auto* track = project.findTrackByRole(roleIds[i]);
            if (track == nullptr)
                continue;

            auto drawClip = [&](sensei::core::Id trackId, sensei::core::Id clipId,
                                double start, double length, const juce::String& name) {
                const float x = labelW + (float) start * beatW;
                const float w = juce::jmax(4.0f, (float) length * beatW - 2.0f);
                const bool selected = document_->selectedTrackId() == trackId
                                      && document_->selectedClipId() == clipId;
                g.setColour(juce::Colour(colours[i]).withAlpha(selected ? 0.95f : 0.7f));
                g.fillRoundedRectangle(x + 1, y + 4, w, laneH - 8, 4.0f);
                if (selected)
                {
                    g.setColour(juce::Colours::white);
                    g.drawRoundedRectangle(x + 1, y + 4, w, laneH - 8, 4.0f, 1.5f);
                }
                g.setColour(juce::Colour(0xff0f1115));
                g.setFont(juce::FontOptions(11.0f));
                g.drawText(name, juce::Rectangle<float>(x + 6, y + 4, w - 10, laneH - 8),
                           juce::Justification::centredLeft, true);
            };

            if (track->type == sensei::core::TrackType::Drums)
            {
                for (const auto& clip : track->drumClips)
                    drawClip(track->id, clip.id, clip.startBeat, clip.lengthBeats, clip.name);
            }
            else
            {
                for (const auto& clip : track->clips)
                    drawClip(track->id, clip.id, clip.startBeat, clip.lengthBeats, clip.name);
            }
        }

        // Playhead
        const float px = labelW + (float) playheadBeats_ * beatW;
        g.setColour(juce::Colour(0xffff6b6b));
        g.drawLine(px, 0.0f, px, (float) getHeight(), 1.5f);

        g.setColour(juce::Colour(0xff9ca6b5));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("ARRANGEMENT · click select · drag move · D duplicate · Delete · double-click edit",
                   getLocalBounds().removeFromBottom(18).reduced(6, 0),
                   juce::Justification::centredLeft, false);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (document_ == nullptr)
            return;
        hit_ = hitTestClip(event.position);
        dragOriginBeat_ = hit_.startBeat;
        dragGrabOffset_ = 0.0;
        if (hit_.valid)
        {
            document_->setSelectedClipId(hit_.trackId, hit_.clipId);
            const double songLen = juce::jmax(document_->project().songLengthBeats(), 1.0);
            const float labelW = 72.0f;
            const float beatW = (float) (getWidth() - 80) / (float) songLen;
            dragGrabOffset_ = beatForX(event.position.x, labelW, beatW) - hit_.startBeat;
            if (onSelectionChanged)
                onSelectionChanged();
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (document_ == nullptr || ! hit_.valid)
            return;
        const double songLen = juce::jmax(document_->project().songLengthBeats(), 1.0);
        const float labelW = 72.0f;
        const float beatW = (float) (getWidth() - 80) / (float) songLen;
        const double beat = sensei::core::snapBeat(
            juce::jmax(0.0, beatForX(event.position.x, labelW, beatW) - dragGrabOffset_));
        previewStart_ = beat;
        dragging_ = true;
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (document_ == nullptr || ! hit_.valid || ! dragging_)
        {
            dragging_ = false;
            return;
        }
        if (std::abs(previewStart_ - dragOriginBeat_) > 1.0e-6)
        {
            document_->execute(std::make_unique<sensei::core::MoveClipCommand>(
                hit_.trackId, hit_.clipId, previewStart_));
            if (onEdited)
                onEdited();
        }
        dragging_ = false;
        hit_ = {};
        repaint();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (document_ == nullptr)
            return false;
        const auto trackId = document_->selectedTrackId();
        const auto clipId = document_->selectedClipId();
        if (trackId == sensei::core::kInvalidId || clipId == sensei::core::kInvalidId)
            return false;

        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            document_->execute(std::make_unique<sensei::core::DeleteClipCommand>(trackId, clipId));
            if (onEdited)
                onEdited();
            return true;
        }
        if (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D')
        {
            double len = sensei::core::kDefaultLoopBeats;
            double start = 0.0;
            if (const auto* midi = document_->project().findClip(trackId, clipId))
            {
                len = midi->lengthBeats;
                start = midi->startBeat + len;
            }
            else if (const auto* drum = document_->project().findDrumClip(trackId, clipId))
            {
                len = drum->lengthBeats;
                start = drum->startBeat + len;
            }
            document_->execute(std::make_unique<sensei::core::DuplicateClipCommand>(trackId, clipId, start));
            if (onEdited)
                onEdited();
            return true;
        }
        return false;
    }

private:
    struct Hit
    {
        bool valid = false;
        sensei::core::Id trackId = sensei::core::kInvalidId;
        sensei::core::Id clipId = sensei::core::kInvalidId;
        double startBeat = 0.0;
    };

    static double beatForX(float x, float labelW, float beatW) noexcept
    {
        return juce::jmax(0.0, (double) ((x - labelW) / beatW));
    }

    Hit hitTestClip(juce::Point<float> pos) const
    {
        Hit hit;
        if (document_ == nullptr)
            return hit;
        const auto& project = document_->project();
        const double songLen = juce::jmax(project.songLengthBeats(), sensei::core::kDefaultLoopBeats);
        const float laneH = 36.0f;
        const float headerH = 28.0f;
        const float labelW = 72.0f;
        const float beatW = (float) (getWidth() - 80) / (float) songLen;

        static constexpr sensei::core::TrackRole roleIds[] {
            sensei::core::TrackRole::Chords, sensei::core::TrackRole::Bass,
            sensei::core::TrackRole::Drums, sensei::core::TrackRole::Melody
        };

        for (int i = 0; i < 4; ++i)
        {
            const float y = headerH + 4.0f + i * (laneH + 6.0f);
            if (pos.y < y || pos.y > y + laneH)
                continue;
            const auto* track = project.findTrackByRole(roleIds[i]);
            if (track == nullptr)
                continue;

            auto consider = [&](sensei::core::Id clipId, double start, double length) {
                const float x = labelW + (float) start * beatW;
                const float w = juce::jmax(4.0f, (float) length * beatW);
                if (pos.x >= x && pos.x <= x + w)
                {
                    hit.valid = true;
                    hit.trackId = track->id;
                    hit.clipId = clipId;
                    hit.startBeat = start;
                }
            };

            if (track->type == sensei::core::TrackType::Drums)
            {
                for (const auto& clip : track->drumClips)
                    consider(clip.id, clip.startBeat, clip.lengthBeats);
            }
            else
            {
                for (const auto& clip : track->clips)
                    consider(clip.id, clip.startBeat, clip.lengthBeats);
            }
        }
        return hit;
    }

    sensei::core::Document* document_ = nullptr;
    double playheadBeats_ = 0.0;
    Hit hit_ {};
    bool dragging_ = false;
    double dragOriginBeat_ = 0.0;
    double dragGrabOffset_ = 0.0;
    double previewStart_ = 0.0;
};
