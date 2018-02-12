cmake_minimum_required(VERSION 3.1)

set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "3")
set(PACKAGE_VERSION_PATCH "0")
project(spatialaudio VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_STATIC_LIBS "Build static library" ON)

include(GNUInstallDirs)

list(APPEND headers
    include/AmbisonicBase.h
    include/AmbisonicDecoderPresets.h
    include/AmbisonicProcessor.h
    include/AmbisonicSpeaker.h
    include/AmbisonicBinauralizer.h
    include/AmbisonicEncoderDist.h
    include/AmbisonicPsychoacousticFilters.h
    include/AmbisonicTypesDefinesCommons.h
    include/SpeakersBinauralizer.h
    include/AmbisonicCommons.h
    include/AmbisonicEncoder.h
    include/Ambisonics.h
    include/AmbisonicZoomer.h
    include/mit_hrtf_filter.h
    include/AmbisonicDecoder.h
    include/AmbisonicMicrophone.h
    include/AmbisonicSource.h
    include/BFormat.h
    include/mit_hrtf_lib.h
    include/hrtf/hrtf.h
    include/hrtf/mit_hrtf.h
    include/hrtf/sofa_hrtf.h
    include/normal/mit_hrtf_normal_44100.h
    include/normal/mit_hrtf_normal_48000.h
    include/normal/mit_hrtf_normal_88200.h
    include/normal/mit_hrtf_normal_96000.h
    source/kiss_fft/kiss_fftr.h
    source/kiss_fft/kiss_fft.h
)

list(APPEND sources
    source/AmbisonicEncoder.cpp
    source/AmbisonicMicrophone.cpp
    source/AmbisonicCommons.cpp
    source/mit_hrtf_lib.c
    source/AmbisonicProcessor.cpp
    source/AmbisonicDecoder.cpp
    source/AmbisonicBinauralizer.cpp
    source/AmbisonicSource.cpp
    source/hrtf/mit_hrtf.cpp
    source/hrtf/sofa_hrtf.cpp
    source/BFormat.cpp
    source/SpeakersBinauralizer.cpp
    source/kiss_fft/kiss_fftr.c
    source/kiss_fft/kiss_fft.c
    source/AmbisonicBase.cpp
    source/AmbisonicSpeaker.cpp
    source/AmbisonicEncoderDist.cpp
    source/AmbisonicZoomer.cpp
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MySofa QUIET)
set(HAVE_MYSOFA ${MYSOFA_FOUND})

include_directories(include include/hrtf source source/kiss_fft ${PROJECT_BINARY_DIR})

if(${MYSOFA_FOUND})
    include_directories(${MYSOFA_INCLUDE_DIRS})
endif(${MYSOFA_FOUND})

if(BUILD_STATIC_LIBS)
    add_library(spatialaudio-static STATIC ${sources})
    if(${MYSOFA_FOUND})
        target_link_libraries(spatialaudio-static ${MYSOFA_LIBRARIES})
    endif(${MYSOFA_FOUND})
    SET_TARGET_PROPERTIES(spatialaudio-static PROPERTIES OUTPUT_NAME spatialaudio CLEAN_DIRECT_OUTPUT 1 POSITION_INDEPENDENT_CODE ON)
    install(TARGETS spatialaudio-static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_STATIC_LIBS)

if(BUILD_SHARED_LIBS)
    add_library(spatialaudio-shared SHARED ${sources})
    if(${MYSOFA_FOUND})
        target_link_libraries(spatialaudio-shared ${MYSOFA_LIBRARIES})
    endif(${MYSOFA_FOUND})
    SET_TARGET_PROPERTIES(spatialaudio-shared PROPERTIES OUTPUT_NAME spatialaudio CLEAN_DIRECT_OUTPUT 1)
    set_property(TARGET spatialaudio-shared PROPERTY VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
    set_property(TARGET spatialaudio-shared PROPERTY SOVERSION ${PACKAGE_VERSION_MAJOR} )
    set_property(TARGET spatialaudio-shared PROPERTY C_VISIBILITY_PRESET hidden)
    install(TARGETS spatialaudio-shared LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_SHARED_LIBS)

option(HAVE_MIT_HRTF "Should MIT HRTF be built-in" ON)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

if(${HAVE_MYSOFA})
    set(MYSOFA_LIB "-L${MYSOFA_ROOT_DIR}/${CMAKE_INSTALL_LIBDIR} -lmysofa")
    set(MYSOFA_INCLUDE "-I${MYSOFA_ROOT_DIR}/include")
endif(${HAVE_MYSOFA})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/spatialaudio.pc.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
    @ONLY
)

install(FILES ${headers} DESTINATION include/spatialaudio)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES ${PROJECT_BINARY_DIR}/config.h DESTINATION include/spatialaudio)

#Tarballs generation
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/build*;/.git*;~$;${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)
