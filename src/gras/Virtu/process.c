/* $Id$ */

/* process - GRAS process handling (common code for RL and SG)              */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include "Virtu/virtu_interface.h"
#include "Msg/msg_interface.h" /* FIXME: Get rid of this cyclic */

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

gras_error_t
gras_procdata_init() {
  gras_error_t errcode;
  gras_procdata_t *pd=gras_procdata_get();
  pd->userdata = NULL;
  TRY(gras_dynar_new(&(pd->msg_queue), sizeof(gras_msg_t),     NULL));
  TRY(gras_dynar_new(&(pd->cbl_list),  sizeof(gras_cblist_t *),gras_cbl_free));
  TRY(gras_dynar_new(&(pd->sockets),   sizeof(gras_socket_t*), NULL));
  return no_error;
}

void
gras_procdata_exit() {
  gras_procdata_t *pd=gras_procdata_get();

  gras_dynar_free(pd->msg_queue);
  gras_dynar_free(pd->cbl_list);
  gras_dynar_free(pd->sockets);
}

gras_dynar_t *
gras_socketset_get(void) {
   return gras_procdata_get()->sockets;
}
