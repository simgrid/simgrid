##
## This file is in charge of setting our paranoid flags with regard to warnings and optimization.
##
##   It is only used for gcc, clang and Intel compilers.
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
  set(warnCFLAGS "-fno-common -Wall -Wextra -Wunused -Wmissing-declarations -Wpointer-arith -Wwrite-strings -Wno-unused-function -Wno-unused-local-typedefs -Wno-unused-parameter -Wno-strict-aliasing")

  if (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    # ignore remarks:
    # 191: type qualifier is meaningless on cast type
    # 1418: external function definition with no prior declaration
    # 2196: routine is both "inline" and "noinline"
    # 2651: attribute does not apply to any entity
    # 3179: deprecated conversion of string literal to char* (should be const char*)
    # set as warning:
    # 597: entity-kind "entity" will not be called for implicit or explicit conversions
    # 2330: argument of type "type" is incompatible with parameter of type "type" (dropping qualifiers)
    # 11003: no IR in object file xxxx; was the source file compiled with xxxx
    set(warnCFLAGS "${warnCFLAGS} -diag-disable=191,1418,2196,2651,3179 -diag-warning=597,2330,11003")
  endif()
  set(warnCXXFLAGS "${warnCFLAGS}")

  if(CMAKE_COMPILER_IS_GNUCC)
    set(warnCFLAGS "${warnCFLAGS} -Wclobbered -Wformat-signedness -Wno-error=clobbered -Wno-error=attributes -Wno-error=maybe-uninitialized")
  endif()

  if(CMAKE_COMPILER_IS_GNUCXX)
    set(warnCXXFLAGS "${warnCXXFLAGS} -Wclobbered -Wformat-signedness -Wno-error=clobbered -Wno-free-nonheap-object -Wno-unused-local-typedefs -Wno-error=attributes -Wno-error=maybe-uninitialized")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0")
      # workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81767
      set(warnCXXFLAGS "${warnCXXFLAGS} -Wno-error=unused-variable")
    endif()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "13.2.0")
      # workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101361
      set(warnCXXFLAGS "${warnCXXFLAGS} -Wno-error=stringop-overread -Wno-error=stringop-overflow")
    endif()
  endif()

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # don't care about class that become struct, avoid issue of empty C structs
    # size (coming from libunwind.h)
    set(warnCXXFLAGS "${warnCXXFLAGS} -Wno-mismatched-tags -Wno-extern-c-compat")
    # also ignore deprecated builtins (seen with clang 15 + boost 1.79)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0")
      set(warnCXXFLAGS "${warnCXXFLAGS} -Wno-deprecated-builtins")
    endif()
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
if("${CMAKE_SYSTEM}" MATCHES "Linux") # Breaks on FreeBSD within Boost headers :(
  set(warnCFLAGS "${warnCFLAGS} -Wundef")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wundef")
endif()

# Set the optimisation flags
# NOTE, we should CMAKE_BUILD_TYPE for this
if(enable_compile_optimizations)
  set(optCFLAGS "-O3 -funroll-loops -fno-strict-aliasing ")
else()
  set(optCFLAGS "-O0 ")
endif()

# ARM platforms have signed char by default, switch to unsigned for consistancy
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(optCFLAGS "${optCFLAGS} -fsigned-char")
endif()

if(enable_compile_optimizations AND CMAKE_COMPILER_IS_GNUCC)
  # This is redundant (already in -03):
  set(optCFLAGS "${optCFLAGS} -finline-functions ")
endif()

