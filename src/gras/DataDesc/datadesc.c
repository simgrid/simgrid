/* $Id$ */

/* datadesc - data description in order to send/recv it in GRAS             */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(DataDesc,GRAS);
/* FIXME: make this machine-dependent using a trick such as UserData*/
gras_set_t *gras_datadesc_set_local=NULL;

/**
 * gras_datadesc_init:
 *
 * Initialize the datadesc module
 **/
void
gras_datadesc_init(void) {
  gras_error_t errcode;

  VERB0("Initializing DataDesc");
  errcode = gras_set_new(&gras_datadesc_set_local);
  gras_assert0(errcode==no_error,
	       "Impossible to create the data set containg locally known types");
}

/**
 * gras_datadesc_exit:
 *
 * Finalize the datadesc module
 **/
void
gras_datadesc_exit(void) {
  VERB0("Exiting DataDesc");
  gras_set_free(&gras_datadesc_set_local);
}


/***
 *** BOOTSTRAPING FUNCTIONS: known within GRAS only
 ***/
