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
#include "vm_workstation_private.h"
#include "cpu_cas01_private.h"
#include "maxmin_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");


surf_model_t surf_vm_workstation_model = NULL;

/* ind means ''indirect'' that this is a reference on the whole dict_elm
 * structure (i.e not on the surf_resource_private infos) */

static void vm_ws_create(const char *name, void *ind_phys_workstation)
{
  workstation_CLM03_t sub_ws = surf_workstation_resource_priv(ind_phys_workstation);
  const char *sub_ws_name = sub_ws->generic_resource.name;

  /* The workstation_VM2013 struct inherits the workstation_CLM03 struct. We
   * create a physical workstation resource, but specifying the size of
   * s_workstation_VM2013_t and the vm workstation model object. */
  workstation_CLM03_t ws = (workstation_CLM03_t) surf_resource_new(sizeof(s_workstation_VM2013_t),
      surf_vm_workstation_model, name, NULL);

  /* Currently, we assume a VM has no storage. */
  ws->storage = NULL;

  /* Currently, a VM uses the network resource of its physical host. In
   * host_lib, this network resource object is refered from two different keys.
   * When deregistering the reference that points the network resource object
   * from the VM name, we have to make sure that the system does not call the
   * free callback for the network resource object. The network resource object
   * is still used by the physical machine. */
  ws->net_elm = xbt_lib_get_or_null(host_lib, sub_ws_name, ROUTING_HOST_LEVEL);
  xbt_lib_set(host_lib, name, ROUTING_HOST_LEVEL, ws->net_elm);

  /* The SURF_WKS_LEVEL at host_lib saves workstation_CLM03 objects. Please
   * note workstation_VM2013 objects, inheriting the workstation_CLM03
   * structure, are also saved there. 
   *
   * If you want to get a workstation_VM2013 object from host_lib, see
   * ws->generic_resouce.model->type first. If it is
   * SURF_MODEL_TYPE_VM_WORKSTATION, you can cast ws to vm_ws. */
  XBT_INFO("Create VM(%s)@PM(%s) with %ld mounted disks", name, sub_ws_name, xbt_dynar_length(ws->storage));
  xbt_lib_set(host_lib, name, SURF_WKS_LEVEL, ws);


  /* We initialize the VM-specific members. */
  workstation_VM2013_t vm_ws = (workstation_VM2013_t) ws;
  vm_ws->sub_ws = sub_ws;
  vm_ws->current_state = SURF_VM_STATE_CREATED;



  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of the sub_cpu one.
  cpu_Cas01_t sub_cpu = surf_cpu_resource_priv(ind_phys_workstation);

  /* We can assume one core and cas01 cpu for the first step.
   * Do xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu) if you get the resource. */
  cpu_cas01_create_resource(name, // name
      sub_cpu->power_peak,        // host->power_peak,
      1,                          // host->power_scale,
      NULL,                       // host->power_trace,
      1,                          // host->core_amount,
      SURF_RESOURCE_ON,           // host->initial_state,
      NULL,                       // host->state_trace,
      NULL,                       // host->properties,
      surf_cpu_model_vm);



  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* FIXME: TODO: we have to peridocally input GUESTOS_NOISE to the system? how ? */
  // vm_ws->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_phys_workstation, GUESTOS_NOISE);
  vm_ws->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_phys_workstation, 0);


  /* TODO:
   * - check how network requests are scheduled between distinct processes competing for the same card.
   */
}

/*
 * Update the physical host of the given VM
 */
