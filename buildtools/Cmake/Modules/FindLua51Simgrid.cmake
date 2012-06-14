find_path(HAVE_LUA_H lua.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lua/ lua/5.1/ include/ include/lua5.1/ include/lua51 include/lua
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_library(HAVE_LUA51_LIB
    NAME lua.5.1 lua5.1 lua51 lua-5.1 lua
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib64 lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_program(HAVE_LUA_BIN NAMES lua)

set(LIB_LUA_NAME ${HAVE_LUA51_LIB})
string(REGEX MATCH "liblua.*$" LIB_LUA_NAME "${LIB_LUA_NAME}")
string(REPLACE ".${LIB_EXE}" "" LIB_LUA_NAME "${LIB_LUA_NAME}")
string(REPLACE "lib" "" LIB_LUA_NAME "${LIB_LUA_NAME}")

if(HAVE_LUA_H AND HAVE_LUA51_LIB AND LIB_LUA_NAME AND HAVE_LUA_BIN)
 set(LUA51_FOUND 1)
 string(REPLACE "/lua.h" "" LUA_INCLUDE_DIR ${HAVE_LUA_H})
 string(REPLACE "/lib${LIB_LUA_NAME}.${LIB_EXE}" "" LUA_LIBRARY_DIR ${HAVE_LUA51_LIB})
endif(HAVE_LUA_H AND HAVE_LUA51_LIB AND LIB_LUA_NAME AND HAVE_LUA_BIN)

mark_as_advanced(LUA_LIB_PATH_1)                                                                                                                      
mark_as_advanced(LUA_LIB_PATH_2)
mark_as_advanced(LUA_LIB_PATH_3)
mark_as_advanced(HAVE_LUA5_1_LAUXLIB_H)                        
mark_as_advanced(HAVE_LUA5_1_LUALIB_H)
mark_as_advanced(HAVE_LUA51_LIB)                                                                                                                                                                             
mark_as_advanced(HAVE_LUA_H)
mark_as_advanced(HAVE_LUA_BIN)

message(STATUS "Looking for lua.h")
if(HAVE_LUA_H)
message(STATUS "Looking for lua.h - found")
else(HAVE_LUA_H)
message(STATUS "Looking for lua.h - not found")
endif(HAVE_LUA_H)

message(STATUS "Looking for lib lua")
if(HAVE_LUA51_LIB)
message(STATUS "Looking for lib lua - found")
message(STATUS "Lib lua version: ${LIB_LUA_NAME}")
else(HAVE_LUA51_LIB)
message(STATUS "Looking for lib lua - not found")
endif(HAVE_LUA51_LIB)

if(HAVE_LUA_BIN)
message(STATUS "Found Lua: ${HAVE_LUA_BIN}")
endif(HAVE_LUA_BIN)

if(LUA51_FOUND)
  set(HAVE_LUA 1)
  include_directories(${LUA_INCLUDE_DIR})
  link_directories(${LUA_LIBRARY_DIR})
else(LUA51_FOUND)
	message(STATUS "Warning : Lua need version 5.1")
endif(LUA51_FOUND)
