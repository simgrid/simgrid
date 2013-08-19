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

typedef struct s_stack_region{
  void *address;
  char *process_name;
  void *context;
  size_t size;
  int block;
}s_stack_region_t, *stack_region_t;

void heap_ignore_region_free(mc_heap_ignore_region_t r);
void heap_ignore_region_free_voidp(void *r);

/************ DWARF structures *************/

typedef enum{
  e_dw_base_type = 0,
  e_dw_enumeration_type,
  e_dw_enumerator,
  e_dw_typedef,
  e_dw_const_type,
  e_dw_array_type,
  e_dw_pointer_type,
  e_dw_structure_type,
  e_dw_union_type,
  e_dw_subroutine_type,
  e_dw_volatile_type
}e_dw_type_type;

typedef struct s_dw_type{
  e_dw_type_type type;
  void *id;
  char *name;
  int size;
  char *dw_type_id;
  xbt_dynar_t members; /* if DW_TAG_structure_type */
  int is_pointer_type;
  int offset;
}s_dw_type_t, *dw_type_t;

char* get_type_description(xbt_dict_t types, char *type_name);

SG_END_DECL()
#endif                          /* _MC_MC_H */
