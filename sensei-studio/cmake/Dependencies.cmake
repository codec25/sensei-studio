include(FetchContent)

# Pin exact versions for reproducible builds.
set(SENSEI_JUCE_GIT_TAG "8.0.6" CACHE STRING "Exact JUCE git tag")
set(SENSEI_CATCH2_GIT_TAG "v3.7.1" CACHE STRING "Exact Catch2 git tag")

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        ${SENSEI_JUCE_GIT_TAG}
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        ${SENSEI_CATCH2_GIT_TAG}
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(juce)

if(SENSEI_BUILD_TESTS)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()
