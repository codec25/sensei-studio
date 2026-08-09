#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/commands/InstrumentCommands.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// Shallow beginner browser — built-in instruments / kit / lesson actions only.
class BrowserPanel final : public juce::Component
{
public:
    std::function<void()> onChanged;
    std::function<void()> onCollapseToggle;

    BrowserPanel()
    {
        title_.setText("Browser", juce::dontSendNotification);
        title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        collapseBtn_.setButtonText("«");
        collapseBtn_.setTooltip("Collapse browser");
        collapseBtn_.onClick = [this] {
            if (onCollapseToggle)
                onCollapseToggle();
        };

        category_.addItem("Instruments", 1);
        category_.addItem("Drums", 2);
        category_.addItem("Sounds", 3);
        category_.addItem("Lessons", 4);
        category_.setSelectedId(1, juce::dontSendNotification);
        category_.onChange = [this] { rebuildItems(); };

        model_.owner = this;
        addAndMakeVisible(title_);
        addAndMakeVisible(collapseBtn_);
        addAndMakeVisible(category_);
        addAndMakeVisible(list_);
        addAndMakeVisible(hint_);
        hint_.setJustificationType(juce::Justification::topLeft);
        list_.setRowHeight(28);
        list_.setModel(&model_);
        rebuildItems();
    }

    void setDocument(sensei::core::Document* document)
    {
        document_ = document;
        rebuildItems();
    }

    void setCollapsed(bool collapsed)
    {
        collapsed_ = collapsed;
        for (auto* c : getChildren())
            c->setVisible(! collapsed_);
        if (collapsed_)
        {
            expandBtn_.setVisible(true);
            if (! expandBtn_.getParentComponent())
                addAndMakeVisible(expandBtn_);
            expandBtn_.setButtonText("›");
            expandBtn_.setTooltip("Expand browser");
            expandBtn_.onClick = [this] {
                if (onCollapseToggle)
                    onCollapseToggle();
            };
        }
        else
        {
            expandBtn_.setVisible(false);
        }
        resized();
        repaint();
    }

    [[nodiscard]] bool isCollapsed() const noexcept { return collapsed_; }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        g.fillAll(p.bg1);
        g.setColour(p.borderSoft.withAlpha(0.65f));
        g.drawLine((float) getWidth() - 0.5f, 0.0f, (float) getWidth() - 0.5f, (float) getHeight());
        if (collapsed_)
        {
            g.setColour(p.textMuted);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText("B", getLocalBounds().withTrimmedTop(36), juce::Justification::centredTop, false);
        }
    }

    void resized() override
    {
        if (collapsed_)
        {
            expandBtn_.setBounds(getLocalBounds().reduced(4).removeFromTop(28));
            return;
        }
        auto area = getLocalBounds().reduced(12, 10);
        auto top = area.removeFromTop(28);
        title_.setBounds(top.removeFromLeft(top.getWidth() - 32));
        collapseBtn_.setBounds(top);
        area.removeFromTop(8);
        category_.setBounds(area.removeFromTop(30));
        area.removeFromTop(8);
        hint_.setBounds(area.removeFromBottom(52));
        area.removeFromBottom(6);
        list_.setBounds(area);
    }

