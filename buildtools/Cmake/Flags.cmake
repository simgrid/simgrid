set(warnCFLAGS "")
set(optCFLAGS "")

if(NOT __VISUALC__ AND NOT __BORLANDC__)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-g3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-g3")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}/Zi")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}/Zi")
endif()

if(enable_compile_warnings)
  set(warnCFLAGS "-fno-common -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wclobbered -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror -Wno-error=clobbered ")
  if(COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.5")
    set(warnCFLAGS "${warnCFLAGS}-Wno-error=unused-but-set-variable ")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    string(REPLACE "-Wclobbered " "" warnCFLAGS "${warnCFLAGS}")
  endif()

  set(CMAKE_Fortran_FLAGS "-Wall") # FIXME: Q&D hack

  set(CMAKE_JAVA_COMPILE_FLAGS "-Xlint")
endif()

if(enable_compile_optimizations)
  set(optCFLAGS "-O3 -finline-functions -funroll-loops -fno-strict-aliasing ")
  if(COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.5")
    set(optCFLAGS "${optCFLAGS}-flto ")
  endif()
else()
  set(optCFLAGS "-O0 ")
endif()

if(APPLE AND COMPILER_C_VERSION_MAJOR_MINOR MATCHES "4.6")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
  set(optCFLAGS "-O0 ")
endif()

if(WIN32) # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50293
  if (COMPILER_C_VERSION_MAJOR_MINOR MATCHES "4.7" OR 
      COMPILER_C_VERSION_MAJOR_MINOR MATCHES "4.6")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --disable-lto")
endif()

if(NOT enable_debug)
  set(CMAKE_C_FLAGS "-DNDEBUG ${CMAKE_C_FLAGS}")
endif()

if(enable_msg_deprecated)
  set(CMAKE_C_FLAGS "-DMSG_USE_DEPRECATED ${CMAKE_C_FLAGS}")
endif()

set(CMAKE_C_FLAGS "${optCFLAGS}${warnCFLAGS}${CMAKE_C_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${optCFLAGS}")

# Try to make Mac a bit more complient to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE")
endif()

set(TESH_OPTION "")
if(enable_coverage)
  find_program(GCOV_PATH gcov)
  if(GCOV_PATH)
    SET(COVERAGE_COMMAND "${GCOV_PATH}" CACHE TYPE FILEPATH FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCOVERAGE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(TESH_OPTION --enable-coverage)
    add_definitions(-fprofile-arcs -ftest-coverage)
  endif()
endif()

if(NOT $ENV{CFLAGS} STREQUAL "")
  message(STATUS "Add CFLAGS: \"$ENV{CFLAGS}\" to CMAKE_C_FLAGS")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} $ENV{CFLAGS}")
endif()

if(NOT $ENV{LDFLAGS} STREQUAL "")
  message(STATUS "Add LDFLAGS: \"$ENV{LDFLAGS}\" to CMAKE_C_LINK_FLAGS")
  set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} $ENV{LDFLAGS}")
endif()

if(enable_model-checking AND enable_compile_optimizations)
  message(WARNING "Sorry for now GCC optimizations does not work with model checking.\nPlease turn off optimizations with command:\ncmake -Denable_compile_optimizations=off .")
endif()
