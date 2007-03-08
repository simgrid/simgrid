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

  xbt_binary_name = xbt_strdup(argv[0]);
  srand((unsigned int)time(NULL));
  VERB0("Initialize XBT");
  
  xbt_log_init(argc,argv);
  xbt_thread_mod_init();
}

/** @brief Finalize the xbt mechanisms. */
void 
xbt_exit(){
  xbt_initialized--;
  if (xbt_initialized == 0) {
    free(xbt_binary_name);
    xbt_fifo_exit();
    xbt_dict_exit();
    xbt_thread_mod_exit();
  }
  xbt_log_exit();
}

