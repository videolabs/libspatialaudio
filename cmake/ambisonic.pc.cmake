prefix=@CMAKE_INSTALL_PREFIX@
prefix_libmysofa=@MYSOFA_ROOT_DIR@
libdir=${prefix}/lib
libdir_mysofa=${prefix_libmysofa}/lib
includedir=${prefix}/include

Name: ambisonic-lib
Description: Ambisonic decoding library
Version: 0.0.1
Libs: -L${libdir} -L${libdir_mysofa} -lambisonic -lmysofa
Cflags: -I${includedir}