# Do not leak the current directory into the binaries
if(CMAKE_COMPILER_IS_GNUCC AND NOT enable_coverage)
  if (CMAKE_VERSION VERSION_LESS "3.20")
    file(RELATIVE_PATH RELATIVE_SOURCE_DIR ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
  else() # cmake >= 3.20
    cmake_path(RELATIVE_PATH CMAKE_SOURCE_DIR BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE RELATIVE_SOURCE_DIR)
  endif()
  message(STATUS "Relative source directory is \"${RELATIVE_SOURCE_DIR}\".")
  if (CMAKE_C_COMPILER_VERSION VERSION_LESS "8.0")
    set(optCFLAGS "${optCFLAGS} -fdebug-prefix-map=\"${CMAKE_SOURCE_DIR}=${RELATIVE_SOURCE_DIR}\"")
  else()
    set(optCFLAGS "${optCFLAGS} -ffile-prefix-map=\"${CMAKE_SOURCE_DIR}=${RELATIVE_SOURCE_DIR}\"")
  endif()
endif()

# Configure LTO
if(enable_lto) # User wants LTO. Try if we can do that
  set(enable_lto OFF)
  if(enable_compile_optimizations)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo LANGUAGES C CXX)
    if(ipo)
      set(enable_lto ON)
    endif()
  endif()

  if(enable_lto)
    message(STATUS "LTO seems usable.")
  else()
    if(NOT enable_compile_optimizations)
      message(STATUS "LTO disabled: Compile-time optimizations turned off.")
    else()
      message(STATUS "LTO does not seem usable -- try updating your build chain.")
    endif()
  endif()
else()
  message(STATUS "LTO disabled on the command line.")
endif()
if(enable_lto) # User wants LTO, and it seems usable. Go for it
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  if(LTO_EXTRA_FLAG AND CMAKE_COMPILER_IS_GNUCC)
    list(APPEND CMAKE_C_COMPILE_OPTIONS_IPO "-flto=${LTO_EXTRA_FLAG}")
    list(APPEND CMAKE_CXX_COMPILE_OPTIONS_IPO "-flto=${LTO_EXTRA_FLAG}")
  endif()

  # Activate fat-lto-objects in case LD and gfortran differ too much.
  # Only test with GNU as it's the only case I know (clang+gfortran+lld)
  execute_process(COMMAND ${CMAKE_LINKER} -v OUTPUT_VARIABLE LINKER_ID ERROR_VARIABLE LINKER_ID)
  string(REGEX MATCH "GNU" LINKER_ID "${LINKER_ID}")
  if(${CMAKE_Fortran_COMPILER_ID} MATCHES "GNU"
     AND NOT "${LINKER_ID}" MATCHES "GNU")
       list(APPEND CMAKE_Fortran_COMPILE_OPTIONS_IPO "-ffat-lto-objects")
  endif()

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

if (CMAKE_C_COMPILER_ID MATCHES "Intel")
  # honor parentheses when determining the order of expression evaluation.
  # disallow optimizations for floating-point arithmetic with Nans or +-Infs (breaks Eigen3)
  set(optCFLAGS "${optCFLAGS} -fprotect-parens -fno-finite-math-only")
endif()

if(NOT enable_debug)
  set(CMAKE_C_FLAGS "-DNDEBUG ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-DNDEBUG ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_FLAGS   "${warnCFLAGS} ${CMAKE_C_FLAGS}   ${optCFLAGS}")
set(CMAKE_CXX_FLAGS "${warnCXXFLAGS} ${CMAKE_CXX_FLAGS} ${optCFLAGS}")

# Try to make Mac a bit more compliant to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE")
endif()

set(TESH_OPTION "")
if(enable_coverage)
  find_program(GCOV_PATH NAMES ENV{GCOV} gcov)
  if(GCOV_PATH)
    set(COVERAGE_COMMAND "${GCOV_PATH}" CACHE FILEPATH "Coverage testing tool (gcov)" FORCE)
    set(COVERAGE_BUILD_FLAGS "-fprofile-arcs -ftest-coverage -fprofile-abs-path")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCOVERAGE ${COVERAGE_BUILD_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_BUILD_FLAGS}")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${COVERAGE_BUILD_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOVERAGE ${COVERAGE_BUILD_FLAGS}")
    add_definitions(${COVERAGE_BUILD_FLAGS})
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
    if(CMAKE_COMPILER_IS_GNUCXX AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0")))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=tsan")
    endif()
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
