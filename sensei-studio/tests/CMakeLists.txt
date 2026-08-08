include(CTest)
include(Catch)

add_executable(sensei_core_tests
  core/TransportTests.cpp
)

target_link_libraries(sensei_core_tests
  PRIVATE
    sensei::core
    Catch2::Catch2WithMain
)

target_compile_features(sensei_core_tests PRIVATE cxx_std_20)

catch_discover_tests(sensei_core_tests)
