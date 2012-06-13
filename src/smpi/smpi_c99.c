/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/dynar.h>
#include "private.h"

static xbt_dynar_t registered_static_stack = NULL;

void smpi_register_static(void* arg)
{
  if (!registered_static_stack)
    registered_static_stack = xbt_dynar_new(sizeof(void*), NULL);
  xbt_dynar_push_as(registered_static_stack, void*, arg);
}

void smpi_free_static(void)
{
  while (!xbt_dynar_is_empty(registered_static_stack)) {
    void *p = xbt_dynar_pop_as(registered_static_stack, void*);
    free(p);
  }
  xbt_dynar_free(&registered_static_stack);
}
