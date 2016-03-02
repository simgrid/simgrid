/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_DATATYPE_H
#define MC_DATATYPE_H

#include <simgrid_config.h>
#include <xbt/base.h>

#ifdef _XBT_WIN32
#  include <xbt/win32_ucontext.h>     /* context relative declarations */
#else
#  include <ucontext.h>           /* context relative declarations */
#endif

#if HAVE_MC
#include <dwarf.h>
#endif 

SG_BEGIN_DECL()

typedef struct s_mc_transition *mc_transition_t;

typedef struct s_stack_region{
  void *address;
  ucontext_t* context;
  size_t size;
  int block;
  int process_index;
}s_stack_region_t, *stack_region_t;

SG_END_DECL()

#endif                          /* _MC_MC_H */
