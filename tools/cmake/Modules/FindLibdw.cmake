find_path(LIBDW_INCLUDE_DIR "elfutils/libdw.h"
  HINTS
  $ENV{SIMGRID_LIBDW_LIBRARY_PATH}
  $ENV{LD_LIBRARY_PATH}
  $ENV{LIBDW_LIBRARY_PATH}
  PATH_SUFFIXES include/ GnuWin32/include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)
find_library(LIBDW_LIBRARY
  NAMES dw
  HINTS
  $ENV{SIMGRID_LIBDW_LIBRARY_PATH}
  $ENV{LD_LIBRARY_PATH}
  $ENV{LIBDW_LIBRARY_PATH}
  PATH_SUFFIXES lib/ GnuWin32/lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)
set(LIBDW_LIBRARIES "${LIBDW_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libdw
  DEFAULT_MSG
  LIBDW_LIBRARIES
  LIBDW_INCLUDE_DIR)
mark_as_advanced(LIBDW_INCLUDE_DIR LIBDW_LIBRARIES)
