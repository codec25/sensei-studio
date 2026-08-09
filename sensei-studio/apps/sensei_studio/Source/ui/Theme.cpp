#include "Theme.hpp"

namespace {

StudioPalette makeDaylight() noexcept
{
    StudioPalette p;
    p.bg0 = juce::Colour(0xffe8ecf1);
    p.bg1 = juce::Colour(0xfff4f6f8);
    p.bg2 = juce::Colour(0xffffffff);
    p.panelSoft = juce::Colour(0xffdde3ea);
    p.textPrimary = juce::Colour(0xff1a2230);
    p.textSecondary = juce::Colour(0xff3d4a5c);
    p.textMuted = juce::Colour(0xff6b7788);
    p.accent = juce::Colour(0xff6f9f1a);
    p.accentSoft = juce::Colour(0x336f9f1a);
    p.playhead = juce::Colour(0xffd64545);
    p.borderSoft = juce::Colour(0xffc5ced8);
    p.gridMajor = juce::Colour(0xffb8c2ce);
    p.gridMinor = juce::Colour(0xffd5dde6);
    p.clipText = juce::Colour(0xff121820);
    p.roleChords = juce::Colour(0xff3d7dd6);
    p.roleBass = juce::Colour(0xffc97a2c);
    p.roleDrums = juce::Colour(0xff6f9f1a);
    p.roleMelody = juce::Colour(0xff8b5cb8);
    p.senseiGlow = juce::Colour(0x556f9f1a);
    p.transportBg = juce::Colour(0xffeef1f5);
    p.danger = juce::Colour(0xffc0392b);
    p.selectedOutline = juce::Colour(0xff1a2230);
    return p;
}

StudioPalette makeNightStudio() noexcept
{
    StudioPalette p;
    p.bg0 = juce::Colour(0xff0f1115);
    p.bg1 = juce::Colour(0xff171a20);
    p.bg2 = juce::Colour(0xff1e232b);
    p.panelSoft = juce::Colour(0xff222832);
    p.textPrimary = juce::Colour(0xfff0f2f5);
    p.textSecondary = juce::Colour(0xffb7c0cc);
    p.textMuted = juce::Colour(0xff7a8492);
    p.accent = juce::Colour(0xffd5ff5c);
    p.accentSoft = juce::Colour(0x22d5ff5c);
    p.playhead = juce::Colour(0xffff6b6b);
    p.borderSoft = juce::Colour(0xff2a303a);
    p.gridMajor = juce::Colour(0xff3a4452);
    p.gridMinor = juce::Colour(0xff272d35);
    p.clipText = juce::Colour(0xff0f1115);
    p.roleChords = juce::Colour(0xff6ea8fe);
    p.roleBass = juce::Colour(0xfff0a35e);
    p.roleDrums = juce::Colour(0xffd5ff5c);
    p.roleMelody = juce::Colour(0xffc792ea);
    p.senseiGlow = juce::Colour(0x44d5ff5c);
    p.transportBg = juce::Colour(0xff12151a);
    p.danger = juce::Colour(0xffff6b6b);
    p.selectedOutline = juce::Colour(0xffffffff);
    return p;
}

StudioPalette makeWarmAmber() noexcept
{
    StudioPalette p;
    p.bg0 = juce::Colour(0xff1a1410);
    p.bg1 = juce::Colour(0xff221c16);
    p.bg2 = juce::Colour(0xff2a221a);
    p.panelSoft = juce::Colour(0xff332a20);
    p.textPrimary = juce::Colour(0xfff3e6d4);
    p.textSecondary = juce::Colour(0xffcbb8a0);
    p.textMuted = juce::Colour(0xff8f7d68);
    p.accent = juce::Colour(0xffe0b35a);
    p.accentSoft = juce::Colour(0x33e0b35a);
    p.playhead = juce::Colour(0xffff8a5c);
    p.borderSoft = juce::Colour(0xff3d3228);
    p.gridMajor = juce::Colour(0xff4a3d30);
    p.gridMinor = juce::Colour(0xff302820);
    p.clipText = juce::Colour(0xff1a1410);
    p.roleChords = juce::Colour(0xff7aa6d9);
    p.roleBass = juce::Colour(0xffe09a4a);
    p.roleDrums = juce::Colour(0xffc9c05a);
    p.roleMelody = juce::Colour(0xffc49ad9);
    p.senseiGlow = juce::Colour(0x55e0b35a);
    p.transportBg = juce::Colour(0xff1c1612);
    p.danger = juce::Colour(0xffff8a5c);
    p.selectedOutline = juce::Colour(0xfff3e6d4);
    return p;
}

StudioPalette makeHighContrast() noexcept
{
    StudioPalette p;
    p.bg0 = juce::Colour(0xff000000);
    p.bg1 = juce::Colour(0xff0a0a0a);
    p.bg2 = juce::Colour(0xff141414);
    p.panelSoft = juce::Colour(0xff1e1e1e);
    p.textPrimary = juce::Colour(0xffffffff);
    p.textSecondary = juce::Colour(0xffe6e6e6);
    p.textMuted = juce::Colour(0xffb0b0b0);
    p.accent = juce::Colour(0xffe8ff66);
    p.accentSoft = juce::Colour(0x44e8ff66);
    p.playhead = juce::Colour(0xffff3344);
    p.borderSoft = juce::Colour(0xff555555);
    p.gridMajor = juce::Colour(0xff777777);
    p.gridMinor = juce::Colour(0xff333333);
    p.clipText = juce::Colour(0xff000000);
    p.roleChords = juce::Colour(0xff66b3ff);
    p.roleBass = juce::Colour(0xffffaa44);
    p.roleDrums = juce::Colour(0xffe8ff66);
    p.roleMelody = juce::Colour(0xffff88ff);
    p.senseiGlow = juce::Colour(0x66e8ff66);
    p.transportBg = juce::Colour(0xff000000);
    p.danger = juce::Colour(0xffff3344);
    p.selectedOutline = juce::Colour(0xffffffff);
    return p;
}

} // namespace

