add_library(sensei_core STATIC
  src/Transport.cpp
)

add_library(sensei::core ALIAS sensei_core)

target_include_directories(sensei_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sensei_core PUBLIC cxx_std_20)

# Core must remain free of JUCE UI, Engine, networking, and AI.
