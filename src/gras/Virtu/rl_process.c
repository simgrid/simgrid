/* $Id$ */

/* process_rl - GRAS process handling on real life                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Virtu/virtu_rl.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(process,GRAS);
			      
/* globals */
static gras_process_data_t *_gras_process_data;

gras_error_t gras_process_init() {
  //  gras_error_t errcode;

  if (!(_gras_process_data=(gras_process_data_t *)malloc(sizeof(gras_process_data_t))))
    RAISE_MALLOC;

  WARNING0("Implement message queue");
  /*
  TRY(gras_dynar_new(  &(_gras_process_data->msg_queue)  ));
  TRY(gras_dynar_new(  &(_gras_process_data->cbl_list)  ));
  */

  _gras_process_data->userdata = NULL;
  return no_error;
}
gras_error_t gras_process_exit() {
  WARNING0("FIXME: not implemented (=> leaking on exit :)");
  return no_error;
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

void *gras_userdata_get(void) {
  return _gras_process_data->userdata;
}

void *gras_userdata_set(void *ud) {
  _gras_process_data->userdata = ud;
  return ud;
}