private:
    struct Item
    {
        juce::String label;
        std::function<void()> action;
    };

    class Model final : public juce::ListBoxModel
    {
    public:
        BrowserPanel* owner = nullptr;
        int getNumRows() override { return owner != nullptr ? (int) owner->items_.size() : 0; }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (owner == nullptr || ! juce::isPositiveAndBelow(row, (int) owner->items_.size()))
                return;
            const auto& p = studioPalette();
            if (selected)
                g.fillAll(p.accentSoft);
            g.setColour(p.textPrimary);
            g.setFont(juce::FontOptions(13.5f));
            g.drawText(owner->items_[(size_t) row].label, 8, 0, w - 12, h,
                       juce::Justification::centredLeft, true);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override
        {
            if (owner == nullptr || ! juce::isPositiveAndBelow(row, (int) owner->items_.size()))
                return;
            if (owner->items_[(size_t) row].action)
                owner->items_[(size_t) row].action();
        }
    };

    void rebuildItems()
    {
        items_.clear();
        hint_.setColour(juce::Label::textColourId, studioPalette().textMuted);

        const int cat = category_.getSelectedId();
        if (cat == 1) // Instruments
        {
            hint_.setText("Built-in pitched instruments for the selected MIDI track.",
                          juce::dontSendNotification);
            addInstrument(sensei::core::InstrumentId::WarmKeys);
            addInstrument(sensei::core::InstrumentId::DeepBass);
            addInstrument(sensei::core::InstrumentId::BrightPluck);
        }
        else if (cat == 2) // Drums
        {
            hint_.setText("Current drum kit for the Drums track.", juce::dontSendNotification);
            addInstrument(sensei::core::InstrumentId::StudioKitBasic);
        }
        else if (cat == 3) // Sounds
        {
            hint_.setText("Sounds library coming later — no samples in this build.",
                          juce::dontSendNotification);
            items_.push_back({ "No sample library yet", {} });
        }
        else // Lessons
        {
            hint_.setText("Deterministic Sensei helpers — same Core lesson flow.",
                          juce::dontSendNotification);
            items_.push_back({ "Focus Chords track", [this] {
                if (document_ == nullptr) return;
                if (auto* t = document_->project().findTrackByRole(sensei::core::TrackRole::Chords))
                    document_->setSelectedTrackId(t->id);
                if (onChanged) onChanged();
            } });
            items_.push_back({ "Add starter drums", [this] {
                if (document_ == nullptr) return;
                document_->applyStarterDrums("basic-rock");
                if (onChanged) onChanged();
            } });
            items_.push_back({ "Add root-note bass", [this] {
                if (document_ == nullptr) return;
                document_->applyRootBass();
                if (onChanged) onChanged();
            } });
            items_.push_back({ "Turn loop into a song", [this] {
                if (document_ == nullptr) return;
                document_->applySongShape();
                if (onChanged) onChanged();
            } });
        }
        list_.updateContent();
        list_.repaint();
        resized();
    }

    void addInstrument(sensei::core::InstrumentId id)
    {
        const auto info = sensei::core::instrumentInfo(id);
        items_.push_back({ juce::String(info.displayName), [this, id] {
            if (document_ == nullptr)
                return;
            auto* track = document_->project().findTrack(document_->selectedTrackId());
            if (track == nullptr)
                return;
            const auto meta = sensei::core::instrumentInfo(id);
            if (track->type == sensei::core::TrackType::Drums && ! meta.isDrumKit)
            {
                if (auto* drums = document_->project().findTrackByRole(sensei::core::TrackRole::Drums))
                    document_->setSelectedTrackId(drums->id);
                track = document_->project().findTrack(document_->selectedTrackId());
            }
            if (track->type == sensei::core::TrackType::Midi && meta.isDrumKit)
            {
                if (auto* drums = document_->project().findTrackByRole(sensei::core::TrackRole::Drums))
                    document_->setSelectedTrackId(drums->id);
                track = document_->project().findTrack(document_->selectedTrackId());
            }
            if (track == nullptr)
                return;
            if (track->type == sensei::core::TrackType::Drums && ! meta.isDrumKit)
                return;
            if (track->type == sensei::core::TrackType::Midi && meta.isDrumKit)
                return;
            if (track->instrumentId == id)
                return;
            document_->execute(std::make_unique<sensei::core::SetTrackInstrumentCommand>(track->id, id));
            if (onChanged)
                onChanged();
        } });
    }

    sensei::core::Document* document_ = nullptr;
    bool collapsed_ = false;
    juce::Label title_;
    juce::Label hint_;
    juce::TextButton collapseBtn_;
    juce::TextButton expandBtn_;
    juce::ComboBox category_;
    juce::ListBox list_ { "browser", nullptr };
    Model model_;
    std::vector<Item> items_;
};
