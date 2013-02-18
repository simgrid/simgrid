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


/* FIXME: Where should the VM state be defined?
 * At now, this must be synchronized with e_msg_vm_state_t */
typedef enum {
  /* created, but not yet started */
  surf_vm_state_created,

  surf_vm_state_running,
  surf_vm_state_migrating,

  /* Suspend/resume does not involve disk I/O, so we assume there is no transition states. */
  surf_vm_state_suspended,

  /* Save/restore involves disk I/O, so there should be transition states. */
  surf_vm_state_saving,
  surf_vm_state_saved,
  surf_vm_state_restoring,

} e_surf_vm_state_t;




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
} s_workstation_VM2013_t, *workstation_VM2013_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");




surf_model_t surf_vm_workstation_model = NULL;

static void vm_ws_create(const char *name, void *ind_phys_workstation)
{
  workstation_VM2013_t vm_ws = xbt_new0(s_workstation_VM2013_t, 1);

  // //// WORKSTATION  RELATED STUFF ////
  __init_workstation_CLM03(&vm_ws->ws, name, surf_vm_workstation_model);

  // Override the model with the current VM one.
  // vm_ws->ws.generic_resource.model = surf_vm_workstation_model;


  // //// CPU  RELATED STUFF ////
  // This can be done directly during the creation of the surf_vm_worsktation model if
  // we consider that there will be only one CPU model for one layer of virtualization.
  // However, if you want to provide several layers (i.e. a VM inside a VM hosted by a PM)
  // we should add a kind of table that stores the different CPU model.
  // vm_ws->ws->generic_resource.model.extension.cpu=cpu_model_cas01(VM level ? );

 // //// NET RELATED STUFF ////
  // Bind virtual net_elm to the host
  // TODO rebind each time you migrate a VM
  // TODO check how network requests are scheduled between distinct processes competing for the same card.
  vm_ws->ws.net_elm=xbt_lib_get_or_null(host_lib, vm_ws->sub_ws->generic_resource.name, ROUTING_HOST_LEVEL);
  xbt_lib_set(host_lib, name, ROUTING_HOST_LEVEL, vm_ws->ws.net_elm);

  // //// STORAGE RELATED STUFF ////

  // ind means ''indirect'' that this is a reference on the whole dict_elm structure (i.e not on the surf_resource_private infos)
  vm_ws->sub_ws = surf_workstation_resource_priv(ind_phys_workstation);
  vm_ws->current_state = surf_vm_state_created;


  /* If you want to get a workstation_VM2013 object from host_lib, see
   * ws->generic_resouce.model->type first. If it is
   * SURF_MODEL_TYPE_VM_WORKSTATION, cast ws to vm_ws. */
  xbt_lib_set(host_lib, name, SURF_WKS_LEVEL, &vm_ws->ws);



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

  // void *ind_host = xbt_lib_get_elm_or_null(host_lib, name);
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


static double vm_ws_share_resources(surf_model_t workstation_model, double now)
{
  // Can be obsolete if you right can ensure that ws_model has been code previously
  // invoke ws_share_resources on the physical_lyer: sub_ws->ws_share_resources()
  {
    unsigned int index_of_pm_ws_model = xbt_dynar_search(model_list_invoke, surf_workstation_model);
    unsigned int index_of_vm_ws_model = xbt_dynar_search(model_list_invoke, surf_vm_workstation_model);
    xbt_assert((index_of_pm_ws_model < index_of_vm_ws_model), "Cannot assume surf_workstation_model comes before");
    /* we have to make sure that the share_resource() callback of the model of the lower layer must be called in advance. */
  }

	// assign the corresponding value to X1, X2, ....

   // invoke cpu and net share resources on layer (1)
	// return min;
  return -1.0;
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
  surf_vm_workstation_model = surf_model_init();

  surf_vm_workstation_model->name = "Virtual Workstation";
  surf_vm_workstation_model->type = SURF_MODEL_TYPE_VM_WORKSTATION;

  surf_vm_workstation_model->extension.vm_workstation.basic.cpu_model = surf_cpu_model_vm;

  surf_vm_workstation_model->extension.vm_workstation.create = vm_ws_create;
  surf_vm_workstation_model->extension.vm_workstation.set_state = vm_ws_set_state;
  surf_vm_workstation_model->extension.vm_workstation.get_state = vm_ws_get_state;
  surf_vm_workstation_model->extension.vm_workstation.migrate = vm_ws_migrate;
  surf_vm_workstation_model->extension.vm_workstation.get_phys_host = vm_ws_get_phys_host;
  surf_vm_workstation_model->extension.vm_workstation.destroy = vm_ws_destroy;

  surf_vm_workstation_model->model_private->share_resources = vm_ws_share_resources;
}

void surf_vm_workstation_model_init()
{
  surf_vm_workstation_model_init_internal();
  xbt_dynar_push(model_list, &surf_vm_workstation_model);
  xbt_dynar_push(model_list_invoke, &surf_vm_workstation_model);
}