static void vm_ws_migrate(void *ind_vm, void *ind_dst_pm)
{ 
   /* ind_phys_workstation equals to smx_host_t */
   workstation_VM2013_t ws_vm2013 = surf_workstation_resource_priv(ind_vm);
   workstation_CLM03_t ws_clm03_dst = surf_workstation_resource_priv(ind_dst_pm);
   const char *vm_name = ws_vm2013->ws.generic_resource.name;
   const char *pm_name_src = ws_vm2013->sub_ws->generic_resource.name;
   const char *pm_name_dst = ws_clm03_dst->generic_resource.name;

   xbt_assert(ws_vm2013);
   xbt_assert(ws_clm03_dst);

   /* do something */

   /* update net_elm with that of the destination physical host */
   void *old_net_elm = ws_vm2013->ws.net_elm;
   void *new_net_elm = xbt_lib_get_or_null(host_lib, pm_name_dst, ROUTING_HOST_LEVEL);
   xbt_assert(new_net_elm);

   /* Unregister the current net_elm from host_lib. Do not call the free callback. */
   xbt_lib_unset(host_lib, vm_name, ROUTING_HOST_LEVEL, 0);

   /* Then, resister the new one. */
   ws_vm2013->ws.net_elm = new_net_elm;
   xbt_lib_set(host_lib, vm_name, ROUTING_HOST_LEVEL, ws_vm2013->ws.net_elm);

   ws_vm2013->sub_ws = ws_clm03_dst;

   /* Update vcpu's action for the new pm */
   {
#if 0
     XBT_INFO("cpu_action->remains %g", ws_vm2013->cpu_action->remains);
     XBT_INFO("cost %f remains %f start %f finish %f", ws_vm2013->cpu_action->cost,
         ws_vm2013->cpu_action->remains,
         ws_vm2013->cpu_action->start,
         ws_vm2013->cpu_action->finish
         );
     XBT_INFO("cpu_action state %d", surf_action_state_get(ws_vm2013->cpu_action));
#endif

     /* create a cpu action bound to the pm model at the destination. */
     surf_action_t new_cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_dst_pm, 0);

     e_surf_action_state_t state = surf_action_state_get(ws_vm2013->cpu_action);
     if (state != SURF_ACTION_DONE)
       XBT_CRITICAL("FIXME: may need a proper handling, %d", state);
     if (ws_vm2013->cpu_action->remains > 0)
       XBT_CRITICAL("FIXME: need copy the state(?), %d", ws_vm2013->cpu_action->remains);

     int ret = surf_cpu_model_pm->action_unref(ws_vm2013->cpu_action);
     xbt_assert(ret == 1, "Bug: some resource still remains");

     ws_vm2013->cpu_action = new_cpu_action;
   }

   XBT_DEBUG("migrate VM(%s): change net_elm (%p to %p)", vm_name, old_net_elm, new_net_elm);
   XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
}

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
static void vm_ws_destroy(void *ind_vm_workstation)
{ 
	/* ind_phys_workstation equals to smx_host_t */

  /* Before clearing the entries in host_lib, we have to pick up resources. */
	workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_workstation);
  cpu_Cas01_t cpu = surf_cpu_resource_priv(ind_vm_workstation);
	const char *name = vm_ws->ws.generic_resource.name;

	xbt_assert(vm_ws);
	xbt_assert(vm_ws->ws.generic_resource.model == surf_vm_workstation_model);


  /* We deregister objects from host_lib, without invoking the freeing callback
   * of each level.
   *
   * Do not call xbt_lib_remove() here. It deletes all levels of the key,
   * including MSG_HOST_LEVEL and others. We should unregister only what we know.
   */
  xbt_lib_unset(host_lib, name, SURF_CPU_LEVEL, 0);
  xbt_lib_unset(host_lib, name, ROUTING_HOST_LEVEL, 0);
  xbt_lib_unset(host_lib, name, SURF_WKS_LEVEL, 0);

  /* TODO: comment out when VM stroage is implemented. */
  // xbt_lib_unset(host_lib, name, SURF_STORAGE_LEVEL, 0);


  /* Free the cpu_action of the VM. */
  int ret = surf_cpu_model_pm->action_unref(vm_ws->cpu_action);
  xbt_assert(ret == 1, "Bug: some resource still remains");

  /* Free the cpu resource of the VM. If using power_trace, we will have to
   * free other objects than lmm_constraint. */
  surf_model_t cpu_model = cpu->generic_resource.model;
  lmm_constraint_free(cpu_model->model_private->maxmin_system, cpu->constraint);
  surf_resource_free(cpu);

  /* Free the network resource of the VM. */
	// Nothing has to be done, because net_elmts is just a pointer on the physical one

  /* Free the storage resource of the VM. */
  // Not relevant yet

	/* Free the workstation resource of the VM. */
  surf_resource_free(vm_ws);
}

static int vm_ws_get_state(void *ind_vm_ws)
{
	return ((workstation_VM2013_t) surf_workstation_resource_priv(ind_vm_ws))->current_state;
}

static void vm_ws_set_state(void *ind_vm_ws, int state)
{
	 ((workstation_VM2013_t) surf_workstation_resource_priv(ind_vm_ws))->current_state = state;
}

static void vm_ws_suspend(void *ind_vm_ws)
{
  workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_ws);

  surf_action_suspend(vm_ws->cpu_action);

  vm_ws->current_state = SURF_VM_STATE_SUSPENDED;
}

static void vm_ws_resume(void *ind_vm_ws)
{
  workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_ws);

  surf_action_resume(vm_ws->cpu_action);

  vm_ws->current_state = SURF_VM_STATE_RUNNING;
}

