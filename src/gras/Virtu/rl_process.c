/* $Id$ */

/* process_rl - GRAS process handling on real life                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Virtu/virtu_rl.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(process,GRAS);
			      
/* globals */
static gras_procdata_t *_gras_procdata = NULL;

gras_error_t gras_process_init() {
  gras_error_t errcode;

  if (!(_gras_procdata=(gras_procdata_t *)malloc(sizeof(gras_procdata_t))))
    RAISE_MALLOC;

  TRY(gras_procdata_init());
  return no_error;
}
gras_error_t gras_process_exit() {
  gras_procdata_exit();
  return no_error;
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void) {
  gras_assert0(_gras_procdata,"Run gras_process_init!");

  return _gras_procdata;
}
