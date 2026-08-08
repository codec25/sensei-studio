cmake_minimum_required(VERSION 3.24)

project(SenseiStudio
  VERSION 0.1.0
  DESCRIPTION "Sensei Studio — music production learning environment"
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SENSEI_BUILD_TESTS "Build Sensei Core unit tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)

add_subdirectory(libs/sensei_core)
add_subdirectory(libs/sensei_engine)
add_subdirectory(apps/sensei_studio)

if(SENSEI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
