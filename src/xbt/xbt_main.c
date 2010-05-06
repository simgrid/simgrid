/* module handling                                                          */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
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
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_add);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_collapse);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_cursor);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_elm);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_multi);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_remove);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dict_search);
XBT_LOG_EXTERNAL_CATEGORY(xbt_dyn);
XBT_LOG_EXTERNAL_CATEGORY(xbt_ex);
XBT_LOG_EXTERNAL_CATEGORY(xbt_fifo);
XBT_LOG_EXTERNAL_CATEGORY(xbt_graph);
XBT_LOG_EXTERNAL_CATEGORY(xbt_matrix);
XBT_LOG_EXTERNAL_CATEGORY(xbt_queue);
XBT_LOG_EXTERNAL_CATEGORY(xbt_set);
XBT_LOG_EXTERNAL_CATEGORY(xbt_sync_os);


/* Declare xbt_preinit and xbt_postexit as constructor/destructor of the library.
 * This is crude and rather compiler-specific, unfortunately.
 */
static void xbt_preinit(void) _XBT_GNUC_CONSTRUCTOR;
static void xbt_postexit(void) _XBT_GNUC_DESTRUCTOR;
#ifdef _XBT_NEED_INIT_PRAGMA
#pragma init (xbt_preinit)
#pragma fini (xbt_postexit)
#endif

#ifdef WIN32
#include <windows.h>

/* Dummy prototype to make gcc happy */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);


/* see also http://msdn.microsoft.com/en-us/library/ms682583%28VS.85%29.aspx */
/* and http://www.microsoft.com/whdc/driver/kernel/DLL_bestprac.mspx */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    xbt_preinit();
  } else if (fdwReason == DLL_PROCESS_DETACH) {
    xbt_postexit();
  }
  return 1;
}


#endif

static void xbt_preinit(void) {
  xbt_log_preinit();

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(graphxml_parse, xbt);
  XBT_LOG_CONNECT(log, xbt);
  XBT_LOG_CONNECT(module, xbt);
  XBT_LOG_CONNECT(peer, xbt);
  XBT_LOG_CONNECT(strbuff, xbt);
  XBT_LOG_CONNECT(xbt_cfg, xbt);
  XBT_LOG_CONNECT(xbt_dict, xbt);
  XBT_LOG_CONNECT(xbt_dict_add, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_collapse, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_cursor, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_elm, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_multi, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_remove, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_search, xbt_dict);
  XBT_LOG_CONNECT(xbt_dyn, xbt);
  XBT_LOG_CONNECT(xbt_ex, xbt);
  XBT_LOG_CONNECT(xbt_fifo, xbt);
  XBT_LOG_CONNECT(xbt_graph, xbt);
  XBT_LOG_CONNECT(xbt_matrix, xbt);
  XBT_LOG_CONNECT(xbt_queue, xbt);
  XBT_LOG_CONNECT(xbt_set, xbt);
  XBT_LOG_CONNECT(xbt_sync_os, xbt);

  xbt_fifo_preinit();

  xbt_backtrace_preinit();
  xbt_os_thread_mod_preinit();
}

static void xbt_postexit(void) {
  xbt_os_thread_mod_postexit();
  xbt_backtrace_postexit();

  xbt_fifo_postexit();
  xbt_dict_postexit();

  xbt_log_postexit();

  free(xbt_binary_name);
}

/** @brief Initialize the xbt mechanisms. */
void xbt_init(int *argc, char **argv)
{
  xbt_assert0(xbt_initialized == 0, "xbt_init must be called only once");
  xbt_initialized++;

  xbt_binary_name = xbt_strdup(argv[0]);
  srand((unsigned int) time(NULL));
  VERB0("Initialize XBT");

  xbt_log_init(argc, argv);
}

/** @brief Finalize the xbt mechanisms. */
void xbt_exit() {
}


/* these two functions belong to xbt/sysdep.h, which have no corresponding .c file */
/** @brief like free, but you can be sure that it is a function  */
XBT_PUBLIC(void) xbt_free_f(void *p) {
  free(p);
}

/** @brief should be given a pointer to pointer, and frees the second one */
XBT_PUBLIC(void) xbt_free_ref(void *d)
{
  free(*(void **) d);
}
