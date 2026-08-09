#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

// UI-only lighting / theme system. Core and Engine must never include this.

enum class ThemeId : std::uint8_t
{
    Daylight = 0,
    NightStudio,
    WarmAmber,
    HighContrast,
    Count
};

[[nodiscard]] inline constexpr ThemeId defaultThemeId() noexcept
{
    return ThemeId::NightStudio;
}

[[nodiscard]] inline const char* themeDisplayName(ThemeId id) noexcept
{
    switch (id)
    {
        case ThemeId::Daylight: return "Daylight";
        case ThemeId::NightStudio: return "Night Studio";
        case ThemeId::WarmAmber: return "Warm Amber";
        case ThemeId::HighContrast: return "High Contrast";
        case ThemeId::Count: break;
    }
    return "Night Studio";
}

[[nodiscard]] inline bool isValidThemeId(ThemeId id) noexcept
{
    return static_cast<std::uint8_t>(id) < static_cast<std::uint8_t>(ThemeId::Count);
}

struct StudioPalette
{
    juce::Colour bg0;           // deepest canvas
    juce::Colour bg1;           // panel
    juce::Colour bg2;           // elevated / lane
    juce::Colour panelSoft;     // subtle fill
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    juce::Colour textMuted;
    juce::Colour accent;        // Sensei lime family (theme-tinted)
    juce::Colour accentSoft;
    juce::Colour playhead;
    juce::Colour borderSoft;
    juce::Colour gridMajor;
    juce::Colour gridMinor;
    juce::Colour clipText;
    juce::Colour roleChords;
    juce::Colour roleBass;
    juce::Colour roleDrums;
    juce::Colour roleMelody;
    juce::Colour senseiGlow;
    juce::Colour transportBg;
    juce::Colour danger;
    juce::Colour selectedOutline;
};

[[nodiscard]] StudioPalette paletteFor(ThemeId id) noexcept;

class StudioLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    StudioLookAndFeel();

    void applyTheme(ThemeId id);
    [[nodiscard]] ThemeId themeId() const noexcept { return themeId_; }
    [[nodiscard]] const StudioPalette& palette() const noexcept { return palette_; }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

private:
    void applyPaletteToWidgets();

    ThemeId themeId_ = defaultThemeId();
    StudioPalette palette_ {};
};

// Narrow UI helper: reads palette from the installed LookAndFeel (no Theme singleton).
[[nodiscard]] const StudioPalette& studioPalette();

void drawSenseiOrb(juce::Graphics& g, juce::Rectangle<float> bounds, const StudioPalette& p,
                   float glowStrength = 1.0f);
