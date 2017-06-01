prefix=@CMAKE_INSTALL_PREFIX@
prefix_libmysofa=@MYSOFA_ROOT_DIR@
libdir=${prefix}/lib
includedir=${prefix}/include

Name: libspatialaudio
Description: Spatial audio rendering library
Version: 0.0.1
Libs: -L${libdir} -lspatialaudio @MYSOFA_LIB@ -lm -lz
Cflags: -I${includedir} @MYSOFA_INCLUDE@
