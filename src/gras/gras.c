/* $Id$ */

/* gras.c -- generic functions not fitting anywhere else                    */

/* Copyright (c) 2003, 2004 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/module.h" /* xbt_init/exit */

#include "gras_modinter.h"   /* module init/exit */
#include "xbt_modinter.h"   /* module init/exit */

#include "gras/core.h"
#include "gras/process.h" /* FIXME: killme and put process_init in modinter */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras,XBT_LOG_ROOT_CAT,"All GRAS categories (cf. section \ref GRAS_API)");
static int gras_running_process = 0;

void gras_init(int *argc,char **argv, const char *defaultlog) {

  INFO0("Initialize GRAS");
  
  /* First initialize the XBT */
  xbt_init_defaultlog(argc,argv,defaultlog);
   
  /* module registrations: 
   *    - declare process specific data we need (without creating them) 
   */
  if (gras_running_process == 0) {
     gras_trp_register();
     gras_msg_register();
  }
   
  /*
   * Initialize the process specific stuff
   */
  gras_process_init(); /* calls procdata_init, which creates process specific data for each module */
  
  /*
   * Initialize the global stuff if it's not the first process created
   */
  if (gras_running_process++ == 0) {
    gras_emul_init();
    gras_msg_init();
    gras_trp_init();
    gras_datadesc_init();
  }
}

void gras_exit(void) {
  INFO0("Exiting GRAS");
  gras_process_exit();
  if (--gras_running_process == 0) {
    gras_msg_exit();
    gras_trp_exit();
    gras_datadesc_exit();
    gras_emul_exit();
  }
  xbt_exit();
}

void hexa_print(const char*name, unsigned char *data, int size) {
   int i;
   printf("%s: ", name);
   for (i=0;i<size;i++)  {
      if (data[i]<32)// || data[i]>'9')
	printf("'\\%d'",data[i]);
      else
	printf("%c",data[i]);
   }
   printf("\n");
}

