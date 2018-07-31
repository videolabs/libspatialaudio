prefix=@CMAKE_INSTALL_PREFIX@
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/include

Name: libspatialaudio
Description: Spatial audio rendering library
Version: @PACKAGE_VERSION_MAJOR@.@PACKAGE_VERSION_MINOR@.@PACKAGE_VERSION_PATCH@
Libs: -L${libdir} -lspatialaudio @MYSOFA_LIB@ -lm -lz
Cflags: -I${includedir} @MYSOFA_INCLUDE@
