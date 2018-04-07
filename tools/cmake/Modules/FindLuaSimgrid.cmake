# Search for the Lua 5.3 include files and libraries
#
#  Input variable:
#     LUA_HINT: path to Lua installation -- only needed for non-standard installs
#  Output variable:
#     SIMGRID_HAVE_LUA         : if Lua was found
#     LUA_LIBRARY      : the path to the dynamic library
#     LUA_INCLUDE_DIR  : where to find lua.h
#     LUA_VERSION_MAJOR: First part of the version (often, 5)
#     LUA_VERSION_MINOR: Second part of the version (3 when we have 5.3)

find_path(LUA_INCLUDE_DIR lua.h
  HINTS
    ENV LUA_HINT
  PATH_SUFFIXES include/lua53 include/lua5.3 include/lua-5.3 include/lua include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(LUA_LIBRARY
  NAMES # I don't want a lua.a
    lua53.so     lua5.3.so     lua-5.3.so
    lua53.dynlib lua5.3.dynlib lua-5.3.dynlib
    lua53.dll    lua5.3.dll    lua-5.3.dll
    lua.so lua.dynlib lua.dll
    lua53 lua5.3 lua-5.3 lua
  HINTS
    ENV LUA_HINT
  PATH_SUFFIXES lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (NOT LUA_LIBRARY)
  message(FATAL_ERROR "Error: Lua library not found. Please install that package (and set LUA_HINT) or disable Lua.")
endif()
if (NOT LUA_INCLUDE_DIR OR NOT EXISTS "${LUA_INCLUDE_DIR}/lua.h")
  message(FATAL_ERROR "Error: Lua header file not found. Please install that package (and set LUA_HINT) or disable Lua.")
endif()

# Extract the version info out of the header file
file(STRINGS "${LUA_INCLUDE_DIR}/lua.h" lua_version_str REGEX "^#define[ \t]+LUA_VERSION_MAJOR[ \t]+\"[456]+\"")
  string(REGEX REPLACE "^#define[ \t]+LUA_VERSION_MAJOR[ \t]+\"([^\"]+)\"" "\\1" LUA_VERSION_MAJOR "${lua_version_str}")
file(STRINGS "${LUA_INCLUDE_DIR}/lua.h" lua_version_str REGEX "^#define[ \t]+LUA_VERSION_MINOR[ \t]+\"[0123456789]+\"")
  string(REGEX REPLACE "^#define[ \t]+LUA_VERSION_MINOR[ \t]+\"([^\"]+)\"" "\\1" LUA_VERSION_MINOR "${lua_version_str}")
unset(lua_version_str)

# Check that we have a sufficient version of Lua
if(LUA_VERSION_MAJOR EQUAL 5 AND LUA_VERSION_MINOR EQUAL 3)
  set(SIMGRID_HAVE_LUA 1)
  include_directories(${LUA_INCLUDE_DIR})
else()
  message(FATAL_ERROR "Error: Lua version 5.3 is required, but version ${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR} found instead.")
endif()

message(STATUS "Lua version: ${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
message(STATUS "Lua library: ${LUA_LIBRARY}")
mark_as_advanced(LUA_INCLUDE_DIR)
mark_as_advanced(LUA_LIBRARY)
