cmake_minimum_required(VERSION 3.1)

set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "3")
set(PACKAGE_VERSION_PATCH "1")
project(spatialaudio VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")

set(LIBRARY_SOVERSION_MAJOR "1")
set(LIBRARY_SOVERSION_MINOR "0")
set(LIBRARY_SOVERSION_PATCH "0")

set(CMAKE_CXX_STANDARD 14)
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
    include/AmbisonicShelfFilters.h
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
    include/AdmDecorrelate.h
    include/AdmLayouts.h
    include/AdmMetadata.h
    include/AdmPointSourcePanner.h
    include/AdmRenderer.h
    include/AdmUtils.h
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
    source/libear/kissfft/kissfft.hh
    source/libear/src/direct_speakers/mapping_rules.hpp
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
    source/AmbisonicShelfFilters.cpp
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
    source/AdmDecorrelate.cpp
    source/AdmPointSourcePanner.cpp
    source/AdmRenderer.cpp
    source/libear/decorrelate.cpp
    source/libear/decorrelate.hpp
    source/libear/src/direct_speakers/mapping_rules.cpp
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MySofa QUIET)
set(HAVE_MYSOFA ${MYSOFA_FOUND})

include_directories(include include/hrtf source source/kiss_fft source/libear ${PROJECT_BINARY_DIR})

if(MYSOFA_FOUND)
    include_directories(${MYSOFA_INCLUDE_DIRS})
endif(MYSOFA_FOUND)

if(BUILD_STATIC_LIBS)
    add_library(spatialaudio-static STATIC ${sources})
    if(MYSOFA_FOUND)
        target_link_libraries(spatialaudio-static ${MYSOFA_LIBRARIES})
    endif(MYSOFA_FOUND)
    SET_TARGET_PROPERTIES(spatialaudio-static PROPERTIES OUTPUT_NAME spatialaudio CLEAN_DIRECT_OUTPUT 1 POSITION_INDEPENDENT_CODE ON)
    install(TARGETS spatialaudio-static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_STATIC_LIBS)

if(BUILD_SHARED_LIBS)
    add_library(spatialaudio-shared SHARED ${sources})
    if(MYSOFA_FOUND)
        target_link_libraries(spatialaudio-shared ${MYSOFA_LIBRARIES})
    endif(MYSOFA_FOUND)
    SET_TARGET_PROPERTIES(spatialaudio-shared PROPERTIES OUTPUT_NAME spatialaudio CLEAN_DIRECT_OUTPUT 1)
    set_property(TARGET spatialaudio-shared PROPERTY VERSION "${LIBRARY_SOVERSION_MAJOR}.${LIBRARY_SOVERSION_MINOR}.${LIBRARY_SOVERSION_PATCH}")
    set_property(TARGET spatialaudio-shared PROPERTY SOVERSION ${LIBRARY_SOVERSION_MAJOR} )
    set_property(TARGET spatialaudio-shared PROPERTY C_VISIBILITY_PRESET hidden)
    install(TARGETS spatialaudio-shared LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_SHARED_LIBS)

option(HAVE_MIT_HRTF "Should MIT HRTF be built-in" ON)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

if(MYSOFA_FOUND)
    get_filename_component(MYSOFA_LIB_DIR "${MYSOFA_LIBRARIES}" DIRECTORY)
    set(MYSOFA_LIB "-L${MYSOFA_LIB_DIR} -lmysofa")
    set(MYSOFA_INCLUDE "-I${MYSOFA_INCLUDE_DIRS}")
endif(MYSOFA_FOUND)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/spatialaudio.pc.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
    @ONLY
)

install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/spatialaudio)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES ${PROJECT_BINARY_DIR}/config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/spatialaudio)

#Tarballs generation
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/build*;/.git*;~$;${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)
