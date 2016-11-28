cmake_minimum_required(VERSION 2.6)
project(ambisonic)

include_directories(include source source/kiss_fft)

list(APPEND HEADERS
    include/Ambisonics.h
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp *.c)

add_library(ambisonic STATIC ${sources})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ambisonic.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