static void vm_ws_save(void *ind_vm_ws)
{
  workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_ws);

  vm_ws->current_state = SURF_VM_STATE_SAVING;

  /* FIXME: do something here */
  surf_action_suspend(vm_ws->cpu_action);

  vm_ws->current_state = SURF_VM_STATE_SAVED;
}

static void vm_ws_restore(void *ind_vm_ws)
{
  workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_ws);

  vm_ws->current_state = SURF_VM_STATE_RESTORING;

  /* FIXME: do something here */
  surf_action_resume(vm_ws->cpu_action);

  vm_ws->current_state = SURF_VM_STATE_RUNNING;
}


static double get_solved_value(surf_action_t cpu_action)
{
  int found = 0;
  /* NOTE: Do not use surf_workstation_model's maxmin_system. It is not used. */
  lmm_system_t pm_system = surf_cpu_model_pm->model_private->maxmin_system;
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
  DIE_IMPOSSIBLE;
  return -1; /* NOT REACHED */
}



/* In the real world, processes on the guest operating system will be somewhat
 * degraded due to virtualization overhead. The total CPU share that these
 * processes get is smaller than that of the VM process gets on a host
 * operating system. */
const double virt_overhead = 0.95;

static double vm_ws_share_resources(surf_model_t workstation_model, double now)
{
  /* TODO: udpate action's cost with the total cost of processes on the VM. */


  /* 0. Make sure that we already calculated the resource share at the physical
   * machine layer. */
  {
    unsigned int index_of_pm_ws_model = xbt_dynar_search(model_list_invoke, &surf_workstation_model);
    unsigned int index_of_vm_ws_model = xbt_dynar_search(model_list_invoke, &surf_vm_workstation_model);
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

    if (!ws_clm03)
      continue;
    /* skip if it is not a virtual machine */
    if (ws_clm03->generic_resource.model != surf_vm_workstation_model)
      continue;
    xbt_assert(cpu_cas01, "cpu-less workstation");

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    workstation_VM2013_t ws_vm2013 = (workstation_VM2013_t) ws_clm03;

    double solved_value = get_solved_value(ws_vm2013->cpu_action);
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value,
        ws_clm03->generic_resource.name, ws_vm2013->sub_ws->generic_resource.name);

    // TODO: check lmm_update_constraint_bound() works fine instead of the below manual substitution.
    // cpu_cas01->constraint->bound = solved_value;
    surf_model_t cpu_model = cpu_cas01->generic_resource.model;
    xbt_assert(cpu_model == surf_cpu_model_vm);
    lmm_system_t vcpu_system = cpu_model->model_private->maxmin_system;
    lmm_update_constraint_bound(vcpu_system, cpu_cas01->constraint, virt_overhead * solved_value);
  }


  /* 2. Calculate resource share at the virtual machine layer. */
  double ret = ws_share_resources(workstation_model, now);


  /* FIXME: 3. do we have to re-initialize our cpu_action object? */
#if 1
  /* iterate for all hosts including virtual machines */
  xbt_lib_foreach(host_lib, cursor, key, ind_host) {
    workstation_CLM03_t ws_clm03 = ind_host[SURF_WKS_LEVEL];

    /* skip if it is not a virtual machine */
    if (!ws_clm03)
      continue;
    if (ws_clm03->generic_resource.model != surf_vm_workstation_model)
      continue;

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    workstation_VM2013_t ws_vm2013 = (workstation_VM2013_t) ws_clm03;
    {
      void *ind_sub_host = xbt_lib_get_elm_or_null(host_lib, ws_vm2013->sub_ws->generic_resource.name);
      XBT_INFO("cost %f remains %f start %f finish %f", ws_vm2013->cpu_action->cost,
          ws_vm2013->cpu_action->remains,
          ws_vm2013->cpu_action->start,
          ws_vm2013->cpu_action->finish
          );

#if 0
      surf_cpu_model_pm->action_unref(ws_vm2013->cpu_action);
      /* FIXME: this means busy loop? */
      // ws_vm2013->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_sub_host, GUESTOS_NOISE);
      ws_vm2013->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_sub_host, 0);
#endif

    }
  }
#endif


  return ret;
}


/*
 * A surf level object will be useless in the upper layer. Returing the
 * dict_elm of the host.
 **/
static void *vm_ws_get_pm(void *ind_vm_ws)
{
	workstation_VM2013_t vm_ws = surf_workstation_resource_priv(ind_vm_ws);
  const char *sub_ws_name = vm_ws->sub_ws->generic_resource.name;

  return xbt_lib_get_elm_or_null(host_lib, sub_ws_name);
}


