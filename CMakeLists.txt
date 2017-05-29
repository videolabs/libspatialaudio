cmake_minimum_required(VERSION 2.6)
project(ambisonic)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE headers LIST_DIRECTORIES false *.h)
file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp *.c)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MySofa REQUIRED)

include_directories(include source source/kiss_fft ${MYSOFA_INCLUDE_DIRS})

add_library(ambisonic STATIC ${sources})

target_link_libraries(ambisonic ${MYSOFA_LIBRARIES})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ambisonic.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(FILES ${headers} DESTINATION include/ambisonic)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
