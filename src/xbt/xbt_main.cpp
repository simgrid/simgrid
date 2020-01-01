/* module handling                                                          */

/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define XBT_LOG_LOCALLY_DEFINE_XBT_CHANNEL /* MSVC don't want it to be declared extern in headers and local here */

#include "simgrid/config.h"
#include "simgrid/sg_config.hpp"
#include "src/internal_config.h"
#include "src/xbt_modinter.h" /* prototype of other module's init/exit in XBT */
#include "xbt/config.hpp"
#include "xbt/coverage.h"
#include "xbt/dynar.h"
#include "xbt/log.h"
#include "xbt/log.hpp"
#include "xbt/misc.h"
#include "xbt/module.h" /* this module */
#include "xbt/sysdep.h"

#include <cmath>
#include <cstdio>
#ifdef _WIN32
# include <csignal> /* To silence MSVC on abort() */
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module, xbt, "module handling");

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories"); /* lives here even if that's a bit odd to solve linking issues: this is used in xbt_log_file_appender to detect whether SMPI is used (and thus whether we should unbench the writing to disk) */

namespace simgrid {
namespace xbt {
std::string binary_name;          /* Name of the system process containing us (mandatory to retrieve neat backtraces) */
std::vector<std::string> cmdline; /* all we got in argv */
} // namespace xbt
} // namespace simgrid

int xbt_initialized = 0;
simgrid::config::Flag<bool> cfg_dbg_clean_atexit{
    "debug/clean-atexit",
    {"clean-atexit"},
    "Whether to cleanup SimGrid at exit. Disable it if your code segfaults after its end.",
    true};

int xbt_pagesize;
int xbt_pagebits = 0;

/* Declare xbt_preinit and xbt_postexit as constructor/destructor of the library.
 * This is crude and rather compiler-specific, unfortunately.
 */
static void xbt_preinit() XBT_ATTRIB_CONSTRUCTOR(200);
static void xbt_postexit();

#ifdef _WIN32
#include <windows.h>

#ifndef __GNUC__
/* Should not be necessary but for some reason, DllMain is called twice at attachment and at detachment.*/
/* see also http://msdn.microsoft.com/en-us/library/ms682583%28VS.85%29.aspx */
/* and http://www.microsoft.com/whdc/driver/kernel/DLL_bestprac.mspx */
static BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  static bool xbt_dll_process_is_attached = false;
  if (fdwReason == DLL_PROCESS_ATTACH && not xbt_dll_process_is_attached) {
    xbt_dll_process_is_attached = true;
    xbt_preinit();
  } else if (fdwReason == DLL_PROCESS_DETACH && xbt_dll_process_is_attached) {
    xbt_dll_process_is_attached = false;
  }
  return 1;
}
#endif

#endif

static void xbt_preinit()
{
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  xbt_pagesize = si.dwPageSize;
#elif HAVE_SYSCONF
  xbt_pagesize = sysconf(_SC_PAGESIZE);
#else
# error Cannot get page size.
#endif

  xbt_pagebits = log2(xbt_pagesize);

#ifdef _TWO_DIGIT_EXPONENT
  /* Even printf behaves differently on Windows... */
  _set_output_format(_TWO_DIGIT_EXPONENT);
#endif
  xbt_log_preinit();
  xbt_dict_preinit();
  atexit(xbt_postexit);
}

static void xbt_postexit()
{
  if (not cfg_dbg_clean_atexit)
    return;
  xbt_initialized--;
  xbt_dict_postexit();
  xbt_log_postexit();
#if SIMGRID_HAVE_MC
  mmalloc_postexit();
#endif
}

/** @brief Initialize the xbt mechanisms. */
void xbt_init(int *argc, char **argv)
{
  xbt_initialized++;
  if (xbt_initialized > 1) {
    XBT_DEBUG("XBT has been initialized %d times.", xbt_initialized);
    return;
  }

  simgrid::xbt::install_exception_handler();

  simgrid::xbt::binary_name = argv[0];
  for (int i = 0; i < *argc; i++)
    simgrid::xbt::cmdline.push_back(argv[i]);

  xbt_log_init(argc, argv);
}

/* these two functions belong to xbt/sysdep.h, which have no corresponding .c file */
/** @brief like xbt_free, but you can be sure that it is a function  */
void xbt_free_f(void* p) noexcept(noexcept(::free))
{
  xbt_free(p);
}

/** @brief should be given a pointer to pointer, and frees the second one */
void xbt_free_ref(void* d) noexcept(noexcept(::free))
{
  xbt_free(*(void**)d);
}

/** @brief Kill the program in silence */
void xbt_abort()
{
  /* Call __gcov_flush on abort when compiling with coverage options. */
  coverage_checkpoint();
#ifdef _WIN32
  /* We said *in silence*. We don't want to see the error message printed by Microsoft's implementation of abort(). */
  raise(SIGABRT);
  signal(SIGABRT, SIG_DFL);
  raise(SIGABRT);
#endif
  abort();
}
