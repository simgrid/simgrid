/* $Id$ */

/* module handling                                                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "time.h" /* to seed the random generator */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/config.h"

#include "xbt/module.h" /* this module */

#include "xbt_modinter.h"  /* prototype of other module's init/exit in XBT */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module,xbt, "module handling");

char *xbt_binary_name=NULL; /* Mandatory to retrieve neat backtraces */
int xbt_initialized=0;

/** @brief Initialize the xbt mechanisms. */
void 
xbt_init(int *argc, char **argv) {
  xbt_initialized++;

  if (xbt_initialized!=1)
    return;

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(graphxml_parse, xbt);
  XBT_LOG_CONNECT(log, xbt);
  XBT_LOG_CONNECT(module, xbt);
  XBT_LOG_CONNECT(peer, xbt);
  XBT_LOG_CONNECT(strbuff, xbt);
  XBT_LOG_CONNECT(xbt_cfg, xbt);
  XBT_LOG_CONNECT(xbt_dict_add, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_collapse, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_cursor, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_elm, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_multi, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_remove, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_search, xbt_dict);
  XBT_LOG_CONNECT(xbt_dict, xbt);
  XBT_LOG_CONNECT(xbt_dyn, xbt);
  XBT_LOG_CONNECT(xbt_ex, xbt);
  XBT_LOG_CONNECT(xbt_fifo, xbt);
  XBT_LOG_CONNECT(xbt_graph, xbt);
  XBT_LOG_CONNECT(xbt_matrix, xbt);
  XBT_LOG_CONNECT(xbt_queue, xbt);
  XBT_LOG_CONNECT(xbt_set, xbt);
  XBT_LOG_CONNECT(xbt_sync_os, xbt);
  XBT_LOG_CONNECT(xbt_sync_rl, xbt);
   
  xbt_binary_name = xbt_strdup(argv[0]);
  srand((unsigned int)time(NULL));
  VERB0("Initialize XBT");
  
  xbt_backtrace_init();
  xbt_log_init(argc,argv);
  xbt_os_thread_mod_init();
  xbt_context_mod_init();
}

/** @brief Finalize the xbt mechanisms. */
void 
xbt_exit(){
  xbt_initialized--;
  if (xbt_initialized == 0) {
    xbt_fifo_exit();
    xbt_dict_exit();
    xbt_context_mod_exit();
    xbt_os_thread_mod_exit();
    xbt_log_exit();
    xbt_backtrace_exit();
  }
   
  if (xbt_initialized == 0)
    free(xbt_binary_name);
}

