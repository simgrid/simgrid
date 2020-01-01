/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_DATATYPE_H
#define MC_DATATYPE_H

#include <src/internal_config.h>
#include <xbt/base.h>

#if HAVE_UCONTEXT_H
#include <ucontext.h>
#endif

SG_BEGIN_DECL

struct s_stack_region{
  void *address;
#if HAVE_UCONTEXT_H
  ucontext_t* context;
#endif
  size_t size;
  int block;
};
typedef struct s_stack_region  s_stack_region_t;

SG_END_DECL

#endif
