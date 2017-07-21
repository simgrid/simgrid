find_path(LIBELF_INCLUDE_DIR "libelf.h"
  HINTS
  $ENV{SIMGRID_LIBELF_LIBRARY_PATH}
  $ENV{LD_LIBRARY_PATH}
  $ENV{LIBELF_LIBRARY_PATH}
  PATH_SUFFIXES include/ GnuWin32/include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)
find_library(LIBELF_LIBRARY
  NAMES elf
  HINTS
  $ENV{SIMGRID_LIBELF_LIBRARY_PATH}
  $ENV{LD_LIBRARY_PATH}
  $ENV{LIBELF_LIBRARY_PATH}
  PATH_SUFFIXES lib/ GnuWin32/lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)
set(LIBELF_LIBRARIES "${LIBELF_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libelf
  DEFAULT_MSG
  LIBELF_LIBRARIES
  LIBELF_INCLUDE_DIR)
mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARIES)
