/* $Id$ */

/* module handling                                                          */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/config.h"

#include "gras/process.h" /* FIXME: bad loop */

#include "xbt/module.h" /* this module */

#include "xbt_modinter.h"  /* prototype of other module's init/exit in XBT */
#include "gras_modinter.h" /* same in GRAS */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module,xbt, "module handling");

static int xbt_running_process = 0;

struct xbt_module_ {
  xbt_dynar_t *deps;
  xbt_cfg_t *cfg;
  int ref;
  xbt_module_new_fct_t new;
  xbt_module_finalize_fct_t finalize;
};

void 
xbt_init(int *argc, char **argv) {
  static short int first_run = 1;
  if(first_run)
    xbt_init_defaultlog(argc, argv, NULL);
  first_run = 0;
}

/**
 * xbt_init_defaultlog:
 * @argc:
 * @argv:
 *
 * Initialize the gras mecanisms.
 */
void
xbt_init_defaultlog(int *argc,char **argv, const char *defaultlog) {
  INFO0("Initialize GRAS");
  
  xbt_log_init(argc,argv,defaultlog);
   
  gras_process_init(); /* calls procdata_init, which calls dynar_new */
  /** init other submodules */
  if (xbt_running_process++ == 0) {
    gras_msg_init();
    gras_trp_init();
    gras_datadesc_init();
  }
}

/**
 * xbt_exit:
 *
 * Finalize the gras mecanisms.
 */
void 
xbt_exit(){
  INFO0("Exiting GRAS");
  gras_process_exit();
  if (--xbt_running_process == 0) {
    gras_msg_exit();
    gras_trp_exit();
    gras_datadesc_exit();
  }
  xbt_log_exit();
  DEBUG0("Exited GRAS");
}
