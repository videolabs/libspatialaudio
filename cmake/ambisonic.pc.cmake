prefix=@CMAKE_INSTALL_PREFIX@
prefix_libmysofa=@MYSOFA_ROOT_DIR@
libdir=${prefix}/lib
includedir=${prefix}/include

Name: ambisonic-lib
Description: Ambisonic decoding library
Version: 0.0.1
Libs: -L${libdir} -lambisonic @MYSOFA_LIB@ -lm -lz
Cflags: -I${includedir} @MYSOFA_INCLUDE@
