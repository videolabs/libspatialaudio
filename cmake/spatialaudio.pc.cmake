prefix=@CMAKE_INSTALL_PREFIX@
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/include

Name: libspatialaudio
Description: Spatial audio rendering library
Version: 0.3.0
Libs: -L${libdir} -lspatialaudio @MYSOFA_LIB@ -lm -lz
Cflags: -I${includedir} @MYSOFA_INCLUDE@
