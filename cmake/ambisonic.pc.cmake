prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: ambisonic-lib
Description: Ambisonic decoding library
Version: 0.0.1
Libs: -L${libdir} -lambisonic
Cflags: -I${includedir}
