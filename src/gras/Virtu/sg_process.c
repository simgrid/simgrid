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
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_process_data_t *pd;
  int i;
  
  if (!(pd=(gras_process_data_t *)malloc(sizeof(gras_process_data_t)))) 
    RAISE_MALLOC;

  WARNING0("Implement msg queue");
  /*
  pd->grasMsgQueueLen=0;
  pd->grasMsgQueue = NULL;

  pd->grasCblListLen = 0;
  pd->grasCblList = NULL;
  */

  if (MSG_process_set_data(MSG_process_self(),(void*)pd) != MSG_OK) {
    return unknown_error;
  }

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
  gras_process_data_t *pd=(gras_process_data_t *)MSG_process_get_data(MSG_process_self());
  int myPID=MSG_process_self_PID();
  int i;

  gras_assert0(hd && pd,"Run gras_process_init!!\n");

  INFO2("GRAS: Finalizing process '%s' (%d)",
	MSG_process_get_name(MSG_process_self()),MSG_process_self_PID());

  WARNING0("Implement msg queue");
  /*
  if (pd->grasMsgQueueLen) {
    fprintf(stderr,"GRAS: Warning: process %d terminated, but some queued messages where not handled\n",MSG_process_self_PID());
  }
  */

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

void *gras_userdata_get(void) {
  gras_process_data_t *pd=(gras_process_data_t *)MSG_process_get_data(MSG_process_self());

  gras_assert0(pd,"Run gras_process_init!");

  return pd->userdata;
}

void *gras_userdata_set(void *ud) {
  gras_process_data_t *pd=(gras_process_data_t *)MSG_process_get_data(MSG_process_self());

  gras_assert0(pd,"Run gras_process_init!");

  pd->userdata = ud;

  return pd->userdata;
}
