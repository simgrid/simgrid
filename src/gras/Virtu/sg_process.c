/* $Id$ */

/* process_sg - GRAS process handling on simulator                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_modinter.h" /* module initialization interface */
#include "gras/Virtu/virtu_sg.h"
#include "gras/Msg/msg_interface.h" /* For some checks at simulation end */
#include "gras/Transport/transport_interface.h" /* For some checks at simulation end */

XBT_LOG_EXTERNAL_CATEGORY(process);
XBT_LOG_DEFAULT_CATEGORY(process);

xbt_error_t
gras_process_init() {
  xbt_error_t errcode;
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=xbt_new(gras_procdata_t,1);
  gras_trp_procdata_t trp_pd;
  gras_sg_portrec_t prraw,pr;
  int i;
  
  if (MSG_process_set_data(MSG_process_self(),(void*)pd) != MSG_OK)
    return unknown_error;
   
  gras_procdata_init();

  if (!hd) {
    /* First process on this host */
    hd=xbt_new(gras_hostdata_t,1);
    hd->ports = xbt_dynar_new(sizeof(gras_sg_portrec_t),NULL);

    memset(hd->proc, 0, sizeof(hd->proc[0]) * XBT_MAX_CHANNEL); 

    if (MSG_host_set_data(MSG_host_self(),(void*)hd) != MSG_OK)
      return unknown_error;
  }
  
  /* take a free channel for this process */
  trp_pd = (gras_trp_procdata_t)gras_libdata_get("gras_trp");
  for (i=0; i<XBT_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == XBT_MAX_CHANNEL) 
    RAISE2(system_error,
	   "GRAS: Can't add a new process on %s, because all channel are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS\n.",
	    MSG_host_get_name(MSG_host_self()),XBT_MAX_CHANNEL);

  trp_pd->chan = i;
  hd->proc[ i ] = MSG_process_self_PID();

  /* regiter it to the ports structure */
  pr.port = -1;
  pr.tochan = i;
  pr.raw = 0;
  xbt_dynar_push(hd->ports,&pr);

  /* take a free RAW channel for this process */
  for (i=0; i<XBT_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == XBT_MAX_CHANNEL) {
    RAISE2(system_error,
	   "GRAS: Can't add a new process on %s, because all channel are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS\n.",
	    MSG_host_get_name(MSG_host_self()),XBT_MAX_CHANNEL);
  }
  trp_pd->rawChan = i;

  hd->proc[ i ] = MSG_process_self_PID();

  /* register it to the ports structure */
  prraw.port = -1;
  prraw.tochan = i;
  prraw.raw = 1;
  xbt_dynar_push(hd->ports,&prraw);

  VERB2("Creating process '%s' (%d)",
	   MSG_process_get_name(MSG_process_self()),
	   MSG_process_self_PID());
  return no_error;
}

xbt_error_t
gras_process_exit() {
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_msg_procdata_t msg_pd=(gras_msg_procdata_t)gras_libdata_get("gras_msg");
  gras_trp_procdata_t trp_pd=(gras_trp_procdata_t)gras_libdata_get("gras_trp");
  int myPID=MSG_process_self_PID();
  int cpt;
  gras_sg_portrec_t pr;

  xbt_assert0(hd,"Run gras_process_init (ie, gras_init)!!");

  INFO2("GRAS: Finalizing process '%s' (%d)",
	MSG_process_get_name(MSG_process_self()),MSG_process_self_PID());

  if (xbt_dynar_length(msg_pd->msg_queue))
    WARN1("process %d terminated, but some messages are still queued",
	  MSG_process_self_PID());

  for (cpt=0; cpt< XBT_MAX_CHANNEL; cpt++)
    if (myPID == hd->proc[cpt])
      hd->proc[cpt] = 0;

  xbt_dynar_foreach(hd->ports, cpt, pr) {
    if (pr.port == trp_pd->chan || pr.port == trp_pd->rawChan) {
      xbt_dynar_cursor_rm(hd->ports, &cpt);
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

  xbt_assert0(pd,"Run gras_process_init! (ie, gras_init)");

  return pd;
}

