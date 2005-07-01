/* $Id$ */

/* msg_config.c - support for MSG user configuration                        */

/* Copyright (c) 2005 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/error.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_cfg, msg,
				"Configuration support in \ref MSG_API");


int _msg_init_status = 0; /* 0: beginning of time; 
                             1: pre-inited (cfg_set created); 
                             2: inited (running) */
xbt_cfg_t _msg_cfg_set = NULL;

/* callback of the surf_workstation_model variable */
static void _msg_cfg_cb__surf_workstation_model(const char *name, int pos) {
  xbt_error_t errcode;
  char *val;

  xbt_assert0(_msg_init_status<2, "Cannot change the model after the initialization");
  
  TRYFAIL(xbt_cfg_get_string (_msg_cfg_set, name, &val));
  
  xbt_assert1(!strcmp(val, "CLM03") ||
              !strcmp(val, "KCCFLN05"),
              "Unknown workstation model: %s (either 'CLM03' or 'KCCFLN05'",val);
}

/* create the config set and register what should be */
void msg_config_init(void) {
  xbt_error_t errcode;

  if (_msg_init_status) 
    return; /* Already inited, nothing to do */

  _msg_init_status = 1;
  _msg_cfg_set = xbt_cfg_new();
  
  xbt_cfg_register (_msg_cfg_set, 
                    "surf_workstation_model", xbt_cfgelm_string, 1,1,
                    &_msg_cfg_cb__surf_workstation_model,NULL);
                    
  TRYFAIL(xbt_cfg_set_string(_msg_cfg_set,"surf_workstation_model", "CLM03"));
}

/** \brief set a configuration variable
 * 
 * Currently existing configuation variable:
 *   - surf_workstation_model (string): Model of workstation to use.  
 *     Possible values (defaults to "CLM03"):
 *     - "CLM03": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03])
 *     - "KCCFLN05": simple network model (no latency) but interference 
 *       between computations and communications (UNSTABLE, DONT USE)
 * 
 * Example:
 * MSG_config("surf_workstation_model","CLM03");
 */
xbt_error_t
MSG_config(const char *name, ...) {
  va_list pa;
  xbt_error_t errcode;
    
  if (!_msg_init_status) {
    msg_config_init();
  }
xbt_cfg_dump("msg_cfg_set","",_msg_cfg_set);
  va_start(pa,name);
  errcode=xbt_cfg_set_vargs(_msg_cfg_set,name,pa);
  va_end(pa);
  
  return errcode;
}
