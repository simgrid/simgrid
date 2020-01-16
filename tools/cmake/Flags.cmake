##
## This file is in charge of setting our paranoid flags with regard to warnings and optimization.
##
##   It is only used for gcc and clang. MSVC builds don't load this file.
##
##   These flags do break some classical CMake tests, so you don't
##   want to do so before the very end of the configuration.
##
##   Other compiler flags (C/C++ standard version) are tested and set
##   by the beginning of the configuration, directly in ~/CMakeList.txt

set(warnCFLAGS "")
set(optCFLAGS "")
set(warnCXXFLAGS "")

if(enable_compile_warnings)
  set(warnCFLAGS "-fno-common -Wall -Wunused -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing")
  if(CMAKE_COMPILER_IS_GNUCC AND (NOT (CMAKE_C_COMPILER_VERSION VERSION_LESS "5.0")))
    set(warnCFLAGS "${warnCFLAGS} -Wformat-signedness")
  endif()
  if(CMAKE_COMPILER_IS_GNUCC)
    set(warnCFLAGS "${warnCFLAGS} -Wclobbered -Wno-error=clobbered  -Wno-unused-local-typedefs -Wno-error=attributes -Wno-error=maybe-uninitialized")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    # ignore remark  #1418: external function definition with no prior declaration
    # 2196: routine is both "inline" and "noinline"
    # 3179: deprecated conversion of string literal to char* (should be const char*)
    # 191: type qualifier is meaningless on cast type
    # 597: entity-kind "entity" will not be called for implicit or explicit conversions
    # 2330: argument of type "type" is incompatible with parameter of type "type" (dropping qualifiers)
    set(warnCFLAGS "${warnCFLAGS} -wd1418 -wd191 -wd2196 -wd3179 -ww597 -ww2330")
  endif()

  set(warnCXXFLAGS "${warnCFLAGS} -Wall -Wextra -Wunused -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing")
  if(CMAKE_COMPILER_IS_GNUCXX AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")))
    set(warnCFLAGS "${warnCFLAGS} -Wformat-signedness")
  endif()
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(warnCXXFLAGS "${warnCXXFLAGS} -Wclobbered -Wno-error=clobbered  -Wno-unused-local-typedefs -Wno-error=attributes -Wno-error=maybe-uninitialized")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # don't care about class that become struct, avoid issue of empty C structs
    # size (coming from libunwind.h)
    set(warnCXXFLAGS "${warnCXXFLAGS} -Wno-mismatched-tags -Wno-extern-c-compat")
  endif()

  # the one specific to C but refused by C++
  set(warnCFLAGS "${warnCFLAGS} -Wmissing-prototypes")

  if(CMAKE_Fortran_COMPILER_ID MATCHES "GCC|PGI")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wall")
  elseif(CMAKE_Fortran_COMPILER_ID MATCHES "Flang")
    # flang >= 7 has a bug with common and debug flags. Ignore cmake-added -g in this case.
    # https://github.com/flang-compiler/flang/issues/671
    set(CMAKE_Fortran_FLAGS "-Wall")
  elseif(CMAKE_Fortran_COMPILER_ID MATCHES "Intel")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -warn all")
  endif()
  set(CMAKE_JAVA_COMPILE_FLAGS "-Xlint")
endif()

# NDEBUG gives a lot of "initialized but unused variables" errors. Don't die anyway.
if(enable_compile_warnings AND enable_debug)
  set(warnCFLAGS "${warnCFLAGS} -Werror")
  set(warnCXXFLAGS "${warnCXXFLAGS} -Werror")
  if(CMAKE_Fortran_COMPILER_ID MATCHES "GCC")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Werror -Werror=format-security")
  endif()
endif()

# Activate the warnings on #if FOOBAR when FOOBAR has no value
# It breaks on FreeBSD within Boost headers, so activate this only in Pure Hardcore debug mode.
if(enable_maintainer_mode)
  set(warnCFLAGS "${warnCFLAGS} -Wundef")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wundef")
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

# Do not leak the current directory into the binaries
if(CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND realpath --relative-to=${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE RESULT OUTPUT_VARIABLE RELATIVE_SOURCE_DIR ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(RESULT EQUAL 0)
    message(STATUS "Relative source directory is \"${RELATIVE_SOURCE_DIR}\".")
  else()
    message(WARNING "Failed to find relative source directory. Using \".\".")
    set(RELATIVE_SOURCE_DIR ".")
  endif()
  if (CMAKE_C_COMPILER_VERSION VERSION_LESS "8.0")
    set(optCFLAGS "${optCFLAGS} -fdebug-prefix-map=${CMAKE_SOURCE_DIR}=${RELATIVE_SOURCE_DIR}")
  else()
    set(optCFLAGS "${optCFLAGS} -ffile-prefix-map=${CMAKE_SOURCE_DIR}=${RELATIVE_SOURCE_DIR}")
  endif()
endif()

# Configure LTO
# NOTE, cmake 3.0 has a INTERPROCEDURAL_OPTIMIZATION target
#       property for this (http://www.cmake.org/cmake/help/v3.0/prop_tgt/INTERPROCEDURAL_OPTIMIZATION.html)
if(enable_lto) # User wants LTO. Try if we can do that
  set(enable_lto OFF)
  if(enable_compile_optimizations
      AND CMAKE_COMPILER_IS_GNUCC
      AND (NOT enable_model-checking))
    # On windows, we need 4.8 or higher to enable lto because of http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50293
    #   We are experiencing assertion failures even with 4.8 on MinGW.
    #   Push the support forward: will see if 4.9 works when we test it.
    #
    # On Linux, we got the following with GCC 4.8.4 on Centos and Ubuntu
    #    lto1: internal compiler error: in output_die, at dwarf2out.c:8478
    #    Please submit a full bug report, with preprocessed source if appropriate.
    # So instead, we push the support forward

    if ( (CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.8.5")
         AND (LINKER_VERSION VERSION_GREATER "2.22"))
      set(enable_lto ON)
    endif()
  endif()
  if(enable_lto)
    message(STATUS "LTO seems usable.")
  else()
    if(NOT enable_compile_optimizations)
      message(STATUS "LTO disabled: Compile-time optimizations turned off.")
    else()
      if(enable_model-checking)
        message(STATUS "LTO disabled when compiling with model-checking.")
      else()
        message(STATUS "LTO does not seem usable -- try updating your build chain.")
      endif()
    endif()
  endif()
else()
  message(STATUS "LTO disabled on the command line.")
endif()
if(enable_lto) # User wants LTO, and it seems usable. Go for it
  set(optCFLAGS "${optCFLAGS} -flto ")
  # See https://gcc.gnu.org/wiki/LinkTimeOptimizationFAQ#ar.2C_nm_and_ranlib:
  # "Since version 4.9 gcc produces slim object files that only contain
  # the intermediate representation. In order to handle archives of
  # these objects you have to use the gcc wrappers:
  # gcc-ar, gcc-nm and gcc-ranlib."
  if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU"
      AND CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.8")
    set (CMAKE_AR gcc-ar)
    set (CMAKE_RANLIB gcc-ranlib)
  endif()
endif()

if(enable_model-checking AND enable_compile_optimizations)
  # Forget it, do not optimize the code (because it confuses the MC):
  set(optCFLAGS "-O0 ")
  # But you can still optimize this:
  foreach(s
      src/simix/popping.cpp src/simix/popping_generated.cpp src/simix/smx_global.cpp
      ${SURF_SRC} ${TRACING_SRC} ${XBT_SRC}
      ${MC_SRC_BASE} ${MC_SRC})
      set (mcCFLAGS "-O3  -funroll-loops -fno-strict-aliasing")
       if(CMAKE_COMPILER_IS_GNUCC)
         set (mcCFLAGS "${mcCFLAGS} -finline-functions")
      endif()
      set_source_files_properties(${s} PROPERTIES COMPILE_FLAGS ${mcCFLAGS})
  endforeach()
endif()

if (CMAKE_C_COMPILER_ID MATCHES "Intel")
  # honor parentheses when determining the order of expression evaluation.
  set(optCFLAGS "${optCFLAGS} -fprotect-parens ")
endif()

if(NOT enable_debug)
  set(CMAKE_C_FLAGS "-DNDEBUG ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-DNDEBUG ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_FLAGS   "${warnCFLAGS} ${CMAKE_C_FLAGS}   ${optCFLAGS}")
set(CMAKE_CXX_FLAGS "${warnCXXFLAGS} ${CMAKE_CXX_FLAGS} ${optCFLAGS}")

# Try to make Mac a bit more complient to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE")
endif()

# Avoid a failure seen with gcc 7.2.0 and ns3 3.27
if(enable_ns3)
  set_source_files_properties(src/surf/network_ns3.cpp PROPERTIES COMPILE_FLAGS " -Wno-unused-local-typedef")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOVERAGE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    add_definitions(-fprofile-arcs -ftest-coverage)
  endif()
endif()

if(enable_address_sanitizer)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -fsanitize=address")
    set(TESH_OPTION --enable-sanitizers)
    try_compile(HAVE_SANITIZER_ADDRESS ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_asan.cpp)
    try_compile(HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_asan.cpp
      COMPILE_DEFINITIONS -DCHECK_FIBER_SUPPORT)
else()
    set(HAVE_SANITIZER_ADDRESS FALSE CACHE INTERNAL "")
    set(HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT FALSE CACHE INTERNAL "")
endif()

if(enable_thread_sanitizer)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -no-pie")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -no-pie")
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -fsanitize=thread -no-pie")
    try_compile(HAVE_SANITIZER_THREAD ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_tsan.cpp)
    try_compile(HAVE_SANITIZER_THREAD_FIBER_SUPPORT ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_tsan.cpp
      COMPILE_DEFINITIONS -DCHECK_FIBER_SUPPORT)
else()
    set(HAVE_SANITIZER_THREAD FALSE CACHE INTERNAL "")
    set(HAVE_SANITIZER_THREAD_FIBER_SUPPORT FALSE CACHE INTERNAL "")
endif()

if(enable_undefined_sanitizer)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -fsanitize=undefined")
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
  # http://stackoverflow.com/questions/10452262/create-64-bit-jni-under-windows
  # We don't want to ship libgcc_s_seh-1.dll nor libstdc++-6.dll
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -static-libgcc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS   "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc")
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++")

  # JNI searches for stdcalls
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wl,--add-stdcall-alias")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--add-stdcall-alias")
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -Wl,--add-stdcall-alias")
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -Wl,--add-stdcall-alias")

  # Specify the data model that we are using (yeah it may help Java)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  else()
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -m64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  endif()
endif()
