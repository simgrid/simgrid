/* module handling                                                          */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "simgrid_config.h"     /*HAVE_MMAP _XBT_WIN32 */
#include "time.h"               /* to seed the random generator */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/config.h"

#include "xbt/module.h"         /* this module */

#include "xbt_modinter.h"       /* prototype of other module's init/exit in XBT */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module, xbt, "module handling");

char *xbt_binary_name = NULL;   /* Mandatory to retrieve neat backtraces */
int xbt_initialized = 0;

XBT_LOG_EXTERNAL_CATEGORY(graphxml_parse);
XBT_LOG_EXTERNAL_CATEGORY(log);
XBT_LOG_EXTERNAL_CATEGORY(module);
XBT_LOG_EXTERNAL_CATEGORY(peer);
XBT_LOG_EXTERNAL_CATEGORY(strbuff);
XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_cursor);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_elm);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_multi);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dyn);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ex);
XBT_LOG_EXTERNAL_CATEGORY(xbt_fifo);
XBT_LOG_EXTERNAL_CATEGORY(xbt_graph);
XBT_LOG_EXTERNAL_CATEGORY(xbt_matrix);
XBT_LOG_EXTERNAL_CATEGORY(xbt_queue);
XBT_LOG_EXTERNAL_CATEGORY(xbt_set);
XBT_LOG_EXTERNAL_CATEGORY(xbt_sync_os);
XBT_LOG_EXTERNAL_CATEGORY(xbt_parmap);
XBT_LOG_EXTERNAL_CATEGORY(xbt_parmap_unit);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt_cbps);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt_convert);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt_create);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt_exchange);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt_lexer);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ddt_parse);
XBT_LOG_EXTERNAL_CATEGORY(xbt_trp);
XBT_LOG_EXTERNAL_CATEGORY(xbt_trp_meas);

int _surf_do_model_check = 0;   /* this variable is used accros the libraries, and must be declared in XBT so that it's also defined in GRAS (not only in libsimgrid) */

/* Declare xbt_preinit and xbt_postexit as constructor/destructor of the library.
 * This is crude and rather compiler-specific, unfortunately.
 */
static void xbt_preinit(void) _XBT_GNUC_CONSTRUCTOR;
static void xbt_postexit(void) _XBT_GNUC_DESTRUCTOR;

#ifdef _XBT_WIN32
# undef _XBT_NEED_INIT_PRAGMA
#endif

#ifdef _XBT_NEED_INIT_PRAGMA
#pragma init (xbt_preinit)
#pragma fini (xbt_postexit)
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
      xbt_postexit();
  }
  return 1;
}
#endif

#endif

static void xbt_preinit(void)
{
#ifdef MMALLOC_WANT_OVERIDE_LEGACY
  mmalloc_preinit();
#endif
  xbt_log_preinit();

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(graphxml_parse, xbt);
  XBT_LOG_CONNECT(log, xbt);
  XBT_LOG_CONNECT(module, xbt);
  XBT_LOG_CONNECT(peer, xbt);
  XBT_LOG_CONNECT(strbuff, xbt);
  XBT_LOG_CONNECT(xbt_cfg, xbt);
  XBT_LOG_CONNECT(xbt_dict, xbt);
  XBT_LOG_CONNECT(xbt_dict_cursor, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_elm, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_multi, xbt_dict);
  XBT_LOG_CONNECT(xbt_dyn, xbt);
  XBT_LOG_CONNECT(xbt_ex, xbt);
  XBT_LOG_CONNECT(xbt_fifo, xbt);
  XBT_LOG_CONNECT(xbt_graph, xbt);
  XBT_LOG_CONNECT(xbt_matrix, xbt);
  XBT_LOG_CONNECT(xbt_queue, xbt);
  XBT_LOG_CONNECT(xbt_set, xbt);
  XBT_LOG_CONNECT(xbt_sync_os, xbt);
  XBT_LOG_CONNECT(xbt_parmap,xbt);
  XBT_LOG_CONNECT(xbt_parmap_unit,xbt_parmap);
  XBT_LOG_CONNECT(xbt_ddt, xbt);
  XBT_LOG_CONNECT(xbt_ddt_cbps, xbt_ddt);
  XBT_LOG_CONNECT(xbt_ddt_convert, xbt_ddt);
  XBT_LOG_CONNECT(xbt_ddt_create, xbt_ddt);
  XBT_LOG_CONNECT(xbt_ddt_exchange, xbt_ddt);
  XBT_LOG_CONNECT(xbt_ddt_lexer, xbt_ddt_parse);
  XBT_LOG_CONNECT(xbt_ddt_parse, xbt_ddt);
  XBT_LOG_CONNECT(xbt_trp, xbt);
  XBT_LOG_CONNECT(xbt_trp_meas, xbt_trp);

  xbt_backtrace_preinit();
  xbt_os_thread_mod_preinit();
  xbt_fifo_preinit();
  xbt_dict_preinit();
  xbt_datadesc_preinit();
  xbt_trp_preinit();
}

static void xbt_postexit(void)
{
  xbt_trp_postexit();
  xbt_datadesc_postexit();

  xbt_backtrace_postexit();

  xbt_fifo_postexit();
  xbt_dict_postexit();

  xbt_log_postexit();
  xbt_os_thread_mod_postexit();

  free(xbt_binary_name);
#ifdef MMALLOC_WANT_OVERIDE_LEGACY
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
  XBT_VERB("Initialize XBT");

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
