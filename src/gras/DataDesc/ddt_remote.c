/* $Id$ */

/* ddt_remote - Stuff needed to get datadescs about remote hosts            */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

static void gras_free_ddt(void *ddt) {
  gras_datadesc_type_t *type= (gras_datadesc_type_t *)ddt;
  
  if (type) {
    gras_ddt_free(&type);
  }
}

gras_error_t gras_ddt_register(gras_set_t           *set,
			       gras_datadesc_type_t *type) {
  return gras_set_add(set,
		      (gras_set_elm_t*)type,
                       &gras_free_ddt);

}
