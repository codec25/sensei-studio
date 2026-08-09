#include "ThemeController.hpp"

namespace {
constexpr const char* kThemeKey = "ui.themeId";
constexpr const char* kEditorFracKey = "ui.editorHeightFraction";
constexpr const char* kBrowserKey = "ui.browserCollapsed";
constexpr const char* kSenseiKey = "ui.senseiCollapsed";
}

ThemeController::ThemeController()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "SenseiStudio";
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "SenseiStudio";
    options.commonToAllUsers = false;
    properties_.setStorageParameters(options);
    loadPrefs();
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);
}

ThemeController::~ThemeController()
{
    if (&juce::LookAndFeel::getDefaultLookAndFeel() == &lookAndFeel_)
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    savePrefs();
}

void ThemeController::setTheme(ThemeId id)
{
    if (! isValidThemeId(id))
        id = defaultThemeId();
    if (lookAndFeel_.themeId() == id)
        return;
    lookAndFeel_.applyTheme(id);
    savePrefs();
    if (onThemeChanged)
        onThemeChanged();
}

void ThemeController::setEditorHeightFraction(float fraction)
{
    editorHeightFraction_ = juce::jlimit(0.18f, 0.62f, fraction);
    savePrefs();
}

void ThemeController::setBrowserCollapsed(bool collapsed)
{
    browserCollapsed_ = collapsed;
    savePrefs();
}

void ThemeController::setSenseiCollapsed(bool collapsed)
{
    senseiCollapsed_ = collapsed;
    savePrefs();
}

void ThemeController::loadPrefs()
{
    if (auto* file = properties_.getUserSettings())
    {
        const int theme = file->getIntValue(kThemeKey, static_cast<int>(defaultThemeId()));
        auto id = static_cast<ThemeId>(theme);
        if (! isValidThemeId(id))
            id = defaultThemeId();
        lookAndFeel_.applyTheme(id);
        editorHeightFraction_ = (float) file->getDoubleValue(kEditorFracKey, 0.38);
        editorHeightFraction_ = juce::jlimit(0.18f, 0.62f, editorHeightFraction_);
        browserCollapsed_ = file->getBoolValue(kBrowserKey, false);
        senseiCollapsed_ = file->getBoolValue(kSenseiKey, false);
    }
}

void ThemeController::savePrefs()
{
    if (auto* file = properties_.getUserSettings())
    {
        file->setValue(kThemeKey, static_cast<int>(lookAndFeel_.themeId()));
        file->setValue(kEditorFracKey, (double) editorHeightFraction_);
        file->setValue(kBrowserKey, browserCollapsed_);
        file->setValue(kSenseiKey, senseiCollapsed_);
        file->saveIfNeeded();
    }
}
