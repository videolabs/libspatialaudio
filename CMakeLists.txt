cmake_minimum_required(VERSION 3.1)
project(spatialaudio)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE headers LIST_DIRECTORIES false *.h)
file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp *.c)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MySofa QUIET)
set(HAVE_MYSOFA ${MYSOFA_FOUND})

include_directories(include include/hrtf source source/kiss_fft ${PROJECT_BINARY_DIR})

if(${MYSOFA_FOUND})
    include_directories(${MYSOFA_INCLUDE_DIRS})
endif(${MYSOFA_FOUND})

add_library(spatialaudio STATIC ${sources})

if(${MYSOFA_FOUND})
    target_link_libraries(spatialaudio ${MYSOFA_LIBRARIES})
endif(${MYSOFA_FOUND})

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
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/spatialaudio.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(FILES ${headers} DESTINATION include/spatialaudio)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
install(FILES ${PROJECT_BINARY_DIR}/config.h DESTINATION include/spatialaudio)

