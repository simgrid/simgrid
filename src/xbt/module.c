/* $Id$ */

/* module handling                                                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(module,GRAS);

extern void gras_log_finalize(void);

struct gras_module_ {
  gras_dynar_t *deps;
  gras_cfg_t *cfg;
  int ref;
  gras_module_new_fct_t new;
  gras_module_finalize_fct_t finalize;
};


/**
 * gras_init:
 * @argc:
 * @argv:
 *
 * Initialize the gras mecanisms.
 */
void
gras_init(int argc,char **argv) {
  int i;
  char *opt;
  gras_error_t errcode;

  INFO0("Initialize GRAS");
  for (i=1; i<argc; i++) {
    if (!strncmp(argv[i],"--gras-log=",strlen("--gras-log="))) {
      opt=strchr(argv[i],'=');
      opt++;
      TRYFAIL(gras_log_control_set(opt));
    }
  }
}

/**
 * gras_finalize:
 * @argc:
 * @argv:
 *
 * Finalize the gras mecanisms.
 */
void 
gras_finalize(){
  gras_log_finalize();
}
