/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_WORKSTATION_PRIVATE_H
#define _SURF_WORKSTATION_PRIVATE_H

#include "surf_private.h"

typedef struct workstation_link {
  surf_resource_t resource;   /* Any such object, added in a trace
				 should start by this field!!! */
                              /* Using this object with the public part of
				 resource does not make sense */
  const char *name;
  void *cpu;
  void *network_card;
} s_workstation_t, *workstation_t;

#endif				/* _SURF_WORKSTATION_PRIVATE_H */
