/* module handling                                                          */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "simgrid_config.h"     /*HAVE_MMAP _XBT_WIN32 */
#include "gras_config.h"        /* MMALLOC_WANT_OVERRIDE_LEGACY */
#include "time.h"               /* to seed the random generator */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/config.h"

#include "xbt/module.h"         /* this module */

#include "xbt_modinter.h"       /* prototype of other module's init/exit in XBT */

XBT_LOG_NEW_CATEGORY(xbt, "All XBT categories (simgrid toolbox)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module, xbt, "module handling");

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories"); /* lives here even if that's a bit odd to solve linking issues: this is used in xbt_log_file_appender to detect whether SMPI is used (and thus whether we should unbench the writing to disk) */


char *xbt_binary_name = NULL;   /* Mandatory to retrieve neat backtraces */
int xbt_initialized = 0;

int _surf_do_model_check = 0;
int _surf_mc_checkpoint=0;
char* _surf_mc_property_file=NULL;
int _surf_mc_timeout=0;
int _surf_mc_max_depth=1000;
int _surf_mc_visited=0;

/* Declare xbt_preinit and xbt_postexit as constructor/destructor of the library.
 * This is crude and rather compiler-specific, unfortunately.
 */
static void xbt_preinit(void) _XBT_GNUC_CONSTRUCTOR(200);
static void xbt_postexit(void);

#ifdef _XBT_WIN32
# undef _XBT_NEED_INIT_PRAGMA
#endif

#ifdef _XBT_NEED_INIT_PRAGMA
#pragma init (xbt_preinit)
#endif

#ifdef _XBT_WIN32
#include <windows.h>

#ifndef __GNUC__
/* Dummy prototype to make gcc happy */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved);

/* Should not be necessary but for some reason,
 * DllMain is called twice at attachment and
 * at detachment.*/
static int xbt_dll_process_is_attached = 0;

/* see also http://msdn.microsoft.com/en-us/library/ms682583%28VS.85%29.aspx */
/* and http://www.microsoft.com/whdc/driver/kernel/DLL_bestprac.mspx */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
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

static void xbt_preinit(void)
{
#ifdef MMALLOC_WANT_OVERRIDE_LEGACY
  mmalloc_preinit();
#endif
  xbt_log_preinit();

  xbt_backtrace_preinit();
  xbt_os_thread_mod_preinit();
  xbt_fifo_preinit();
  xbt_dict_preinit();

  atexit(xbt_postexit);
}

static void xbt_postexit(void)
{
  xbt_backtrace_postexit();

  xbt_fifo_postexit();
  xbt_dict_postexit();

  xbt_log_postexit();
  xbt_os_thread_mod_postexit();

  free(xbt_binary_name);
#ifdef MMALLOC_WANT_OVERRIDE_LEGACY
  mmalloc_postexit();
#endif
}

/** @brief Initialize the xbt mechanisms. */
void xbt_init(int *argc, char **argv)
{
  // FIXME it would be nice to assert that this function is called only once. But each gras process do call it...
  xbt_initialized++;

  if (xbt_initialized > 1)
    return;

  xbt_binary_name = xbt_strdup(argv[0]);
  srand((unsigned int) time(NULL));

  xbt_log_init(argc, argv);
}

/** @brief Finalize the xbt mechanisms. */
void xbt_exit()
{
  XBT_WARN("This function is deprecated, you shouldn't use it");
}


/* these two functions belong to xbt/sysdep.h, which have no corresponding .c file */
/** @brief like free, but you can be sure that it is a function  */
XBT_PUBLIC(void) xbt_free_f(void *p)
{
  free(p);
}

/** @brief should be given a pointer to pointer, and frees the second one */
XBT_PUBLIC(void) xbt_free_ref(void *d)
{
  free(*(void **) d);
}
