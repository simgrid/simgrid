/* Copyright (c) 2008-2012. Da SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_DATATYPE_H
#define MC_DATATYPE_H
#include "xbt/misc.h"
#include "xbt/swag.h"
#include "xbt/fifo.h"

SG_BEGIN_DECL()

/******************************* Transitions **********************************/

typedef struct s_mc_transition *mc_transition_t;


/*********** Structures for snapshot comparison **************************/

typedef struct s_mc_heap_ignore_region{
  int block;
  int fragment;
  void *address;
  size_t size;
}s_mc_heap_ignore_region_t, *mc_heap_ignore_region_t;

typedef struct s_mc_stack_ignore_variable{
  char *var_name;
  char *frame;
}s_mc_stack_ignore_variable_t, *mc_stack_ignore_variable_t;

typedef struct s_stack_region{
  void *address;
  char *process_name;
  void *context;
  size_t size;
}s_stack_region_t, *stack_region_t;

typedef struct s_heap_equality{
  void *address1;
  void *address2;
}s_heap_equality_t, *heap_equality_t;

void heap_equality_free_voidp(void *e);
void stack_region_free_voidp(void *s);

SG_END_DECL()
#endif                          /* _MC_MC_H */
