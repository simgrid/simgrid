##
## This file is in charge of setting our paranoid flags with regard to warnings and optimization.
##
##   These flags do break some classical CMake tests, so you don't
##   want to do so before the very end of the configuration.
## 
##   Other compiler flags (C/C++ standard version) are tested and set
##   by the beginning of the configuration, directly in ~/CMakeList.txt

set(warnCFLAGS "")
set(optCFLAGS "")



if(enable_compile_warnings)
  set(warnCFLAGS "-fno-common -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")
  if(CMAKE_COMPILER_IS_GNUCC)
    set(warnCFLAGS "${warnCFLAGS}-Wclobbered -Wno-error=clobbered ")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra  -Wunused -Wpointer-arith -Wchar-subscripts -Wcomment  -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror")
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wclobbered -Wno-error=clobbered")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang") # don't care about class that become struct
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-mismatched-tags")
  endif()

  set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wall")
  set(CMAKE_JAVA_COMPILE_FLAGS "-Xlint")
endif()

# Se the optimisation flags
# NOTE, we should CMAKE_BUILD_TYPE for this
if(enable_compile_optimizations)
  set(optCFLAGS "-O3 -funroll-loops -fno-strict-aliasing ")
else()
  set(optCFLAGS "-O0 ")
endif()
if(enable_compile_optimizations AND CMAKE_COMPILER_IS_GNUCC
    AND (NOT enable_model-checking))
  # This is redundant (already in -03):
  set(optCFLAGS "${optCFLAGS} -finline-functions ")
endif()

# Configure LTO
# TODO, provide an option to manually choose whether to use LTO
# NOTE, cmake 3.0 has a INTERPROCEDURAL_OPTIMIZATION target
#       property for this (http://www.cmake.org/cmake/help/v3.0/prop_tgt/INTERPROCEDURAL_OPTIMIZATION.html)
set(enable_lto OFF)
if(enable_compile_optimizations
    AND CMAKE_COMPILER_IS_GNUCC
    AND (NOT enable_model-checking))
  if(WIN32)
    if (COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.8")
      # On windows, we need 4.8 or higher to enable lto because of http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50293
      #
      # We are experiencing assertion failures even with 4.8 on MinGW.
      # Push the support forward: will see if 4.9 works when we test it.
      set(enable_lto ON)
    endif()
  elseif(LINKER_VERSION STRGREATER "2.22")
    set(enable_lto ON)
  endif()
endif()
if(enable_lto)
  set(optCFLAGS "${optCFLAGS} -flto ")
  # See https://gcc.gnu.org/wiki/LinkTimeOptimizationFAQ#ar.2C_nm_and_ranlib:
  # "Since version 4.9 gcc produces slim object files that only contain
  # the intermediate representation. In order to handle archives of
  # these objects you have to use the gcc wrappers:
  # gcc-ar, gcc-nm and gcc-ranlib."
  if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU"
      AND COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.8")
    set (CMAKE_AR gcc-ar)
    set (CMAKE_RANLIB gcc-ranlib)
  endif()
endif()

if(enable_model-checking AND enable_compile_optimizations)
  # Forget it, do not optimize the code (because it confuses the MC):
  set(optCFLAGS "-O0 ")
  # But you can still optimize this:
  foreach(s
      src/xbt/mmalloc/mm.c
      src/xbt/log.c src/xbt/xbt_log_appender_file.c
      src/xbt/xbt_log_layout_format.c src/xbt/xbt_log_layout_simple.c
      src/xbt/dict.c src/xbt/dict_elm.c src/xbt/dict_multi.c src/xbt/dict_cursor.c
      src/xbt/set.c src/xbt/setset.c
      src/xbt/dynar.c src/xbt/fifo.c src/xbt/heap.c src/xbt/swag.c
      src/xbt/str.c src/xbt/strbuff.c src/xbt/snprintf.c
      src/xbt/queue.c
      src/xbt/xbt_os_time.c src/xbt/xbt_os_thread.c
      src/xbt/sha.c
      src/xbt/matrix.c
      src/xbt/backtrace_linux.c
      ${MC_SRC_BASE} ${MC_SRC})
      set (mcCFLAGS "-O3  -funroll-loops -fno-strict-aliasing")
       if(CMAKE_COMPILER_IS_GNUCC)
         set (mcCFLAGS "${mcCFLAGS} -finline-functions")
      endif()
      set_source_files_properties(${s} PROPERTIES COMPILE_FLAGS ${mcCFLAGS})
  endforeach()
endif()

if(NOT enable_debug)
  set(CMAKE_C_FLAGS "-DNDEBUG ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-DNDEBUG ${CMAKE_CXX_FLAGS}")
endif()

if(enable_msg_deprecated)
  set(CMAKE_C_FLAGS "-DMSG_USE_DEPRECATED ${CMAKE_C_FLAGS}")
endif()

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${optCFLAGS} ${warnCFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${optCFLAGS}")

# Try to make Mac a bit more complient to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE")
endif()

set(TESH_OPTION "")
if(enable_coverage)
  find_program(GCOV_PATH gcov)
  if(GCOV_PATH)
    set(COVERAGE_COMMAND "${GCOV_PATH}" CACHE TYPE FILEPATH FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCOVERAGE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
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

if(NOT $ENV{CXXFLAGS} STREQUAL "")
  message(STATUS "Add CXXFLAGS: \"$ENV{CXXFLAGS}\" to CMAKE_CXX_FLAGS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{CXXFLAGS}")
endif()

if(NOT $ENV{LDFLAGS} STREQUAL "")
  message(STATUS "Add LDFLAGS: \"$ENV{LDFLAGS}\" to CMAKE_C_LINK_FLAGS")
  set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} $ENV{LDFLAGS}")
endif()

if(MINGW)
  add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()
