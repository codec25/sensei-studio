#pragma once

#include "ui/Theme.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// Draggable horizontal bar that splits arrangement (above) from editor (below).
class VerticalSplitter final : public juce::Component
{
public:
    std::function<void(int deltaY)> onDragDelta;

    VerticalSplitter()
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        setRepaintsOnMouseActivity(true);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = studioPalette();
        g.fillAll(p.bg0);
        const float cy = (float) getHeight() * 0.5f;
        g.setColour(isMouseOverOrDragging() ? p.accent.withAlpha(0.7f) : p.borderSoft);
        g.fillRoundedRectangle(8.0f, cy - 1.5f, (float) getWidth() - 16.0f, 3.0f, 1.5f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        lastY_ = e.getEventRelativeTo(getParentComponent()).y;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const int y = e.getEventRelativeTo(getParentComponent()).y;
        const int delta = y - lastY_;
        lastY_ = y;
        if (delta != 0 && onDragDelta)
            onDragDelta(delta);
        repaint();
    }

private:
    bool isMouseOverOrDragging() const noexcept
    {
        return isMouseOver() || isMouseButtonDown();
    }

    int lastY_ = 0;
};
