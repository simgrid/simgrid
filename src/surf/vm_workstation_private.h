/* Copyright (c) 2009, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef VM_WS_PRIVATE_H_
#define VM_WS_PRIVATE_H_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

#include "workstation_private.h"

/* NOTE:
 * The workstation_VM2013 struct includes the workstation_CLM03 struct in
 * its first member. The workstation_VM2013_t struct inherites all
 * characteristics of the workstation_CLM03 struct. So, we can treat a
 * workstation_VM2013 object as a workstation_CLM03 if necessary.
 **/
typedef struct workstation_VM2013 {
  s_workstation_CLM03_t ws;    /* a VM is a ''v''host */

  /* The workstation object of the lower layer */
  workstation_CLM03_t sub_ws;  // Pointer to the ''host'' OS

  e_surf_vm_state_t current_state;


  surf_action_t cpu_action;

} s_workstation_VM2013_t, *workstation_VM2013_t;


void surf_vm_workstation_model_init(void);

#endif /* VM_WS_PRIVATE_H_ */
