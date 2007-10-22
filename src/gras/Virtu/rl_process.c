/* $Id$ */

/* process_rl - GRAS process handling on real life                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_modinter.h" /* module initialization interface */
#include "gras/Virtu/virtu_rl.h"
#include "portable.h"

/* globals */
static gras_procdata_t *_gras_procdata = NULL;
XBT_EXPORT_NO_IMPORT(char const *) _gras_procname = NULL;

void gras_process_init() {
  _gras_procdata=xbt_new0(gras_procdata_t,1);
  gras_procdata_init();
}
void gras_process_exit() {
  gras_procdata_exit();
  free(_gras_procdata);
}

const char *xbt_procname(void) {
  if(_gras_procname) return _gras_procname;
  else return "";
}

int gras_os_getpid(void) {
#ifdef _WIN32
   return (long int) GetCurrentProcessId();
#else
   return (long int) getpid();
#endif
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void) {
  xbt_assert0(_gras_procdata,"Run gras_process_init (ie, gras_init)!");

  return _gras_procdata;
}

void gras_agent_spawn(const char *name, void *data, 
		      xbt_main_func_t code, int argc, char *argv[]) {
   THROW_UNIMPLEMENTED;
}
