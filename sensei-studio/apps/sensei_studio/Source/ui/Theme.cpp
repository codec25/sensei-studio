#include "Theme.hpp"

namespace {

StudioPalette makeDaylight() noexcept
{
    StudioPalette p;
    // F.1 flagship: bright neutral canvas, clean elevated work surfaces and
    // restrained colour. The arrangement should read before the chrome.
    p.bg0 = juce::Colour(0xfff3f5f7);
    p.bg1 = juce::Colour(0xfff8f9fb);
    p.bg2 = juce::Colour(0xffffffff);
    p.panelSoft = juce::Colour(0xffedf0f3);
    p.textPrimary = juce::Colour(0xff111820);
    p.textSecondary = juce::Colour(0xff394553);
    p.textMuted = juce::Colour(0xff687482);
    p.accent = juce::Colour(0xff6f981c);
    p.accentSoft = juce::Colour(0x246f981c);
    p.playhead = juce::Colour(0xffd33d49);
    p.borderSoft = juce::Colour(0xffd9dee5);
    p.gridMajor = juce::Colour(0xffcbd2da);
    p.gridMinor = juce::Colour(0xffe7eaee);
    p.clipText = juce::Colour(0xff10161d);
    p.roleChords = juce::Colour(0xff3977c9);
    p.roleBass = juce::Colour(0xffbd7128);
    p.roleDrums = juce::Colour(0xff70981d);
    p.roleMelody = juce::Colour(0xff8254b2);
    p.senseiGlow = juce::Colour(0x426f981c);
    p.transportBg = juce::Colour(0xfffbfcfd);
    p.danger = juce::Colour(0xffbd3540);
    p.selectedOutline = juce::Colour(0xff17212b);
    return p;
}

StudioPalette makeNightStudio() noexcept
{
    StudioPalette p;
    p.bg0 = juce::Colour(0xff101318);
    p.bg1 = juce::Colour(0xff171b21);
    p.bg2 = juce::Colour(0xff20252d);
    p.panelSoft = juce::Colour(0xff272d36);
    p.textPrimary = juce::Colour(0xfff4f6f8);
    p.textSecondary = juce::Colour(0xffc0c7d0);
    p.textMuted = juce::Colour(0xff89939f);
    p.accent = juce::Colour(0xffcdea58);
    p.accentSoft = juce::Colour(0x24cdea58);
    p.playhead = juce::Colour(0xffff686f);
    p.borderSoft = juce::Colour(0xff303741);
    p.gridMajor = juce::Colour(0xff414b58);
    p.gridMinor = juce::Colour(0xff2a3038);
    p.clipText = juce::Colour(0xff101318);
    p.roleChords = juce::Colour(0xff70a8ee);
    p.roleBass = juce::Colour(0xffeda05d);
    p.roleDrums = juce::Colour(0xffcdea58);
    p.roleMelody = juce::Colour(0xffbd8ce1);
    p.senseiGlow = juce::Colour(0x40cdea58);
    p.transportBg = juce::Colour(0xff14181e);
    p.danger = juce::Colour(0xffff686f);
    p.selectedOutline = juce::Colour(0xfff7f9fb);
    return p;
}

StudioPalette makeWarmAmber() noexcept
{
    StudioPalette p;
    // Warm, not brown: charcoal-neutral surfaces with an amber atmosphere.
    p.bg0 = juce::Colour(0xff181613);
    p.bg1 = juce::Colour(0xff201d19);
    p.bg2 = juce::Colour(0xff29251f);
    p.panelSoft = juce::Colour(0xff322d26);
    p.textPrimary = juce::Colour(0xfff5eadb);
    p.textSecondary = juce::Colour(0xffd2c1aa);
    p.textMuted = juce::Colour(0xff9b8b77);
    p.accent = juce::Colour(0xffe0b45e);
    p.accentSoft = juce::Colour(0x2ee0b45e);
    p.playhead = juce::Colour(0xffff8660);
    p.borderSoft = juce::Colour(0xff40392f);
    p.gridMajor = juce::Colour(0xff51483b);
    p.gridMinor = juce::Colour(0xff302b25);
    p.clipText = juce::Colour(0xff181613);
    p.roleChords = juce::Colour(0xff7fa9d5);
    p.roleBass = juce::Colour(0xffe39b4d);
    p.roleDrums = juce::Colour(0xffc8bd62);
    p.roleMelody = juce::Colour(0xffc29ad6);
    p.senseiGlow = juce::Colour(0x4ee0b45e);
    p.transportBg = juce::Colour(0xff1c1916);
    p.danger = juce::Colour(0xffff8660);
    p.selectedOutline = juce::Colour(0xfff5eadb);
    return p;
}

StudioPalette makeHighContrast() noexcept
{
    StudioPalette p;
    // Intentionally designed accessibility theme: maximum separation without
    // making every surface compete for attention.
    p.bg0 = juce::Colour(0xff000000);
    p.bg1 = juce::Colour(0xff090909);
    p.bg2 = juce::Colour(0xff171717);
    p.panelSoft = juce::Colour(0xff242424);
    p.textPrimary = juce::Colour(0xffffffff);
    p.textSecondary = juce::Colour(0xffeeeeee);
    p.textMuted = juce::Colour(0xffc2c2c2);
    p.accent = juce::Colour(0xffe9ff70);
    p.accentSoft = juce::Colour(0x42e9ff70);
    p.playhead = juce::Colour(0xffff4050);
    p.borderSoft = juce::Colour(0xff666666);
    p.gridMajor = juce::Colour(0xff858585);
    p.gridMinor = juce::Colour(0xff3b3b3b);
    p.clipText = juce::Colour(0xff000000);
    p.roleChords = juce::Colour(0xff72bcff);
    p.roleBass = juce::Colour(0xffffb354);
    p.roleDrums = juce::Colour(0xffe9ff70);
    p.roleMelody = juce::Colour(0xffff96ff);
    p.senseiGlow = juce::Colour(0x60e9ff70);
    p.transportBg = juce::Colour(0xff000000);
    p.danger = juce::Colour(0xffff4050);
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
    setColour(juce::TextButton::buttonOnColourId, palette_.accent.withMultipliedAlpha(0.88f));
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
    setColour(juce::ListBox::backgroundColourId, palette_.bg1);
    setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ScrollBar::thumbColourId, palette_.borderSoft);
    setColour(juce::CaretComponent::caretColourId, palette_.accent);
}

juce::Font StudioLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions(juce::jlimit(13.0f, 15.5f, (float) buttonHeight * 0.44f));
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
