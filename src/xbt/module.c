/* $Id$ */

/* module handling                                                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(module,GRAS);

extern void gras_log_exit(void);

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
      TRYFAIL(gras_log_control_set(opt));
      DEBUG1("Did apply '%s' as log setting",opt);
      /*remove this from argv*/
      for (j=i+1; j<*argc; j++) {
	argv[j-1] = argv[j];
      } 
      argv[j-1] = NULL;
      (*argc)--;
      i--; /* compensate effect of next loop incrementation */
      WARN1("argc %d",*argc);
    }
  }
  if (!found && defaultlog) {
     TRYFAIL(gras_log_control_set(defaultlog));
  }
   
  /** init other submodules */
  gras_msg_init();
  gras_trp_init();
  gras_datadesc_init();
}

/**
 * gras_exit:
 *
 * Finalize the gras mecanisms.
 */
void 
gras_exit(){
  gras_msg_exit();
  gras_trp_exit();
  gras_datadesc_exit();
  gras_log_exit();
}
