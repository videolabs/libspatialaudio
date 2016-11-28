prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@INCLUDE_INSTALL_DIR@

Name: ambisonic-lib
Description: Ambisonic decoding library
Requires: 
Libs: -L${libdir} -lambisonic
Cflags: -I${includedir}/ambisonic
