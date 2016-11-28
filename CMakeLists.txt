cmake_minimum_required(VERSION 2.6)
project(ambisonic)

include_directories(include source source/kiss_fft)

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp)

add_library(ambisonic STATIC ${sources})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ambisonic.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)
