/* $Id$ */

/* gras/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

#include <stdlib.h>

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(sysdep, gros, "System dependency");

/****
 **** Memory management
 ****/

void* gras_malloc  (long int bytes) {
   return bytes == 0 ? NULL : (void*) malloc ((size_t) bytes);
}
void* gras_malloc0 (long int bytes) {
   return bytes == 0 ? NULL : (void*) calloc ((size_t) bytes, 1);
}

void* gras_realloc (void  *memory, long int bytes) {
   if (bytes == 0) {
      gras_free(memory);
      return NULL;
   } else {
      return (void*) realloc (memory, (size_t) bytes);
   }
}

void  gras_free (void  *memory) {
   if (memory) {
      free (memory);
   }
}

/****
 **** Misc
 ****/

void gras_abort(void) {
   abort();
}
