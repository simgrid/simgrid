/* $Id$ */

/* gras.c -- generic functions not fitting anywhere else                    */

/* Copyright (c) 2003, 2004 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/virtu.h" /* set the XBT virtualization to use GRAS */
#include "xbt/module.h" /* xbt_init/exit */

#include "Virtu/virtu_interface.h" /* Module mechanism FIXME: deplace&rename */
#include "gras_modinter.h"   /* module init/exit */
#include "amok/amok_modinter.h"   /* module init/exit */
#include "xbt_modinter.h"   /* module init/exit */

#include "gras.h"
#include "gras/process.h" /* FIXME: killme and put process_init in modinter */
  
#include "portable.h" /* hexa_*(); signalling stuff */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras,XBT_LOG_ROOT_CAT,"All GRAS categories (cf. section \ref GRAS_API)");
static int gras_running_process = 0;
#if defined(HAVE_SIGNAL) && defined(HAVE_SIGNAL_H)
static void gras_sigusr_handler(int sig) {
   INFO0("SIGUSR1 received. Display the backtrace");
   xbt_backtrace_display();
}

static void gras_sigint_handler(int sig) {
   static double lastone = 0;
   if (lastone == 0 || gras_os_time() - lastone > 5) {
      lastone = gras_os_time();
      xbt_backtrace_display();
      fprintf(stderr,"\nBacktrace displayed because Ctrl-C was pressed. Press again (within 5 sec) to abort the process.\n");
   } else {
      exit(1);
   }
}
#endif

void gras_init(int *argc,char **argv) {

  VERB0("Initialize GRAS");

  xbt_getpid = &gras_os_getpid;
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
#if defined(HAVE_SIGNAL) && defined(HAVE_SIGNAL_H)
# ifdef SIGUSR1
    signal(SIGUSR1,gras_sigusr_handler);
# endif
    signal(SIGINT,gras_sigint_handler);
#endif     
  }
   
  /* and then init amok */
  amok_init();
}

void gras_exit(void) {
  INFO0("Exiting GRAS");
  amok_exit();
  gras_moddata_leave();
  if (--gras_running_process == 0) {
    gras_msg_exit();
    gras_trp_exit();
    gras_datadesc_exit();
    gras_emul_exit();
    gras_moddata_exit();
  }
  gras_process_exit();
  xbt_exit();
}

const char *hexa_str(unsigned char *data, int size, int downside) {
  static char*buff=NULL;
  static int buffsize=0;
  int i,pos=0;
  int begin,increment;	
  
  if (buffsize<5*(size+1)) {
    if (buff)
      free(buff);
    buffsize=5*(size+1);
    buff=xbt_malloc(buffsize);
  }
   

  if (downside) {
     begin=size-1;
     increment=-1;
  } else {
     begin=0;
     increment=1;
  }
   
  for (i=begin; 0<=i && i<size ; i+=increment)  {
    if (data[i]<32 || data[i]>126)
      sprintf(buff+pos,".");
    else
      sprintf(buff+pos,"%c",data[i]);
    while (buff[++pos]);
   }
  sprintf(buff+pos,"(");
  while (buff[++pos]);
  for (i=begin; 0<=i && i<size ; i+=increment)  {
    sprintf(buff+pos,"%02x",data[i]);
    while (buff[++pos]);
   }
  sprintf(buff+pos,")");
  while (buff[++pos]);
  buff[pos]='\0';  
  return buff;
}
void hexa_print(const char*name, unsigned char *data, int size) {
   printf("%s: %s\n", name,hexa_str(data,size,0));
}

