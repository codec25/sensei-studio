include(CTest)
include(Catch)

add_executable(sensei_core_tests
  core/TransportTests.cpp
  core/ProjectModelTests.cpp
  core/NoteEditingTests.cpp
  core/LoopTransportTests.cpp
  core/UndoRedoTests.cpp
  core/SenseiObservationTests.cpp
  core/SnapshotPublisherTests.cpp
  core/MultiTrackTests.cpp
  core/ChordTests.cpp
  core/DrumBassLessonTests.cpp
  core/ArrangementTests.cpp
  core/InstrumentTests.cpp
  core/MuteSoloTests.cpp
  ui/ThemeTests.cpp
  ${CMAKE_SOURCE_DIR}/apps/sensei_studio/Source/ui/Theme.cpp
  engine/MidiEventBufferTests.cpp
)

target_include_directories(sensei_core_tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/sensei_engine/include
    ${CMAKE_SOURCE_DIR}/apps/sensei_studio/Source
)

target_link_libraries(sensei_core_tests
  PRIVATE
    sensei::core
    sensei::engine
    juce::juce_gui_basics
    Catch2::Catch2WithMain
)

target_compile_features(sensei_core_tests PRIVATE cxx_std_20)

catch_discover_tests(sensei_core_tests)
