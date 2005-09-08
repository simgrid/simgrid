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

#include "gras.h"
#include "gras/process.h" /* FIXME: killme and put process_init in modinter */

#include "portable.h" /* hexa_*() */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras,XBT_LOG_ROOT_CAT,"All GRAS categories (cf. section \ref GRAS_API)");
static int gras_running_process = 0;

void gras_init(int *argc,char **argv) {

  VERB0("Initialize GRAS");
  
  /* First initialize the XBT */
  xbt_init(argc,argv);
   
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
  if (--gras_running_process == 0) {
    gras_msg_exit();
    gras_trp_exit();
    gras_datadesc_exit();
    gras_emul_exit();
  }
  gras_process_exit();
  xbt_exit();
}

const char *hexa_str(unsigned char *data, int size) {
  static char*buff=NULL;
  static int buffsize=0;
  int i,pos=0;	
  
  if (buffsize<5*(size+1)) {
    if (buff)
      free(buff);
    buffsize=5*(size+1);
    buff=xbt_malloc(buffsize);
  }
  for (i=0;i<size;i++)  {
    if (data[i]<32 || data[i]>126)
      sprintf(buff+pos,".(%02x)",data[i]);
    else
      sprintf(buff+pos,"%c(%02x)",data[i],data[i]);
    while (buff[++pos]);
   }
  buff[pos]='\0';  
  return buff;
}
void hexa_print(const char*name, unsigned char *data, int size) {
   printf("%s: %s\n", name,hexa_str(data,size));
}

