
/* platf_generator.h - Public interface to the SimGrid platforms generator  */

/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_GEN_H
#define SG_PLATF_GEN_H

#include <xbt.h>

typedef enum {
  ROUTER,
  HOST,
  CLUSTER
} e_platf_node_kind;

XBT_PUBLIC(void) platf_random_seed(unsigned long seed[6]);

XBT_PUBLIC(void) platf_graph_uniform(int node_count);

#endif              /* SG_PLATF_GEN_H */
