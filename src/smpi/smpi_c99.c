/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/dynar.h>
#include "private.h"

typedef struct s_smpi_static {
  void *ptr;
  void_f_pvoid_t free_fn;
} s_smpi_static_t;

static xbt_dynar_t registered_static_stack = NULL;

void smpi_register_static(void* arg, void_f_pvoid_t free_fn)
{
  s_smpi_static_t elm = { arg, free_fn };
  if (!registered_static_stack)
    registered_static_stack = xbt_dynar_new(sizeof(s_smpi_static_t), NULL);
  xbt_dynar_push_as(registered_static_stack, s_smpi_static_t, elm);
}

void smpi_free_static(void)
{
  while (!xbt_dynar_is_empty(registered_static_stack)) {
    s_smpi_static_t elm =
      xbt_dynar_pop_as(registered_static_stack, s_smpi_static_t);
    elm.free_fn(elm.ptr);
  }
  xbt_dynar_free(&registered_static_stack);
}
