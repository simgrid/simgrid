/* $Id$ */

/* process_sg - GRAS process handling on simulator                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras_modinter.h" /* module initialization interface */
#include "gras_simix/Virtu/gras_simix_virtu_sg.h"
#include "gras_simix/Msg/gras_simix_msg_interface.h" /* For some checks at simulation end */
#include "gras_simix/Transport/gras_simix_transport_interface.h" /* For some checks at simulation end */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_virtu_process);

static long int PID = 1;

void
gras_process_init() {
  gras_hostdata_t *hd=(gras_hostdata_t *)SIMIX_host_get_data(SIMIX_host_self());
  gras_procdata_t *pd=xbt_new0(gras_procdata_t,1);
  gras_trp_procdata_t trp_pd;

  SIMIX_process_set_data(SIMIX_process_self(),(void*)pd);


  gras_procdata_init();

  if (!hd) {
    /* First process on this host */
    hd=xbt_new(gras_hostdata_t,1);
    hd->refcount = 1;
    hd->ports = xbt_dynar_new(sizeof(gras_sg_portrec_t),NULL);
		SIMIX_host_set_data(SIMIX_host_self(),(void*)hd);
  } else {
    hd->refcount++;
  }

	trp_pd = (gras_trp_procdata_t)gras_libdata_by_name("gras_trp");
	pd->pid = PID++;

	if (SIMIX_process_self() != NULL ) {
		pd->ppid = gras_os_getpid();
	}
	else pd->ppid = -1; 

	trp_pd->mutex = SIMIX_mutex_init();
	trp_pd->cond = SIMIX_cond_init();
	trp_pd->active_socket = xbt_fifo_new();

  VERB2("Creating process '%s' (%ld)",
	   SIMIX_process_get_name(SIMIX_process_self()),
	   gras_os_getpid());
}

void
gras_process_exit() {
	xbt_dynar_t sockets = ((gras_trp_procdata_t) gras_libdata_by_name("gras_trp"))->sockets;
  gras_socket_t sock_iter;
  int cursor;
  gras_hostdata_t *hd=(gras_hostdata_t *)SIMIX_host_get_data(SIMIX_host_self());
  gras_procdata_t *pd=(gras_procdata_t*)SIMIX_process_get_data(SIMIX_process_self());

  gras_msg_procdata_t msg_pd=(gras_msg_procdata_t)gras_libdata_by_name("gras_msg");
  gras_trp_procdata_t trp_pd=(gras_trp_procdata_t)gras_libdata_by_name("gras_trp");

	SIMIX_mutex_destroy(trp_pd->mutex);
	SIMIX_cond_destroy(trp_pd->cond);
	xbt_fifo_free(trp_pd->active_socket);
  //int myPID=gras_os_getpid();
  //int cpt;
  //gras_sg_portrec_t pr;

  xbt_assert0(hd,"Run gras_process_init (ie, gras_init)!!");

  VERB2("GRAS: Finalizing process '%s' (%ld)",
	SIMIX_process_get_name(SIMIX_process_self()),gras_os_getpid());

  if (xbt_dynar_length(msg_pd->msg_queue))
    WARN1("process %ld terminated, but some messages are still queued",
	  gras_os_getpid());

	/* if each process has its sockets list, we need to close them when the process finish */
	xbt_dynar_foreach(sockets,cursor,sock_iter) {
		VERB1("Closing the socket %p left open on exit. Maybe a socket leak?",
				sock_iter);
		gras_socket_close(sock_iter);
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
    (gras_procdata_t *)SIMIX_process_get_data(SIMIX_process_self());

  xbt_assert0(pd,"Run gras_process_init! (ie, gras_init)");

  return pd;
}
void *
gras_libdata_by_name_from_remote(const char *name, smx_process_t p) {
  gras_procdata_t *pd=
    (gras_procdata_t *)SIMIX_process_get_data(p);

  xbt_assert2(pd,"process '%s' on '%s' didn't run gras_process_init! (ie, gras_init)", 
	      SIMIX_process_get_name(p),SIMIX_host_get_name(SIMIX_process_get_host(p)));
   
  return gras_libdata_by_name_from_procdata(name, pd);
}   
  
const char* xbt_procname(void) {
  const char *res = NULL;
  smx_process_t process = SIMIX_process_self();
  if ((process != NULL) && (process->simdata))
    res = SIMIX_process_get_name(process);
  if (res) 
    return res;
  else
    return "";
}

long int gras_os_getpid(void) {

  smx_process_t process = SIMIX_process_self();
	
  if ((process != NULL) && (process->data))
		return ((gras_procdata_t*)process->data)->pid;
  else
    return 0;
}
