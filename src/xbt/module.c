/* $Id$ */

/* module handling                                                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/config.h"

#include "gras/process.h" /* FIXME: bad loop */

#include "xbt/module.h" /* this module */

#include "xbt_modinter.h"  /* prototype of other module's init/exit in XBT */
#include "gras_modinter.h" /* same in GRAS */

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(module,xbt, "module handling");

static int gras_running_process = 0;

struct gras_module_ {
  gras_dynar_t *deps;
  gras_cfg_t *cfg;
  int ref;
  gras_module_new_fct_t new;
  gras_module_finalize_fct_t finalize;
};

void 
gras_init(int *argc, char **argv) {
   gras_init_defaultlog(argc, argv, NULL);
}

/**
 * gras_init_defaultlog:
 * @argc:
 * @argv:
 *
 * Initialize the gras mecanisms.
 */
void
gras_init_defaultlog(int *argc,char **argv, const char *defaultlog) {
  int i,j;
  char *opt;
  gras_error_t errcode;
  int found=0;

  INFO0("Initialize GRAS");
  
  /** Set logs and init log submodule */
  for (i=1; i<*argc; i++) {
    if (!strncmp(argv[i],"--gras-log=",strlen("--gras-log="))) {
      found = 1;
      opt=strchr(argv[i],'=');
      opt++;
      gras_log_control_set(opt);
      DEBUG1("Did apply '%s' as log setting",opt);
      /*remove this from argv*/
      for (j=i+1; j<*argc; j++) {
	argv[j-1] = argv[j];
      } 
      argv[j-1] = NULL;
      (*argc)--;
      i--; /* compensate effect of next loop incrementation */
    }
  }
  if (!found && defaultlog) {
     gras_log_control_set(defaultlog);
  }
   
  gras_process_init(); /* calls procdata_init, which calls dynar_new */
  /** init other submodules */
  if (gras_running_process++ == 0) {
    gras_msg_init();
    gras_trp_init();
    gras_datadesc_init();
  }
}

/**
 * gras_exit:
 *
 * Finalize the gras mecanisms.
 */
void 
gras_exit(){
  INFO0("Exiting GRAS");
  gras_process_exit();
  if (--gras_running_process == 0) {
    gras_msg_exit();
    gras_trp_exit();
    gras_datadesc_exit();
  }
  gras_log_exit();
  DEBUG0("Exited GRAS");
}
