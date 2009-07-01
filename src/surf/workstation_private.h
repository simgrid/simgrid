/* 	$Id$ */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_WORKSTATION_PRIVATE_H
#define _SURF_WORKSTATION_PRIVATE_H

#include "surf_private.h"

typedef struct workstation_CLM03 {
  s_surf_resource_t generic_resource; /* Must remain first to add this to a trace */
  void *cpu;
  int id;
} s_workstation_CLM03_t, *workstation_CLM03_t;

typedef struct surf_action_parallel_task_CSL05 {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
  double rate;
  int suspended;
} s_surf_action_parallel_task_CSL05_t, *surf_action_parallel_task_CSL05_t;

#endif /* _SURF_WORKSTATION_PRIVATE_H */
