/* Copyright (c) 2009, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef WS_PRIVATE_H_
#define WS_PRIVATE_H_
#include "surf_private.h"
typedef struct workstation_CLM03 {
  s_surf_resource_t generic_resource;   /* Must remain first to add this to a trace */
  void *net_elm;
  xbt_dynar_t storage;
} s_workstation_CLM03_t, *workstation_CLM03_t;

void __init_ws(workstation_CLM03_t ws,  const char *id, int level);

#endif /* WS_PRIVATE_H_ */
