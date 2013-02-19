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
#include "workstation_private.h"
#include "surf/cpu_cas01_private.h"
#include "surf/maxmin_private.h"


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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");




surf_model_t surf_vm_workstation_model = NULL;

static void vm_ws_create(const char *name, void *ind_phys_workstation)
{
  workstation_VM2013_t vm_ws = xbt_new0(s_workstation_VM2013_t, 1);

  vm_ws->sub_ws = surf_workstation_resource_priv(ind_phys_workstation);
  vm_ws->current_state = SURF_VM_STATE_CREATED;


  // //// WORKSTATION  RELATED STUFF ////
  /* Create a workstation_CLM03 resource and register it to the system
     Please note that the new ws is added into the host_lib. Then,
     if you want to get a workstation_VM2013 object from host_lib, see
     ws->generic_resouce.model->type first. If it is  SURF_MODEL_TYPE_VM_WORKSTATION,
     you can cast ws to vm_ws. */

  __init_workstation_CLM03(&vm_ws->ws, name);

  // Override the model with the current VM one.
  vm_ws->ws.generic_resource.model = surf_vm_workstation_model;


  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of  the sub_cpu one.
  cpu_Cas01_t sub_cpu = surf_cpu_resource_priv(ind_phys_workstation);

  /* We can assume one core and cas01 cpu for the first step.
   * Do xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu) if you get the resource.
   * */
  cpu_cas01_create_resource(name, // name
      sub_cpu->power_peak,        // host->power_peak,
      1,                          // host->power_scale,
      NULL,                       // host->power_trace,
      1,                          // host->core_amount,
      SURF_RESOURCE_ON,           // host->initial_state,
      NULL,                       // host->state_trace,
      NULL,                       // host->properties,
      surf_cpu_model_vm);

  vm_ws->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_phys_workstation, 0); // cost 0 is okay?

  //// NET RELATED STUFF ////
  // Bind virtual net_elm to the host
  // TODO rebind each time you migrate a VM
  // TODO check how network requests are scheduled between distinct processes competing for the same card.
  // Please note that the __init_workstation_CLM03 invocation assigned NULL to ws.net_elm since no network elements
  // were previously created for this hostname. Indeed all network elements are created during the SimGrid initialization phase by considering
  // the platform file.
  vm_ws->ws.net_elm = xbt_lib_get_or_null(host_lib, vm_ws->sub_ws->generic_resource.name, ROUTING_HOST_LEVEL);
  xbt_lib_set(host_lib, name, ROUTING_HOST_LEVEL, vm_ws->ws.net_elm);

  // //// STORAGE RELATED STUFF ////
 // ind means ''indirect'' that this is a reference on the whole dict_elm structure (i.e not on the surf_resource_private infos)


}

/*
 * Update the physical host of the given VM
 */
static void vm_ws_migrate(void *ind_vm_workstation, void *ind_dest_phys_workstation)
{ 
   /* ind_phys_workstation equals to smx_host_t */
   workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_workstation);
   xbt_assert(vm_ws);

   /* do something */

   vm_ws->sub_ws = surf_workstation_resource_priv(ind_dest_phys_workstation);
}

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
static void vm_ws_destroy(void *ind_vm_workstation)
{ 
	/* ind_phys_workstation equals to smx_host_t */
	workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_workstation);
	xbt_assert(vm_ws);
	xbt_assert(vm_ws->ws.generic_resource.model == surf_vm_workstation_model);

  {
    int ret = surf_cpu_model_pm->action_unref(vm_ws->cpu_action);
    xbt_assert(ret == 1, "Bug: some resource still remains");
  }

	const char *name = vm_ws->ws.generic_resource.name;
	/* this will call surf_resource_free() */
	xbt_lib_unset(host_lib, name, SURF_WKS_LEVEL);

	xbt_free(vm_ws->ws.generic_resource.name);
	xbt_free(vm_ws);
}

static int vm_ws_get_state(void *ind_vm_ws){
	return ((workstation_VM2013_t) surf_workstation_resource_priv(ind_vm_ws))->current_state;
}

static void vm_ws_set_state(void *ind_vm_ws, int state){
	 ((workstation_VM2013_t) surf_workstation_resource_priv(ind_vm_ws))->current_state=state;
}


// TODO Please fix it (if found is wrong, nothing is returned)
static double get_solved_value(surf_action_t cpu_action)
{
  int found = 0;
  lmm_system_t pm_system = surf_workstation_model->model_private->maxmin_system;
  lmm_variable_t var = NULL;

  xbt_swag_foreach(var, &pm_system->variable_set) {
    XBT_DEBUG("var id %p id_int %d double %f", var->id, var->id_int, var->value);
    if (var->id == cpu_action) {
      found = 1;
      break;
    }
  }

  if (found)
    return var->value;

  XBT_CRITICAL("bug: cannot found the solved variable of the action %p", cpu_action);
}


