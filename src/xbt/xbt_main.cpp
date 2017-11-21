/* module handling                                                          */

/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define XBT_LOG_LOCALLY_DEFINE_XBT_CHANNEL /* MSVC don't want it to be declared extern in headers and local here */

#include "simgrid_config.h"
#include "xbt/config.h"
#include "xbt/dynar.h"
#include "xbt/log.h"
#include "xbt/log.hpp"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include <cmath>

#include "xbt/module.h"         /* this module */

#include "src/xbt_modinter.h"       /* prototype of other module's init/exit in XBT */

#include "simgrid/sg_config.h"

#include "src/internal_config.h"
#include <cstdio>
#ifdef _WIN32
# include <csignal> /* To silence MSVC on abort() */
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module, xbt, "module handling");

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories"); /* lives here even if that's a bit odd to solve linking issues: this is used in xbt_log_file_appender to detect whether SMPI is used (and thus whether we should unbench the writing to disk) */

char *xbt_binary_name = NULL;   /* Name of the system process containing us (mandatory to retrieve neat backtraces) */
xbt_dynar_t xbt_cmdline = NULL; /* all we got in argv */

int xbt_initialized = 0;
int _sg_do_clean_atexit = 1;

int xbt_pagesize;
int xbt_pagebits = 0;

/* Declare xbt_preinit and xbt_postexit as constructor/destructor of the library.
 * This is crude and rather compiler-specific, unfortunately.
 */
static void xbt_preinit() _XBT_GNUC_CONSTRUCTOR(200);
static void xbt_postexit();

#ifdef _WIN32
#include <windows.h>

#ifndef __GNUC__
/* Should not be necessary but for some reason, DllMain is called twice at attachment and at detachment.*/
static int xbt_dll_process_is_attached = 0;

/* see also http://msdn.microsoft.com/en-us/library/ms682583%28VS.85%29.aspx */
/* and http://www.microsoft.com/whdc/driver/kernel/DLL_bestprac.mspx */
static BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH
      && xbt_dll_process_is_attached == 0) {
    xbt_dll_process_is_attached = 1;
    xbt_preinit();
  } else if (fdwReason == DLL_PROCESS_DETACH
      && xbt_dll_process_is_attached == 1) {
    xbt_dll_process_is_attached = 0;
  }
  return 1;
}
#endif

#endif

static void xbt_preinit()
{
  unsigned int seed = 2147483647;
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
  xbt_os_thread_mod_preinit();
  xbt_dict_preinit();

  srand(seed);
#ifndef _WIN32
  srand48(seed);
#endif
  atexit(xbt_postexit);
}

static void xbt_postexit()
{
  if (not _sg_do_clean_atexit)
    return;
  xbt_initialized--;
  xbt_dict_postexit();
  xbt_os_thread_mod_postexit();
  xbt_dynar_free(&xbt_cmdline);
  xbt_log_postexit();
#if SIMGRID_HAVE_MC
  mmalloc_postexit();
#endif
}

/** @brief Initialize the xbt mechanisms. */
void xbt_init(int *argc, char **argv)
{
  simgrid::xbt::installExceptionHandler();

  xbt_initialized++;
  if (xbt_initialized > 1) {
    XBT_DEBUG("XBT has been initialized %d times.", xbt_initialized);
    return;
  }

  xbt_binary_name = argv[0];
  xbt_cmdline     = xbt_dynar_new(sizeof(char*), NULL);
  for (int i = 0; i < *argc; i++)
    xbt_dynar_push(xbt_cmdline,&(argv[i]));

  xbt_log_init(argc, argv);
}

/* these two functions belong to xbt/sysdep.h, which have no corresponding .c file */
/** @brief like xbt_free, but you can be sure that it is a function  */
void xbt_free_f(void *p)
{
  xbt_free(p);
}

/** @brief should be given a pointer to pointer, and frees the second one */
void xbt_free_ref(void *d)
{
  xbt_free(*(void**)d);
}

/** @brief Kill the program in silence */
void xbt_abort()
{
#ifdef COVERAGE
  /* Call __gcov_flush on abort when compiling with coverage options. */
  extern void __gcov_flush();
  __gcov_flush();
#endif
#ifdef _WIN32
  /* We said *in silence*. We don't want to see the error message printed by Microsoft's implementation of abort(). */
  raise(SIGABRT);
  signal(SIGABRT, SIG_DFL);
  raise(SIGABRT);
#endif
  abort();
}
