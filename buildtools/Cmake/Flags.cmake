set(warnCFLAGS "")
set(optCFLAGS "")

if(NOT __VISUALC__ AND NOT __BORLANDC__)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-std=gnu99 -g3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-g3")
  set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -g")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}/Zi")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}/Zi")
endif()

if(enable_compile_warnings)
  set(warnCFLAGS "-fno-common -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(warnCFLAGS "${warnCFLAGS}-Wclobbered -Wno-error=clobbered ")
    if(COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.5")
      set(warnCFLAGS "${warnCFLAGS}-Wno-error=unused-but-set-variable ")
    endif()
    if(COMPILER_C_VERSION_MAJOR_MINOR STREQUAL "4.6")
    #some old compilers emit bogus warnings here, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45978 . Avoid failing the build in this case
      set(warnCFLAGS "${warnCFLAGS}-Wno-error=array-bounds")
    endif()
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra  -Wunused -Wpointer-arith -Wchar-subscripts -Wcomment -Wno-unknown-warning-option -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wclobbered -Wno-error=clobbered -Wno-format-nonliteral -Werror")

  set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wall") # FIXME: Q&D hack

  set(CMAKE_JAVA_COMPILE_FLAGS "-Xlint")
endif()

if(enable_compile_optimizations)
  set(optCFLAGS "-O3 -funroll-loops -fno-strict-aliasing ")
  if(CMAKE_COMPILER_IS_GNUCC AND (NOT enable_model-checking))
    set(optCFLAGS "${optCFLAGS} -finline-functions ")
    if(WIN32)
      if (COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.7")
      # On windows, we need 4.8 or higher to enable lto because of http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50293
        set(optCFLAGS "${optCFLAGS} -flto ")
      endif()
    else()    
      # On non-windows, 4.6 is enough for that
      if(COMPILER_C_VERSION_MAJOR_MINOR STRGREATER "4.5" AND LINKER_VERSION STRGREATER "2.22")
        set(optCFLAGS "${optCFLAGS} -flto ")
      endif()
    endif()
  endif()
else()
  set(optCFLAGS "-O0 ")
endif()

if(enable_sdt)
  add_definitions(-DUSE_SDT)
endif()

if(enable_ust)
  add_definitions(-DUSE_UST)
endif()

if(enable_model-checking AND enable_compile_optimizations)
  # Forget it, do not optimize the code (because it confuses the MC):
  set(optCFLAGS "-O0 ")
  # But you can still optimize this:
  foreach(s
      # src/xbt/mmalloc/mm.c
      # src/xbt/snprintf.c
      # src/xbt/log.c src/xbt/xbt_log_appender_file.c
      # src/xbt/xbt_log_layout_format.c src/xbt/xbt_log_layout_simple.c
      # src/xbt/dynar.c
      # src/xbt/dict.c src/xbt/dict_elm.c src/xbt/dict_multi.c
      # src/xbt/set.c src/xbt/setset.c
      # src/xbt/fifo.c
      # src/xbt/setset.c
      # src/xbt/heap.c
      # src/xbt/swag.c
      # src/xbt/str.c src/xbt/strbuff.c
      # src/xbt/queue.c
      # src/xbt/xbt_os_time.c src/xbt/xbt_os_thread.c
      # src/xbt/backtrace_linux.c
      ${MC_SRC_BASE} ${MC_SRC})
      set (mcCFLAGS "-O3  -funroll-loops -fno-strict-aliasing")
       if(CMAKE_COMPILER_IS_GNUCC)
         set (mcCFLAGS "${mcCFLAGS} -finline-functions")
      endif()
      set_source_files_properties(${s} PROPERTIES COMPILE_FLAGS ${mcCFLAGS})
  endforeach()
endif()

if(enable_mc_content_adressable_pages)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMC_PAGE_STORE_MD4")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMC_PAGE_STORE_MD4")
endif()

if(APPLE AND COMPILER_C_VERSION_MAJOR_MINOR MATCHES "4.6")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
  set(optCFLAGS "-O0 ")
endif()

if(NOT enable_debug)
  set(CMAKE_C_FLAGS "-DNDEBUG ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-DNDEBUG ${CMAKE_CXX_FLAGS}")
endif()

if(enable_msg_deprecated)
  set(CMAKE_C_FLAGS "-DMSG_USE_DEPRECATED ${CMAKE_C_FLAGS}")
endif()

set(CMAKE_C_FLAGS "${optCFLAGS} ${warnCFLAGS} ${CMAKE_C_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${optCFLAGS}")

# Try to make Mac a bit more complient to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE")
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