/* Adding a task to a VM updates the VCPU task on its physical machine. */
surf_action_t vm_ws_execute(void *workstation, double size)
{
  surf_resource_t ws = ((surf_resource_t) surf_workstation_resource_priv(workstation));

  xbt_assert(ws->model->type == SURF_MODEL_TYPE_VM_WORKSTATION);
  workstation_VM2013_t vm_ws = (workstation_VM2013_t) ws;

  double old_cost = vm_ws->cpu_action->cost;
  double new_cost = old_cost + size;

  XBT_INFO("VM(%s)@PM(%s): update dummy action's cost (%f -> %f)",
      ws->name, vm_ws->sub_ws->generic_resource.name,
      old_cost, new_cost);

  vm_ws->cpu_action->cost = new_cost;

  return ws_execute(workstation, size);
}

static void vm_ws_action_cancel(surf_action_t action)
{
  XBT_CRITICAL("FIXME: Not yet implemented. Reduce dummy action's cost by %f", action->cost);

  ws_action_cancel(action);
}


static void surf_vm_workstation_model_init_internal(void)
{
  surf_model_t model = surf_model_init();

  model->name = "Virtual Workstation";
  model->type = SURF_MODEL_TYPE_VM_WORKSTATION;

  model->action_unref     = ws_action_unref;
  model->action_cancel    = vm_ws_action_cancel;
  // model->action_state_set = ws_action_state_set;


  model->model_private->share_resources       = vm_ws_share_resources;
  model->model_private->resource_used         = ws_resource_used;
  model->model_private->update_actions_state  = ws_update_actions_state;
  model->model_private->update_resource_state = ws_update_resource_state;
  model->model_private->finalize              = ws_finalize;


  /* operation for an action, not for VM it self */
  model->suspend          = ws_action_suspend;
  model->resume           = ws_action_resume;
//   model->is_suspended     = ws_action_is_suspended;
//   model->set_max_duration = ws_action_set_max_duration;
  model->set_priority     = ws_action_set_priority;
// #ifdef HAVE_TRACING
//   model->set_category     = ws_action_set_category;
// #endif
  model->get_remains      = ws_action_get_remains;
// #ifdef HAVE_LATENCY_BOUND_TRACKING
//   model->get_latency_limited = ws_get_latency_limited;
// #endif







  xbt_assert(surf_cpu_model_vm);
  model->extension.workstation.cpu_model = surf_cpu_model_vm;

  model->extension.workstation.execute   = vm_ws_execute;
  model->extension.workstation.sleep     = ws_action_sleep;
  model->extension.workstation.get_state = ws_get_state;
  // model->extension.workstation.get_speed = ws_get_speed;
  // model->extension.workstation.get_available_speed = ws_get_available_speed;

  // model->extension.workstation.communicate           = ws_communicate;
  // model->extension.workstation.get_route             = ws_get_route;
  // model->extension.workstation.execute_parallel_task = ws_execute_parallel_task;
  // model->extension.workstation.get_link_bandwidth    = ws_get_link_bandwidth;
  // model->extension.workstation.get_link_latency      = ws_get_link_latency;
  // model->extension.workstation.link_shared           = ws_link_shared;
  // model->extension.workstation.get_properties        = ws_get_properties;

  // model->extension.workstation.open   = ws_action_open;
  // model->extension.workstation.close  = ws_action_close;
  // model->extension.workstation.read   = ws_action_read;
  // model->extension.workstation.write  = ws_action_write;
  // model->extension.workstation.stat   = ws_action_stat;
  // model->extension.workstation.unlink = ws_action_unlink;
  // model->extension.workstation.ls     = ws_action_ls;


  model->extension.vm_workstation.create        = vm_ws_create;
  model->extension.vm_workstation.set_state     = vm_ws_set_state;
  model->extension.vm_workstation.get_state     = vm_ws_get_state;
  model->extension.vm_workstation.migrate       = vm_ws_migrate;
  model->extension.vm_workstation.destroy       = vm_ws_destroy;
  model->extension.vm_workstation.suspend       = vm_ws_suspend;
  model->extension.vm_workstation.resume        = vm_ws_resume;
  model->extension.vm_workstation.save          = vm_ws_save;
  model->extension.vm_workstation.restore       = vm_ws_restore;
  model->extension.vm_workstation.get_pm        = vm_ws_get_pm;

  model->extension.workstation.set_params    = ws_set_params;
  model->extension.workstation.get_params    = ws_get_params;

  surf_vm_workstation_model = model;
}

void surf_vm_workstation_model_init(void)
{
  surf_vm_workstation_model_init_internal();
  xbt_dynar_push(model_list, &surf_vm_workstation_model);
  xbt_dynar_push(model_list_invoke, &surf_vm_workstation_model);
}
