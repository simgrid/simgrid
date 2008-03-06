/* $Id$ */

/* base - several addons to do specific stuff not in GRAS itself            */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "amok/base.h"

XBT_LOG_NEW_SUBCATEGORY(amok,XBT_LOG_ROOT_CAT,"All AMOK categories");

void amok_base_init(void) {
  gras_datadesc_type_t host_desc;

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(amok_bw, amok);
    XBT_LOG_CONNECT(amok_bw_sat, amok_bw);
  XBT_LOG_CONNECT(amok_pm, amok);
   
  /* Build the datatype descriptions */
  host_desc = gras_datadesc_struct("s_xbt_host_t");
  gras_datadesc_struct_append(host_desc,"name",gras_datadesc_by_name("string"));
  gras_datadesc_struct_append(host_desc,"exp_size",gras_datadesc_by_name("int"));
  gras_datadesc_struct_close(host_desc);
  host_desc = gras_datadesc_ref("xbt_host_t",host_desc);
}

void amok_base_exit(void) {
   /* FIXME: No real module mechanism in GRAS so far, nothing to do. */
}