static double vm_ws_share_resources(surf_model_t workstation_model, double now)
{
  /* 0. Make sure that we already calculated the resource share at the physical
   * machine layer. */
  {
    unsigned int index_of_pm_ws_model = xbt_dynar_search(model_list_invoke, surf_workstation_model);
    unsigned int index_of_vm_ws_model = xbt_dynar_search(model_list_invoke, surf_vm_workstation_model);
    xbt_assert((index_of_pm_ws_model < index_of_vm_ws_model), "Cannot assume surf_workstation_model comes before");

    /* Another option is that we call sub_ws->share_resource() here. The
     * share_resource() function has no side-effect. We can call it here to
     * ensure that. */
  }


  /* 1. Now we know how many resource should be assigned to each virtual
   * machine. We update constraints of the virtual machine layer.
   *
   *
   * If we have two virtual machine (VM1 and VM2) on a physical machine (PM1).
   *     X1 + X2 = C       (Equation 1)
   * where
   *    the resource share of VM1: X1
   *    the resource share of VM2: X2
   *    the capacity of PM1: C
   *
   * Then, if we have two process (P1 and P2) on VM1.
   *     X1_1 + X1_2 = X1  (Equation 2)
   * where
   *    the resource share of P1: X1_1
   *    the resource share of P2: X1_2
   *    the capacity of VM1: X1
   *
   * Equation 1 was solved in the physical machine layer.
   * Equation 2 is solved in the virtual machine layer (here).
   * X1 must be passed to the virtual machine laye as a constraint value.
   *
   **/

  /* iterate for all hosts including virtual machines */
  xbt_lib_cursor_t cursor;
  char *key;
  void **ind_host;
  xbt_lib_foreach(host_lib, cursor, key, ind_host) {
    workstation_CLM03_t ws_clm03 = ind_host[SURF_WKS_LEVEL];
    cpu_Cas01_t cpu_cas01 = ind_host[SURF_CPU_LEVEL];

    /* skip if it is not a virtual machine */
    if (!ws_clm03)
      continue;
    if (ws_clm03->generic_resource.model != surf_vm_workstation_model)
      continue;
    xbt_assert(cpu_cas01, "cpu-less workstation");

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    workstation_VM2013_t ws_vm2013 = (workstation_VM2013_t) ws_clm03;

    double solved_value = get_solved_value(ws_vm2013->cpu_action);
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value,
        ws_clm03->generic_resource.name, ws_vm2013->sub_ws->generic_resource.name);

    cpu_cas01->constraint->bound = solved_value;
  }


  /* 2. Calculate resource share at the virtual machine layer. */
  return ws_share_resources(workstation_model, now);
}


/*
 * A surf level object will be useless in the upper layer. Returing the name
 * will be simple and suffcient.
 **/
static const char *vm_ws_get_phys_host(void *ind_vm_ws)
{
	workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_ws);
	return vm_ws->sub_ws->generic_resource.name;
}

static void surf_vm_workstation_model_init_internal(void)
{
  surf_model_t model = surf_model_init();

  model->name = "Virtual Workstation";
  model->type = SURF_MODEL_TYPE_VM_WORKSTATION;

  model->extension.workstation.cpu_model = surf_cpu_model_vm;

  model->extension.vm_workstation.create        = vm_ws_create;
  model->extension.vm_workstation.set_state     = vm_ws_set_state;
  model->extension.vm_workstation.get_state     = vm_ws_get_state;
  model->extension.vm_workstation.migrate       = vm_ws_migrate;
  model->extension.vm_workstation.get_phys_host = vm_ws_get_phys_host;
  model->extension.vm_workstation.destroy       = vm_ws_destroy;

  model->model_private->share_resources       = vm_ws_share_resources;
  model->model_private->resource_used         = ws_resource_used;
  model->model_private->update_actions_state  = ws_update_actions_state;
  model->model_private->update_resource_state = ws_update_resource_state;
  model->model_private->finalize              = ws_finalize;

  surf_vm_workstation_model = model;
}

void surf_vm_workstation_model_init()
{
  surf_vm_workstation_model_init_internal();
  xbt_dynar_push(model_list, &surf_vm_workstation_model);
  xbt_dynar_push(model_list_invoke, &surf_vm_workstation_model);
}
