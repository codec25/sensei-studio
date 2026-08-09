#include <catch2/catch_test_macros.hpp>

#include "../../apps/sensei_studio/Source/ui/Theme.hpp"

#include "sensei/core/Document.hpp"

TEST_CASE("Default theme is Night Studio", "[theme]")
{
    REQUIRE(defaultThemeId() == ThemeId::NightStudio);
    REQUIRE(std::string(themeDisplayName(ThemeId::Daylight)) == "Daylight");
    REQUIRE(std::string(themeDisplayName(ThemeId::WarmAmber)) == "Warm Amber");
    REQUIRE(std::string(themeDisplayName(ThemeId::HighContrast)) == "High Contrast");
    REQUIRE(isValidThemeId(ThemeId::Daylight));
    REQUIRE_FALSE(isValidThemeId(ThemeId::Count));
}

TEST_CASE("Theme palettes are distinct", "[theme]")
{
    const auto day = paletteFor(ThemeId::Daylight);
    const auto night = paletteFor(ThemeId::NightStudio);
    const auto amber = paletteFor(ThemeId::WarmAmber);
    const auto hc = paletteFor(ThemeId::HighContrast);

    REQUIRE(day.bg0 != night.bg0);
    REQUIRE(night.bg0 != amber.bg0);
    REQUIRE(amber.bg0 != hc.bg0);
    REQUIRE(day.textPrimary != night.textPrimary);
}

TEST_CASE("Applying theme does not mutate Project", "[theme]")
{
    sensei::core::Document doc;
    const auto noteCount = doc.project().totalNoteCount();
    const auto songLen = doc.project().songLengthBeats();
    const auto* chords = doc.project().findTrackByRole(sensei::core::TrackRole::Chords);
    REQUIRE(chords != nullptr);
    const auto instrument = chords->instrumentId;
    const auto muted = chords->muted;

    StudioLookAndFeel lf;
    lf.applyTheme(ThemeId::Daylight);
    lf.applyTheme(ThemeId::WarmAmber);
    lf.applyTheme(ThemeId::HighContrast);
    lf.applyTheme(ThemeId::NightStudio);

    REQUIRE(doc.project().totalNoteCount() == noteCount);
    REQUIRE(doc.project().songLengthBeats() == songLen);
    REQUIRE(chords->instrumentId == instrument);
    REQUIRE(chords->muted == muted);
}
