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

typedef struct s_mc_ignore_region{
  int block;
  int fragment;
  void *address;
  size_t size;
}s_mc_ignore_region_t, *mc_ignore_region_t;

SG_END_DECL()
#endif                          /* _MC_MC_H */
