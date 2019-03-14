IF(SIMGRID_HAVE_LUA)
  # Tests testing simulation from C but using lua for platform files. Executed like this
  # ~$ ./masterslave platform.lua deploy.lua
  ADD_TESH(lua-platform-masterslave                --setenv srcdir=${CMAKE_HOME_DIRECTORY} --setenv bindir=${CMAKE_BINARY_DIR} --cd ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/teshsuite/lua/lua_platforms.tesh)
  SET_TESTS_PROPERTIES(lua-platform-masterslave    PROPERTIES ENVIRONMENT "LUA_CPATH=${CMAKE_BINARY_DIR}/lib/lib?.${LIB_EXE}")
ENDIF()
