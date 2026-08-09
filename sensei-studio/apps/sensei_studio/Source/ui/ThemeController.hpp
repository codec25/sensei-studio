#pragma once

#include "Theme.hpp"

#include <juce_data_structures/juce_data_structures.h>

#include <functional>

// Owns LookAndFeel + UI-only preferences. Never touches Project / Core musical data.
class ThemeController final
{
public:
    std::function<void()> onThemeChanged;

    ThemeController();
    ~ThemeController();

    ThemeController(const ThemeController&) = delete;
    ThemeController& operator=(const ThemeController&) = delete;

    void setTheme(ThemeId id);
    [[nodiscard]] ThemeId themeId() const noexcept { return lookAndFeel_.themeId(); }
    [[nodiscard]] StudioLookAndFeel& lookAndFeel() noexcept { return lookAndFeel_; }
    [[nodiscard]] const StudioPalette& palette() const noexcept { return lookAndFeel_.palette(); }

    void setEditorHeightFraction(float fraction);
    [[nodiscard]] float editorHeightFraction() const noexcept { return editorHeightFraction_; }

    void setBrowserCollapsed(bool collapsed);
    [[nodiscard]] bool browserCollapsed() const noexcept { return browserCollapsed_; }

    void setSenseiCollapsed(bool collapsed);
    [[nodiscard]] bool senseiCollapsed() const noexcept { return senseiCollapsed_; }

private:
    void loadPrefs();
    void savePrefs();

    juce::ApplicationProperties properties_;
    StudioLookAndFeel lookAndFeel_;
    float editorHeightFraction_ = 0.38f;
    bool browserCollapsed_ = false;
    bool senseiCollapsed_ = false;
};
