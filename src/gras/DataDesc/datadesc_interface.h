/* $Id$ */

/* datadesc_interface - declarations visible within GRAS, but not public    */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_INTERFACE_H
#define GRAS_DATADESC_INTERFACE_H

#include "gras_private.h"

void gras_datadesc_init(void);
void gras_datadesc_exit(void);


/* free a given ddt */
void gras_ddt_free(gras_datadesc_type_t **type);

/* declare in the given set, and retrieve afterward */
gras_error_t gras_ddt_register(gras_set_t           *set,
			       gras_datadesc_type_t *type);
gras_error_t gras_ddt_get_by_name(gras_set_t            *set,
				  const char            *name,
				  gras_datadesc_type_t **type);


/* create a type set, and bootstrap it by declaring all basic types in it */
gras_error_t
gras_dd_typeset_create(int gras_arch,
		       gras_set_t **set);


#endif /* GRAS_DATADESC_INTERFACE_H */
