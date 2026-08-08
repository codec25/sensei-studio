juce_add_gui_app(sensei_studio
  PRODUCT_NAME "Sensei Studio"
  COMPANY_NAME "Sensei"
  BUNDLE_ID "studio.sensei.app"
  NEEDS_CURL FALSE
  NEEDS_WEB_BROWSER FALSE
)

target_sources(sensei_studio
  PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.hpp
    Source/TransportBar.cpp
    Source/TransportBar.hpp
    Source/NoteKeyboard.cpp
    Source/NoteKeyboard.hpp
    Source/PianoRoll.cpp
    Source/PianoRoll.hpp
    Source/SenseiPanel.cpp
    Source/SenseiPanel.hpp
    Source/TrackList.hpp
    Source/ChordHelperPanel.hpp
    Source/DrumGrid.hpp
)

target_compile_definitions(sensei_studio
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:sensei_studio,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:sensei_studio,JUCE_VERSION>"
)

target_link_libraries(sensei_studio
  PRIVATE
    sensei::core
    sensei::engine
    juce::juce_gui_extra
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_compile_features(sensei_studio PRIVATE cxx_std_20)
