/* $Id$ */

/* process_sg - GRAS process handling on simulator                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Virtu/virtu_sg.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(process,GRAS);

gras_error_t
gras_process_init() {
  gras_error_t errcode;
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd;
  int i;
  
  if (!(pd=(gras_procdata_t *)malloc(sizeof(gras_procdata_t)))) 
    RAISE_MALLOC;

  if (MSG_process_set_data(MSG_process_self(),(void*)pd) != MSG_OK) {
    return unknown_error;
  }
  TRY(gras_procdata_init());

  if (!hd) {
    if (!(hd=(gras_hostdata_t *)malloc(sizeof(gras_hostdata_t)))) 
      RAISE_MALLOC;

    hd->portLen = 0;
    hd->port=NULL;
    hd->port2chan=NULL;
    for (i=0; i<GRAS_MAX_CHANNEL; i++) {
      hd->proc[i]=0;
    }

    if (MSG_host_set_data(MSG_host_self(),(void*)hd) != MSG_OK) {
      return unknown_error;
    }
  }
  
  /* take a free channel for this process */
  for (i=0; i<GRAS_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == GRAS_MAX_CHANNEL) 
    RAISE2(system_error,
	   "GRAS: Can't add a new process on %s, because all channel are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS\n.",
	    MSG_host_get_name(MSG_host_self()),GRAS_MAX_CHANNEL);

  pd->chan = i;
  hd->proc[ i ] = MSG_process_self_PID();

  /* take a free RAW channel for this process */
  for (i=0; i<GRAS_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == GRAS_MAX_CHANNEL) {
    RAISE2(system_error,
	   "GRAS: Can't add a new process on %s, because all channel are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS\n.",
	    MSG_host_get_name(MSG_host_self()),GRAS_MAX_CHANNEL);
  }
  pd->rawChan = i;
  hd->proc[ i ] = MSG_process_self_PID();

  VERB2("Creating process '%s' (%d)",
	   MSG_process_get_name(MSG_process_self()),
	   MSG_process_self_PID());
  return no_error;
}

gras_error_t
gras_process_exit() {
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=(gras_procdata_t *)MSG_process_get_data(MSG_process_self());
  int myPID=MSG_process_self_PID();
  int i;

  gras_assert0(hd && pd,"Run gras_process_init!!\n");

  INFO2("GRAS: Finalizing process '%s' (%d)",
	MSG_process_get_name(MSG_process_self()),MSG_process_self_PID());

  if (gras_dynar_length(pd->msg_queue))
    WARN1("process %d terminated, but some queued messages where not handled",MSG_process_self_PID());

  for (i=0; i< GRAS_MAX_CHANNEL; i++)
    if (myPID == hd->proc[i])
      hd->proc[i] = 0;

  for (i=0; i<hd->portLen; i++) {
    if (hd->port2chan[ i ] == pd->chan) {
      memmove(&(hd->port[i]),      &(hd->port[i+1]),      (hd->portLen -i -1) * sizeof(int));
      memmove(&(hd->port2chan[i]), &(hd->port2chan[i+1]), (hd->portLen -i -1) * sizeof(int));
      hd->portLen--;
      i--; /* counter the effect of the i++ at the end of the iteration */
    }
  }

  return no_error;
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void) {
  gras_procdata_t *pd=(gras_procdata_t *)MSG_process_get_data(MSG_process_self());

  gras_assert0(pd,"Run gras_process_init!");

  return pd;
}

