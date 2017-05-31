cmake_minimum_required(VERSION 2.9)
project(ambisonic)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE headers LIST_DIRECTORIES false *.h)
file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp *.c)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MySofa)

include_directories(include include/hrtf source source/kiss_fft ${PROJECT_BINARY_DIR} ${MYSOFA_INCLUDE_DIRS})

add_library(ambisonic STATIC ${sources})

target_link_libraries(ambisonic ${MYSOFA_LIBRARIES})

set(HAVE_MYSOFA ${MYSOFA_FOUND})
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

if(${HAVE_MYSOFA})
    set(MYSOFA_LIB "-L${MYSOFA_ROOT_DIR}/lib -l:libmysofa.a")
    set(MYSOFA_INCLUDE "-I${MYSOFA_ROOT_DIR}/include")
endif(${HAVE_MYSOFA})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ambisonic.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(FILES ${headers} DESTINATION include/ambisonic)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
install(FILES ${PROJECT_BINARY_DIR}/config.h DESTINATION include/ambisonic)
