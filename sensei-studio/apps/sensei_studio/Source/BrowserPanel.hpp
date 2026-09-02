#pragma once

#include "sensei/core/Document.hpp"
#include "sensei/core/InstrumentId.hpp"
#include "sensei/core/commands/InstrumentCommands.hpp"
#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// Focused creation browser. It exposes only things that work in the current build;
// future libraries stay out of the primary workflow until they are usable.
class BrowserPanel final : public juce::Component
{
public:
    std::function<void()> onChanged;
    std::function<void()> onCollapseToggle;

    BrowserPanel()
    {
        title_.setText("Create", juce::dontSendNotification);
        title_.setFont(juce::FontOptions(17.0f).withStyle("Bold"));
        collapseBtn_.setButtonText("«");
        collapseBtn_.setTooltip("Hide Create browser");
        collapseBtn_.onClick = [this] {
            if (onCollapseToggle)
                onCollapseToggle();
        };

        // F.1: don't advertise an empty Sounds library. Every primary category
        // must lead to a useful action now.
        category_.addItem("Instruments", 1);
        category_.addItem("Drums", 2);
        category_.addItem("Guided", 3);
        category_.setSelectedId(1, juce::dontSendNotification);
        category_.onChange = [this] { rebuildItems(); };

        model_.owner = this;
        addAndMakeVisible(title_);
        addAndMakeVisible(collapseBtn_);
        addAndMakeVisible(category_);
        addAndMakeVisible(list_);
        addAndMakeVisible(hint_);
        hint_.setJustificationType(juce::Justification::topLeft);
        list_.setRowHeight(44); // touch-friendly and easier to scan
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
            expandBtn_.setButtonText("+");
            expandBtn_.setTooltip("Show Create browser");
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
        g.setColour(p.borderSoft.withAlpha(0.55f));
        g.drawLine((float) getWidth() - 0.5f, 0.0f, (float) getWidth() - 0.5f, (float) getHeight());
        if (collapsed_)
        {
            g.setColour(p.textMuted);
            g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
            g.drawFittedText("CREATE", getLocalBounds().withTrimmedTop(42).reduced(3),
                             juce::Justification::centredTop, 1, 0.7f);
        }
    }

    void resized() override
    {
        if (collapsed_)
        {
            expandBtn_.setBounds(getLocalBounds().reduced(5).removeFromTop(34));
            return;
        }
        auto area = getLocalBounds().reduced(14, 12);
        auto top = area.removeFromTop(34);
        title_.setBounds(top.removeFromLeft(juce::jmax(0, top.getWidth() - 38)));
        collapseBtn_.setBounds(top.reduced(2));
        area.removeFromTop(10);
        category_.setBounds(area.removeFromTop(36));
        area.removeFromTop(10);
        hint_.setBounds(area.removeFromBottom(48));
        area.removeFromBottom(8);
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
            auto r = juce::Rectangle<float>(4.0f, 3.0f, (float) w - 8.0f, (float) h - 6.0f);
            if (selected)
            {
                g.setColour(p.accentSoft);
                g.fillRoundedRectangle(r, 7.0f);
                g.setColour(p.accent.withAlpha(0.65f));
                g.drawRoundedRectangle(r, 7.0f, 1.0f);
            }
            g.setColour(p.textPrimary);
            g.setFont(juce::FontOptions(14.0f));
            g.drawText(owner->items_[(size_t) row].label, 14, 0, w - 20, h,
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
        if (cat == 1)
        {
            hint_.setText("Choose a sound for the selected musical track.",
                          juce::dontSendNotification);
            addInstrument(sensei::core::InstrumentId::WarmKeys);
            addInstrument(sensei::core::InstrumentId::DeepBass);
            addInstrument(sensei::core::InstrumentId::BrightPluck);
        }
        else if (cat == 2)
        {
            hint_.setText("Choose the kit, then shape the groove below.", juce::dontSendNotification);
            addInstrument(sensei::core::InstrumentId::StudioKitBasic);
        }
        else
        {
            hint_.setText("Useful starting moves. Sensei explains the result in your song.",
                          juce::dontSendNotification);
            items_.push_back({ "Start with chords", [this] {
                if (document_ == nullptr) return;
                if (auto* t = document_->project().findTrackByRole(sensei::core::TrackRole::Chords))
                    document_->setSelectedTrackId(t->id);
                if (onChanged) onChanged();
            } });
            items_.push_back({ "Build a starter beat", [this] {
                if (document_ == nullptr) return;
                document_->applyStarterDrums("basic-rock");
                if (onChanged) onChanged();
            } });
            items_.push_back({ "Add bass from the chords", [this] {
                if (document_ == nullptr) return;
                document_->applyRootBass();
                if (onChanged) onChanged();
            } });
            items_.push_back({ "Shape this loop into a song", [this] {
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
