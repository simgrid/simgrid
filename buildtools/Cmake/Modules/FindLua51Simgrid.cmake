find_path(HAVE_LUA_H lua.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lua/ lua/5.1/ include/ include/lua5.1/
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_library(HAVE_LUA51_LIB
    NAME lua.5.1 lua5.1
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

set(LIB_LUA_NAME ${HAVE_LUA51_LIB})
string(REGEX MATCH "liblua.*$" LIB_LUA_NAME "${LIB_LUA_NAME}")
string(REPLACE ".${LIB_EXE}" "" LIB_LUA_NAME "${LIB_LUA_NAME}")
string(REPLACE "lib" "" LIB_LUA_NAME "${LIB_LUA_NAME}")

if(HAVE_LUA_H AND HAVE_LUA51_LIB AND LIB_LUA_NAME)
 set(LUA51_FOUND 1)
 string(REPLACE "/lua.h" "" LUA_INCLUDE_DIR ${HAVE_LUA_H})
 string(REPLACE "/lib${LIB_LUA_NAME}.${LIB_EXE}" "" LUA_LIBRARY_DIR ${HAVE_LUA51_LIB})
endif(HAVE_LUA_H AND HAVE_LUA51_LIB AND LIB_LUA_NAME)