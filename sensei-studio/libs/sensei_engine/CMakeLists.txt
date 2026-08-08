add_library(sensei_engine STATIC
  src/AudioEngine.cpp
  src/SimpleSynth.cpp
)

add_library(sensei::engine ALIAS sensei_engine)

target_include_directories(sensei_engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sensei_engine PUBLIC cxx_std_20)

target_link_libraries(sensei_engine
  PUBLIC
    sensei::core
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(sensei_engine
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Engine may use JUCE audio modules only. It must not link GUI or teaching code.
