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
   void *ptr = (bytes == 0 ? NULL : (void*) malloc ((size_t) bytes));
   gras_assert1(ptr, "Malloc of %ld bytes failed",bytes);
   return ptr;
}
void* gras_malloc0 (long int bytes) {
   void *ptr = (bytes == 0 ? NULL : (void*) calloc ((size_t) bytes, 1));
   gras_assert1(ptr, "Malloc of %ld bytes failed",bytes);
   return ptr;
}

void* gras_realloc (void  *memory, long int bytes) {
   if (bytes == 0) {
      gras_free(memory);
      return NULL;
   } else {
      void *ptr = (void*) realloc (memory, (size_t) bytes);
      gras_assert1(ptr, "Realloc of %ld bytes failed",bytes);
      return ptr;
   }
}
char* gras_strdup  (const char* str) {
   char *ret = (char*)strdup(str);
   gras_assert0(ret, "String duplication failed");
   return ret;
}

void  gras_free (void  *memory) {
   if (memory)
     free (memory);
}

/****
 **** Misc
 ****/

void gras_abort(void) {
   abort();
}
