/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "portable.h"
#include "surf_private.h"
#include "surf/surf_resource.h"
#include "simgrid/sg_config.h"

typedef struct workstation_VM2013 {
  s_surf_resource_t generic_resource;   /* Must remain first to add this to a trace */
  surf_resource_t physical_workstation;  // Pointer to the host OS
  e_msg_vm_state_t current_state;  	     // See include/msg/datatypes.h
} s_workstation_VM2013_t, *workstation_VM2013_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");

surf_model_t surf_vm_workstation_model = NULL;

static void vm_ws_create (const char *name, void *phys_workstation)
{
  workstation_VM2013_t vm_ws = xbt_new0(s_workstation_VM2013_t, 1);
// TODO Complete the surf vm workstation model
  vm_ws->generic_resource.model = surf_vm_workstation_model;
  vm_ws->generic_resource.name = xbt_strdup(name);
  vm_ws->physical_workstation = phys_workstation;
  vm_ws->current_state=msg_vm_state_created,
  xbt_lib_set(host_lib, name, SURF_WKS_LEVEL, vm_ws);
}
/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
static void vm_ws_destroy(const char *name)
{
	workstation_VM2013_t workstation = xbt_lib_get_or_null(host_lib, name, SURF_WKS_LEVEL);
	xbt_assert(workstation);
	xbt_assert(workstation->generic_resource.model == surf_vm_workstation_model);

	xbt_free(workstation->generic_resource.name);

	/* not defined yet, but we should have  */
	/* this will call surf_resource_free() */
	xbt_lib_unset(host_lib, name, SURF_WKS_LEVEL);

	xbt_free(workstation);
}

static int vm_ws_get_state(void *vm_ws){
	return ((workstation_VM2013_t)vm_ws)->current_state;
}

static void vm_ws_set_state(void *vm_ws, int state){
	 ((workstation_VM2013_t)vm_ws)->current_state=state;
}
static void surf_vm_workstation_model_init_internal(void)
{
  surf_vm_workstation_model = surf_model_init();

  surf_vm_workstation_model->name = "Virtual Workstation";

  surf_vm_workstation_model->extension.vm_workstation.create = vm_ws_create;
  surf_vm_workstation_model->extension.vm_workstation.set_state = vm_ws_set_state;
  surf_vm_workstation_model->extension.vm_workstation.get_state = vm_ws_get_state;
  surf_vm_workstation_model->extension.vm_workstation.destroy = vm_ws_destroy;

}

void surf_vm_workstation_model_init()
{
  surf_vm_workstation_model_init_internal();
  xbt_dynar_push(model_list, &surf_vm_workstation_model);
}
