/* $Id$ */

/* module handling                                                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/config.h"

#include "xbt/module.h" /* this module */

#include "xbt_modinter.h"  /* prototype of other module's init/exit in XBT */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module,xbt, "module handling");

struct xbt_module_ {
  xbt_dynar_t *deps;
  xbt_cfg_t *cfg;
  int ref;
  xbt_module_new_fct_t new;
  xbt_module_finalize_fct_t finalize;
};

/** @brief Initialize the xbt mechanisms. */
void 
xbt_init(int *argc, char **argv) {
  xbt_init_defaultlog(argc, argv, NULL);
}

/** @brief Initialize the xbt mechanisms. */
void
xbt_init_defaultlog(int *argc,char **argv, const char *defaultlog) {
  static short int first_run = 1;
  if (!first_run)
    return;
  
  first_run = 0;
  INFO0("Initialize XBT");
  
  xbt_log_init(argc,argv,defaultlog);
}

/** @brief Finalize the xbt mechanisms. */
void 
xbt_exit(){
  xbt_log_exit();
}

