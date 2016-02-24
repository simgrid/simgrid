/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_DATATYPE_H
#define MC_DATATYPE_H

#ifdef _XBT_WIN32
#  include <xbt/win32_ucontext.h>     /* context relative declarations */
#else
#  include <ucontext.h>           /* context relative declarations */
#endif


#include "xbt/misc.h"
#include "xbt/swag.h"
#include "xbt/fifo.h"

#if HAVE_MC
#include <libunwind.h>
#include <dwarf.h>
#endif 

SG_BEGIN_DECL()

/******************************* Transitions **********************************/

typedef struct s_mc_transition *mc_transition_t;

/*********** Structures for snapshot comparison **************************/

typedef struct s_stack_region{
  void *address;
  ucontext_t* context;
  size_t size;
  int block;
  int process_index;
}s_stack_region_t, *stack_region_t;

/************ DWARF structures *************/

SG_END_DECL()
#endif                          /* _MC_MC_H */
