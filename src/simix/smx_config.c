/* $Id$ */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

int _simix_init_status = 0; /* 0: beginning of time; 
                             1: pre-inited (cfg_set created); 
                             2: inited (running) */
xbt_cfg_t _simix_cfg_set = NULL;

/* callback of the workstation_model variable */
static void _simix_cfg_cb__workstation_model(const char *name, int pos) 
{
  char *val;

  xbt_assert0(_simix_init_status<2, "Cannot change the model after the initialization");
  
  val = xbt_cfg_get_string (_simix_cfg_set, name);
  /* New Module missing */
  xbt_assert1(!strcmp(val, "CLM03") ||
              !strcmp(val, "KCCFLN05") ||
	      !strcmp(val, "KCCFLN05_proportional") ||
	      !strcmp(val, "KCCFLN05_Vegas") ||
	      !strcmp(val, "KCCFLN05_Reno") ||
              !strcmp(val, "GTNETS"),
              "Unknown workstation model: %s (choices are: 'CLM03', 'KCCFLN05', 'KCCFLN05_proportional', 'KCCFLN05_Vegas', 'KCCFLN05_Reno' and 'GTNETS'",val);
}

/* create the config set and register what should be */
void simix_config_init(void) 
{

  if (_simix_init_status) 
    return; /* Already inited, nothing to do */

  _simix_init_status = 1;
  _simix_cfg_set = xbt_cfg_new();
  
  xbt_cfg_register (_simix_cfg_set, 
                    "workstation_model", xbt_cfgelm_string, 1,1,
                    &_simix_cfg_cb__workstation_model,NULL);
                    
  xbt_cfg_set_string(_simix_cfg_set,"workstation_model", "KCCFLN05");
}

void simix_config_finalize(void) 
{

  if (!_simix_init_status) 
    return; /* Not initialized yet. Nothing to do */

  xbt_cfg_free(&_simix_cfg_set);
  _simix_init_status = 0;
}

/** \brief Set a configuration variable
 * 
 * Currently existing configuration variable:
 *   - workstation_model (string): Model of workstation to use.  
 *     Possible values (defaults to "KCCFLN05"):
 *     - "CLM03": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + support for parallel tasks
 *     - "KCCFLN05": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file.
 * 
 * 	\param name Configuration variable name that will change.
 *	\param pa A va_list with the others parameters
 */
void SIMIX_config(const char *name, va_list pa) 
{
  if (!_simix_init_status) {
    simix_config_init();
  }
  xbt_cfg_set_vargs(_simix_cfg_set,name,pa);
}