StudioPalette paletteFor(ThemeId id) noexcept
{
    switch (id)
    {
        case ThemeId::Daylight: return makeDaylight();
        case ThemeId::WarmAmber: return makeWarmAmber();
        case ThemeId::HighContrast: return makeHighContrast();
        case ThemeId::NightStudio: return makeNightStudio();
        case ThemeId::Count: break;
    }
    return makeNightStudio();
}

StudioLookAndFeel::StudioLookAndFeel()
{
    applyTheme(defaultThemeId());
}

void StudioLookAndFeel::applyTheme(ThemeId id)
{
    themeId_ = isValidThemeId(id) ? id : defaultThemeId();
    palette_ = paletteFor(themeId_);
    applyPaletteToWidgets();
}

void StudioLookAndFeel::applyPaletteToWidgets()
{
    setColourScheme({
        palette_.bg0,
        palette_.bg1,
        palette_.bg2,
        palette_.borderSoft,
        palette_.textPrimary,
        palette_.accentSoft,
        palette_.accent,
        palette_.clipText,
        palette_.textPrimary
    });

    setColour(juce::ResizableWindow::backgroundColourId, palette_.bg0);
    setColour(juce::Label::textColourId, palette_.textPrimary);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, palette_.panelSoft);
    setColour(juce::TextButton::buttonOnColourId, palette_.accent.withMultipliedAlpha(0.85f));
    setColour(juce::TextButton::textColourOffId, palette_.textPrimary);
    setColour(juce::TextButton::textColourOnId, palette_.clipText);
    setColour(juce::ComboBox::backgroundColourId, palette_.bg2);
    setColour(juce::ComboBox::outlineColourId, palette_.borderSoft);
    setColour(juce::ComboBox::textColourId, palette_.textPrimary);
    setColour(juce::ComboBox::arrowColourId, palette_.textMuted);
    setColour(juce::PopupMenu::backgroundColourId, palette_.bg1);
    setColour(juce::PopupMenu::textColourId, palette_.textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, palette_.accentSoft);
    setColour(juce::PopupMenu::highlightedTextColourId, palette_.textPrimary);
    setColour(juce::ScrollBar::thumbColourId, palette_.borderSoft);
    setColour(juce::CaretComponent::caretColourId, palette_.accent);
}

juce::Font StudioLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions(juce::jmin(15.0f, (float) buttonHeight * 0.48f));
}

juce::Font StudioLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::FontOptions(14.0f);
}

juce::Font StudioLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::FontOptions(14.0f);
}

const StudioPalette& studioPalette()
{
    if (auto* lf = dynamic_cast<StudioLookAndFeel*>(&juce::LookAndFeel::getDefaultLookAndFeel()))
        return lf->palette();

    static const StudioPalette fallback = paletteFor(defaultThemeId());
    return fallback;
}

void drawSenseiOrb(juce::Graphics& g, juce::Rectangle<float> bounds, const StudioPalette& p,
                   float glowStrength)
{
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    if (glowStrength > 0.01f)
    {
        juce::ColourGradient glow(
            p.senseiGlow.withMultipliedAlpha(glowStrength), centre.x, centre.y,
            p.senseiGlow.withAlpha(0.0f), centre.x, centre.y - radius * 1.6f, true);
        g.setGradientFill(glow);
        g.fillEllipse(bounds.expanded(radius * 0.45f));
    }

    juce::ColourGradient body(
        p.accent.brighter(0.25f), centre.x - radius * 0.3f, centre.y - radius * 0.35f,
        p.accent.darker(0.35f), centre.x + radius * 0.4f, centre.y + radius * 0.45f, false);
    g.setGradientFill(body);
    g.fillEllipse(bounds);

    g.setColour(p.accentSoft.withMultipliedAlpha(1.2f));
    g.drawEllipse(bounds.reduced(radius * 0.18f), 1.2f);

    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.fillEllipse(bounds.getX() + radius * 0.35f, bounds.getY() + radius * 0.22f,
                  radius * 0.55f, radius * 0.35f);
}
