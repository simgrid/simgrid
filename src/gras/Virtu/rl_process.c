/* $Id$ */

/* process_rl - GRAS process handling on real life                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Virtu/virtu_rl.h"

XBT_LOG_EXTERNAL_CATEGORY(process);
XBT_LOG_DEFAULT_CATEGORY(process);

/* globals */
static gras_procdata_t *_gras_procdata = NULL;

xbt_error_t gras_process_init() {
  _gras_procdata=xbt_new(gras_procdata_t,1);
  gras_procdata_init();
  return no_error;
}
xbt_error_t gras_process_exit() {
  gras_procdata_exit();
  return no_error;
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void) {
  xbt_assert0(_gras_procdata,"Run gras_process_init!");

  return _gras_procdata;
}
