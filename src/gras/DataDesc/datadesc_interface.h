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


/* declare in the given set */
gras_error_t gras_ddt_register(gras_set_t           *set,
			       gras_datadesc_type_t *type);

#endif /* GRAS_DATADESC_INTERFACE_H */
