/* $Id$ */

/* process_sg - GRAS process handling on simulator                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Virtu/virtu_sg.h"

GRAS_LOG_EXTERNAL_CATEGORY(process);

gras_error_t
gras_process_init() {
  gras_error_t errcode;
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd;
  gras_sg_portrec_t prraw,pr;
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

    TRY(gras_dynar_new(&(hd->ports),sizeof(gras_sg_portrec_t),NULL));

    memset(hd->proc, 0, sizeof(hd->proc[0]) * GRAS_MAX_CHANNEL); 

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

  /* regiter it to the ports structure */
  pr.port = -1;
  pr.tochan = i;
  pr.raw = 0;
  TRY(gras_dynar_push(hd->ports,&pr));

  /* take a free RAW channel for this process */
  for (i=0; i<GRAS_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == GRAS_MAX_CHANNEL) {
    RAISE2(system_error,
	   "GRAS: Can't add a new process on %s, because all channel are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS\n.",
	    MSG_host_get_name(MSG_host_self()),GRAS_MAX_CHANNEL);
  }
  pd->rawChan = i;

  hd->proc[ i ] = MSG_process_self_PID();

  /* regiter it to the ports structure */
  prraw.port = -1;
  prraw.tochan = i;
  prraw.raw = 1;
  TRY(gras_dynar_push(hd->ports,&prraw));

  VERB2("Creating process '%s' (%d)",
	   MSG_process_get_name(MSG_process_self()),
	   MSG_process_self_PID());
  return no_error;
}

gras_error_t
gras_process_exit() {
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=gras_procdata_get();
  int myPID=MSG_process_self_PID();
  int cpt;
  gras_sg_portrec_t pr;

  gras_assert0(hd && pd,"Run gras_process_init!!");

  INFO2("GRAS: Finalizing process '%s' (%d)",
	MSG_process_get_name(MSG_process_self()),MSG_process_self_PID());

  if (gras_dynar_length(pd->msg_queue))
    WARN1("process %d terminated, but some messages are still queued",
	  MSG_process_self_PID());

  for (cpt=0; cpt< GRAS_MAX_CHANNEL; cpt++)
    if (myPID == hd->proc[cpt])
      hd->proc[cpt] = 0;

  gras_dynar_foreach(hd->ports, cpt, pr) {
    if (pr.port == pd->chan || pr.port == pd->rawChan) {
      gras_dynar_cursor_rm(hd->ports, &cpt);
    }
  }

  return no_error;
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void) {
  gras_procdata_t *pd=
    (gras_procdata_t *)MSG_process_get_data(MSG_process_self());

  gras_assert0(pd,"Run gras_process_init!");

  return pd;
}

