/* $Id$ */

/* process_sg - GRAS process handling on simulator                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras_modinter.h" /* module initialization interface */
#include "gras/Virtu/virtu_sg.h"
#include "gras/Msg/msg_interface.h" /* For some checks at simulation end */
#include "gras/Transport/transport_interface.h" /* For some checks at simulation end */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_virtu_process);

void
gras_process_init() {
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=xbt_new0(gras_procdata_t,1);
  gras_trp_procdata_t trp_pd;
  gras_sg_portrec_t prmeas,pr;
  int i;
  
  if (MSG_process_set_data(MSG_process_self(),(void*)pd) != MSG_OK)
    THROW0(system_error,0,"Error in MSG_process_set_data()");
   
  gras_procdata_init();

  if (!hd) {
    /* First process on this host */
    hd=xbt_new(gras_hostdata_t,1);
    hd->refcount = 1;
    hd->ports = xbt_dynar_new(sizeof(gras_sg_portrec_t),NULL);

    memset(hd->proc, 0, sizeof(hd->proc[0]) * XBT_MAX_CHANNEL); 

    if (MSG_host_set_data(MSG_host_self(),(void*)hd) != MSG_OK)
      THROW0(system_error,0,"Error in MSG_host_set_data()");
  } else {
    hd->refcount++;
  }
  
  /* take a free channel for this process */
  trp_pd = (gras_trp_procdata_t)gras_libdata_by_name("gras_trp");
  for (i=0; i<XBT_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == XBT_MAX_CHANNEL) 
    THROW2(system_error,0,
	   "Can't add a new process on %s, because all channels are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS.",
	    MSG_host_get_name(MSG_host_self()),XBT_MAX_CHANNEL);

  trp_pd->chan = i;
  hd->proc[ i ] = MSG_process_self_PID();

  /* regiter it to the ports structure */
  pr.port = -1;
  pr.tochan = i;
  pr.meas = 0;
  xbt_dynar_push(hd->ports,&pr);

  /* take a free meas channel for this process */
  for (i=0; i<XBT_MAX_CHANNEL && hd->proc[i]; i++);
  if (i == XBT_MAX_CHANNEL) {
    THROW2(system_error,0,
	   "Can't add a new process on %s, because all channels are already in use. Please increase MAX CHANNEL (which is %d for now) and recompile GRAS.",
	    MSG_host_get_name(MSG_host_self()),XBT_MAX_CHANNEL);
  }
  trp_pd->measChan = i;

  hd->proc[ i ] = MSG_process_self_PID();

  /* register it to the ports structure */
  prmeas.port = -1;
  prmeas.tochan = i;
  prmeas.meas = 1;
  xbt_dynar_push(hd->ports,&prmeas);

  VERB2("Creating process '%s' (%d)",
	   MSG_process_get_name(MSG_process_self()),
	   MSG_process_self_PID());
}

void
gras_process_exit() {
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=(gras_procdata_t*)MSG_process_get_data(MSG_process_self());

  gras_msg_procdata_t msg_pd=(gras_msg_procdata_t)gras_libdata_by_name("gras_msg");
  gras_trp_procdata_t trp_pd=(gras_trp_procdata_t)gras_libdata_by_name("gras_trp");
  int myPID=MSG_process_self_PID();
  int cpt;
  gras_sg_portrec_t pr;

  xbt_assert0(hd,"Run gras_process_init (ie, gras_init)!!");

  VERB2("GRAS: Finalizing process '%s' (%d)",
	MSG_process_get_name(MSG_process_self()),MSG_process_self_PID());

  if (xbt_dynar_length(msg_pd->msg_queue))
    WARN1("process %d terminated, but some messages are still queued",
	  MSG_process_self_PID());

  for (cpt=0; cpt< XBT_MAX_CHANNEL; cpt++)
    if (myPID == hd->proc[cpt])
      hd->proc[cpt] = 0;

  xbt_dynar_foreach(hd->ports, cpt, pr) {
    if (pr.port == trp_pd->chan || pr.port == trp_pd->measChan) {
      xbt_dynar_cursor_rm(hd->ports, &cpt);
    }
  }

  if ( ! --(hd->refcount)) {
    xbt_dynar_free(&hd->ports);
    free(hd);
  }
  gras_procdata_exit();
  free(pd);
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

const char* xbt_procname(void) {
  const char *res = NULL;
  m_process_t process = MSG_process_self();
  if ((process != NULL) && (process->simdata))
    res = MSG_process_get_name(process);
  if (res) 
    return res;
  else
    return "";
}

long int gras_os_getpid(void) {
  m_process_t process = MSG_process_self();
  if ((process != NULL) && (process->simdata))
    return (long int)MSG_process_get_PID(MSG_process_self());
  else
    return (long int)0;
}
