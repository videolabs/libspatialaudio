# Try to find libMySofa headers and library.
#
# Usage of this module as follows:
#
#     find_package(MySofa)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  MYSOFA_ROOT_DIR           Set this variable to the root installation of
#                            libMySofa if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  MYSOFA_FOUND               System has libMySofa library/headers.
#  MYSOFA_LIBRARIES           The libMySofa library.
#  MYSOFA_INCLUDE_DIRS        The location of libMySofa headers.

find_path(MYSOFA_ROOT_DIR
    NAMES include/mysofa.h
)

find_library(MYSOFA_LIBRARIES
    NAMES libmysofa.a mysofa
    HINTS ${MYSOFA_ROOT_DIR}/lib
)

find_path(MYSOFA_INCLUDE_DIRS
    NAMES mysofa.h
    HINTS ${MYSOFA_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySofa DEFAULT_MSG
    MYSOFA_LIBRARIES
    MYSOFA_INCLUDE_DIRS
)

mark_as_advanced(
    MYSOFA_LIBRARIES
    MYSOFA_INCLUDE_DIRS
)
