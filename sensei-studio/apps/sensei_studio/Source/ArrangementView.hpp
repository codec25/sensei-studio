#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/commands/ArrangementCommands.hpp"
#include "sensei/core/commands/TrackMuteCommands.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <functional>

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
        const auto& p = studioPalette();
        g.fillAll(p.bg0);
        if (document_ == nullptr)
        {
            drawEmpty(g, p);
            return;
        }

        const auto& project = document_->project();
        const double songLen = juce::jmax(project.songLengthBeats(), sensei::core::kDefaultLoopBeats);
        const int bars = juce::jmax(1, (int) std::ceil(songLen / sensei::core::kBeatsPerBar));
        const float headerH = 36.0f;
        const float labelW = kLabelW;
        const float contentW = juce::jmax(1.0f, (float) getWidth() - labelW - 4.0f);
        const float beatW = contentW / (float) songLen;
        const float laneH = laneHeight();

        // Section band
        for (const auto& section : project.sections())
        {
            const float x = labelW + (float) section.startBeat * beatW;
            const float w = (float) section.lengthBeats * beatW;
            g.setColour(p.panelSoft.withAlpha(0.55f));
            g.fillRoundedRectangle(x + 1.0f, 4.0f, juce::jmax(2.0f, w - 2.0f), headerH - 8.0f, 6.0f);
            g.setColour(p.accent.withAlpha(0.35f));
            g.fillRect(x + 1.0f, 4.0f, 3.0f, headerH - 8.0f);
            g.setColour(p.textPrimary);
            g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
            g.drawText(section.name, juce::Rectangle<float>(x + 10, 4, w - 14, headerH - 8),
                       juce::Justification::centredLeft, true);
        }

        // Ruler / bar numbers + soft grid
        g.setFont(juce::FontOptions(12.0f));
        for (int bar = 0; bar < bars; ++bar)
        {
            const float x = labelW + (float) (bar * sensei::core::kBeatsPerBar) * beatW;
            g.setColour(p.gridMajor.withAlpha(0.55f));
            g.drawVerticalLine((int) x, headerH, (float) getHeight() - 4.0f);
            g.setColour(p.textMuted);
            g.drawText(juce::String(bar + 1), juce::Rectangle<float>(x + 4, headerH - 16, 28, 14),
                       juce::Justification::centredLeft, false);

            for (int b = 1; b < (int) sensei::core::kBeatsPerBar; ++b)
            {
                const float bx = x + (float) b * beatW;
                g.setColour(p.gridMinor.withAlpha(0.4f));
                g.drawVerticalLine((int) bx, headerH + 2.0f, (float) getHeight() - 4.0f);
            }
        }

        static constexpr sensei::core::TrackRole roleIds[] {
            sensei::core::TrackRole::Chords, sensei::core::TrackRole::Bass,
            sensei::core::TrackRole::Drums, sensei::core::TrackRole::Melody
        };

        const bool anySolo = sensei::core::projectHasSolo(project.tracks());

        for (int i = 0; i < 4; ++i)
        {
            const float y = headerH + 6.0f + i * (laneH + kLaneGap);
            const auto* track = project.findTrackByRole(roleIds[i]);
            const auto roleColour = colourForRole(roleIds[i], p);
            const bool selectedTrack = track != nullptr
                                       && document_->selectedTrackId() == track->id;

            // Lane background
            g.setColour(selectedTrack ? p.bg2.brighter(0.04f) : p.bg2);
            g.fillRoundedRectangle(4.0f, y, (float) getWidth() - 8.0f, laneH, 8.0f);
            g.setColour(roleColour);
            g.fillRoundedRectangle(4.0f, y + 6.0f, 4.0f, laneH - 12.0f, 2.0f);

            // Header text
            if (track != nullptr)
            {
                const auto info = sensei::core::instrumentInfo(track->instrumentId);
                g.setColour(p.textPrimary);
                g.setFont(juce::FontOptions(13.5f).withStyle("Bold"));
                g.drawText(track->name, juce::Rectangle<float>(14, y + 4, labelW - 70, 18),
                           juce::Justification::centredLeft, true);
                g.setColour(p.textMuted);
                g.setFont(juce::FontOptions(11.5f));
                g.drawText(roleName(roleIds[i]) + " · " + info.displayName,
                           juce::Rectangle<float>(14, y + 22, labelW - 70, 16),
                           juce::Justification::centredLeft, true);

                drawMuteSolo(g, p, muteBounds(i, y, laneH), track->muted, false);
                drawMuteSolo(g, p, soloBounds(i, y, laneH), track->solo, true);

                if (! sensei::core::isTrackAudible(*track, anySolo))
                {
                    g.setColour(p.bg0.withAlpha(0.35f));
                    g.fillRoundedRectangle(labelW, y + 2, contentW, laneH - 4, 6.0f);
                }
            }

            // Timeline lane
            g.setColour(p.bg0.withAlpha(0.35f));
            g.fillRect(labelW, y + 4, contentW, laneH - 8);

            if (track == nullptr)
                continue;

            auto drawClip = [&](sensei::core::Id trackId, sensei::core::Id clipId,
                                double start, double length, const juce::String& name,
                                bool drums) {
                double drawStart = start;
                if (dragging_ && hit_.valid && hit_.clipId == clipId && hit_.trackId == trackId)
                    drawStart = previewStart_;

                const float x = labelW + (float) drawStart * beatW;
                const float w = juce::jmax(8.0f, (float) length * beatW - 3.0f);
                const bool selected = document_->selectedTrackId() == trackId
                                      && document_->selectedClipId() == clipId;
                auto fill = roleColour.withAlpha(selected ? 0.92f : 0.72f);
                g.setColour(fill);
                g.fillRoundedRectangle(x + 1, y + 8, w, laneH - 16, 6.0f);
                if (selected)
                {
                    g.setColour(p.selectedOutline.withAlpha(0.9f));
                    g.drawRoundedRectangle(x + 1, y + 8, w, laneH - 16, 6.0f, 1.8f);
                }
                g.setColour(p.bg0.withAlpha(0.25f));
                g.fillRect(x + 1, y + 8, 3.0f, laneH - 16);

                g.setColour(p.clipText);
                g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
                g.drawText(name, juce::Rectangle<float>(x + 8, y + 9, w - 14, 16),
                           juce::Justification::centredLeft, true);
                drawClipPreview(g, p, track, clipId, drums,
                                juce::Rectangle<float>(x + 8, y + 26, w - 14, laneH - 36));
            };

            if (track->type == sensei::core::TrackType::Drums)
            {
                for (const auto& clip : track->drumClips)
                    drawClip(track->id, clip.id, clip.startBeat, clip.lengthBeats, clip.name, true);
            }
            else
            {
                for (const auto& clip : track->clips)
                    drawClip(track->id, clip.id, clip.startBeat, clip.lengthBeats, clip.name, false);
            }
        }

        // Loop region tint
        if (project.loop().enabled)
        {
            const float x = labelW + (float) project.loop().startBeat * beatW;
            const float w = (float) project.loop().lengthBeats * beatW;
            g.setColour(p.accent.withAlpha(0.06f));
            g.fillRect(x, headerH, w, (float) getHeight() - headerH);
            g.setColour(p.accent.withAlpha(0.35f));
            g.drawVerticalLine((int) x, headerH, (float) getHeight());
            g.drawVerticalLine((int) (x + w), headerH, (float) getHeight());
        }

        // Playhead
        const float px = labelW + (float) playheadBeats_ * beatW;
        g.setColour(p.playhead);
        g.drawLine(px, 2.0f, px, (float) getHeight() - 2.0f, 2.0f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (document_ == nullptr)
            return;

        const float headerH = 36.0f;
        const float laneH = laneHeight();
        for (int i = 0; i < 4; ++i)
        {
            const float y = headerH + 6.0f + i * (laneH + kLaneGap);
            auto* track = trackAt(i);
            if (track == nullptr)
                continue;
            if (muteBounds(i, y, laneH).contains(event.position))
            {
                document_->execute(std::make_unique<sensei::core::SetTrackMuteCommand>(
                    track->id, ! track->muted));
                if (onEdited)
                    onEdited();
                return;
            }
            if (soloBounds(i, y, laneH).contains(event.position))
            {
                document_->execute(std::make_unique<sensei::core::SetTrackSoloCommand>(
                    track->id, ! track->solo));
                if (onEdited)
                    onEdited();
                return;
            }
            if (event.position.x < kLabelW && event.position.y >= y && event.position.y <= y + laneH)
            {
                document_->setSelectedTrackId(track->id);
                if (onSelectionChanged)
                    onSelectionChanged();
                repaint();
                return;
            }
        }

        hit_ = hitTestClip(event.position);
        dragOriginBeat_ = hit_.startBeat;
        dragGrabOffset_ = 0.0;
        if (hit_.valid)
        {
            document_->setSelectedClipId(hit_.trackId, hit_.clipId);
            const double songLen = juce::jmax(document_->project().songLengthBeats(), 1.0);
            const float beatW = ((float) getWidth() - kLabelW - 4.0f) / (float) songLen;
            dragGrabOffset_ = beatForX(event.position.x, kLabelW, beatW) - hit_.startBeat;
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
        const float beatW = ((float) getWidth() - kLabelW - 4.0f) / (float) songLen;
        const double beat = sensei::core::snapBeat(
            juce::jmax(0.0, beatForX(event.position.x, kLabelW, beatW) - dragGrabOffset_));
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

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        mouseDown(event);
        // Editor dock already shows selected track — double-click just focuses selection.
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
    static constexpr float kLabelW = 168.0f;
    static constexpr float kLaneGap = 8.0f;

    struct Hit
    {
        bool valid = false;
        sensei::core::Id trackId = sensei::core::kInvalidId;
        sensei::core::Id clipId = sensei::core::kInvalidId;
        double startBeat = 0.0;
    };

    float laneHeight() const noexcept
    {
        return juce::jmax(52.0f, ((float) getHeight() - 48.0f) / 4.0f - kLaneGap);
    }

    static juce::String roleName(sensei::core::TrackRole role)
    {
        switch (role)
        {
            case sensei::core::TrackRole::Chords: return "Chords";
            case sensei::core::TrackRole::Bass: return "Bass";
            case sensei::core::TrackRole::Drums: return "Drums";
            case sensei::core::TrackRole::Melody: return "Melody";
        }
        return {};
    }

    static juce::Colour colourForRole(sensei::core::TrackRole role, const StudioPalette& p)
    {
        switch (role)
        {
            case sensei::core::TrackRole::Chords: return p.roleChords;
            case sensei::core::TrackRole::Bass: return p.roleBass;
            case sensei::core::TrackRole::Drums: return p.roleDrums;
            case sensei::core::TrackRole::Melody: return p.roleMelody;
        }
        return p.accent;
    }

    static void drawEmpty(juce::Graphics& g, const StudioPalette& p)
    {
        drawSenseiOrb(g, { (float) g.getClipBounds().getCentreX() - 18.0f, 40.0f, 36.0f, 36.0f }, p, 0.8f);
        g.setColour(p.textSecondary);
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("Your arrangement appears here", g.getClipBounds().withTrimmedTop(90),
                   juce::Justification::centredTop, false);
    }

    static void drawMuteSolo(juce::Graphics& g, const StudioPalette& p, juce::Rectangle<float> r,
                             bool on, bool isSolo)
    {
        g.setColour(on ? (isSolo ? p.accent.withAlpha(0.9f) : p.danger.withAlpha(0.85f))
                       : p.panelSoft);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(on ? p.clipText : p.textMuted);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(isSolo ? "S" : "M", r, juce::Justification::centred, false);
    }

    juce::Rectangle<float> muteBounds(int index, float y, float laneH) const
    {
        juce::ignoreUnused(index);
        return { kLabelW - 56.0f, y + laneH * 0.5f - 11.0f, 22.0f, 22.0f };
    }

    juce::Rectangle<float> soloBounds(int index, float y, float laneH) const
    {
        juce::ignoreUnused(index);
        return { kLabelW - 30.0f, y + laneH * 0.5f - 11.0f, 22.0f, 22.0f };
    }

    sensei::core::Track* trackAt(int index) const
    {
        if (document_ == nullptr)
            return nullptr;
        static constexpr sensei::core::TrackRole roleIds[] {
            sensei::core::TrackRole::Chords, sensei::core::TrackRole::Bass,
            sensei::core::TrackRole::Drums, sensei::core::TrackRole::Melody
        };
        return document_->project().findTrackByRole(roleIds[index]);
    }

    static double beatForX(float x, float labelW, float beatW) noexcept
    {
        return juce::jmax(0.0, (double) ((x - labelW) / beatW));
    }

    void drawClipPreview(juce::Graphics& g, const StudioPalette& p, const sensei::core::Track* track,
                         sensei::core::Id clipId, bool drums, juce::Rectangle<float> area) const
    {
        if (track == nullptr || area.getWidth() < 28.0f)
            return;
        g.setColour(p.clipText.withAlpha(0.28f));
        if (drums)
        {
            const auto* clip = document_->project().findDrumClip(track->id, clipId);
            if (clip == nullptr)
                return;
            const int steps = juce::jmin(clip->pattern.stepCount, 32);
            const float stepW = area.getWidth() / (float) juce::jmax(1, steps);
            for (const auto& hit : clip->pattern.hits)
            {
                if (hit.step >= steps)
                    continue;
                const float h = 3.0f + (float) static_cast<int>(hit.lane) * 2.5f;
                g.fillRoundedRectangle(area.getX() + (float) hit.step * stepW,
                                       area.getBottom() - h - 2.0f, juce::jmax(2.0f, stepW - 1.0f), h, 1.0f);
            }
        }
        else
        {
            const auto* clip = document_->project().findClip(track->id, clipId);
            if (clip == nullptr || clip->notes.empty())
                return;
            int minP = 127, maxP = 0;
            for (const auto& n : clip->notes)
            {
                minP = juce::jmin(minP, n.pitch);
                maxP = juce::jmax(maxP, n.pitch);
            }
            const float span = (float) juce::jmax(1, maxP - minP);
            for (const auto& n : clip->notes)
            {
                if (n.startBeat >= clip->lengthBeats)
                    continue;
                const float x = area.getX() + (float) (n.startBeat / clip->lengthBeats) * area.getWidth();
                const float w = juce::jmax(2.0f, (float) (n.lengthBeats / clip->lengthBeats) * area.getWidth());
                const float y = area.getY() + (1.0f - ((float) (n.pitch - minP) / span)) * (area.getHeight() - 4.0f);
                g.fillRoundedRectangle(x, y, w, 3.0f, 1.0f);
            }
        }
    }

    Hit hitTestClip(juce::Point<float> pos) const
    {
        Hit hit;
        if (document_ == nullptr)
            return hit;
        const auto& project = document_->project();
        const double songLen = juce::jmax(project.songLengthBeats(), sensei::core::kDefaultLoopBeats);
        const float laneH = laneHeight();
        const float headerH = 36.0f;
        const float beatW = ((float) getWidth() - kLabelW - 4.0f) / (float) songLen;

        static constexpr sensei::core::TrackRole roleIds[] {
            sensei::core::TrackRole::Chords, sensei::core::TrackRole::Bass,
            sensei::core::TrackRole::Drums, sensei::core::TrackRole::Melody
        };

        for (int i = 0; i < 4; ++i)
        {
            const float y = headerH + 6.0f + i * (laneH + kLaneGap);
            if (pos.y < y || pos.y > y + laneH)
                continue;
            const auto* track = project.findTrackByRole(roleIds[i]);
            if (track == nullptr)
                continue;

            auto consider = [&](sensei::core::Id clipId, double start, double length) {
                const float x = kLabelW + (float) start * beatW;
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
