/* $Id$ */

/* bloc_transport - send/receive a bunch of bytes                           */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include "transport.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(transport,GRAS);


gras_error_t 
gras_trp_init(void){
  RAISE_UNIMPLEMENTED;
}

void
gras_trp_exit(void){

  ERROR1("%s not implemented",__FUNCTION__);
  abort();
}

gras_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst){

  RAISE_UNIMPLEMENTED;
}
