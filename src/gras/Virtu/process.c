/* $Id$ */

/* process - GRAS process handling (common code for RL and SG)              */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "gras/transport.h"
#include "gras/datadesc.h"
#include "gras/messages.h"

#include "gras/Virtu/virtu_interface.h"
#include "gras/Msg/msg_interface.h" /* FIXME: Get rid of this cyclic */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(process,gras,"Process manipulation code");

/* **************************************************************************
 * Process data
 * **************************************************************************/
void *gras_userdata_get(void) {
  gras_procdata_t *pd=gras_procdata_get();
  return pd->userdata;
}

void gras_userdata_set(void *ud) {
  gras_procdata_t *pd=gras_procdata_get();

  pd->userdata = ud;
}

void
gras_procdata_init() {
  gras_procdata_t *pd=gras_procdata_get();
  pd->userdata  = NULL;
  pd->msg_queue = xbt_dynar_new(sizeof(gras_msg_t),     NULL);
  pd->cbl_list  = xbt_dynar_new(sizeof(gras_cblist_t *),gras_cbl_free);
  pd->sockets   = xbt_dynar_new(sizeof(gras_socket_t*), NULL);
}

void
gras_procdata_exit() {
  gras_procdata_t *pd=gras_procdata_get();

  xbt_dynar_free(&( pd->msg_queue ));
  xbt_dynar_free(&( pd->cbl_list ));
  xbt_dynar_free(&( pd->sockets ));
}

xbt_dynar_t 
gras_socketset_get(void) {
   return gras_procdata_get()->sockets;
}
