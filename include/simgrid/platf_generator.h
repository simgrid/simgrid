
/* platf_generator.h - Public interface to the SimGrid platforms generator  */

/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_GEN_H
#define SG_PLATF_GEN_H

typedef enum {
  ROUTER,
  HOST,
  CLUSTER
} e_platf_node_kind;

typedef enum {
  UNIFORM,
  HEAVY_TAILED
} e_platf_placement;

void platf_random_seed(unsigned long seed[6]);

void platf_graph_init(int node_count, e_platf_placement placement);

#endif
