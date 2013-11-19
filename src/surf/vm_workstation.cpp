/*
 * vm_workstation.cpp
 *
 *  Created on: Nov 12, 2013
 *      Author: bedaride
 */
#include "vm_workstation.hpp"
#include "cpu_cas01.hpp"
#include "maxmin_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_vm_workstation, surf,
                                "Logging specific to the SURF VM workstation module");

WorkstationVMModelPtr surf_vm_workstation_model = NULL;

void surf_vm_workstation_model_init_current_default(void){
  if (surf_cpu_model_vm) {
    surf_vm_workstation_model = new WorkstationVMModel();
    ModelPtr model = static_cast<ModelPtr>(surf_vm_workstation_model);

    xbt_dynar_push(model_list, &model);
    xbt_dynar_push(model_list_invoke, &model);
  }
}

/*********
 * Model *
 *********/

WorkstationVMModel::WorkstationVMModel() : WorkstationModel("Virtual Workstation") {
  p_cpuModel = surf_cpu_model_vm;
}

/* ind means ''indirect'' that this is a reference on the whole dict_elm
 * structure (i.e not on the surf_resource_private infos) */

void WorkstationVMModel::createResource(const char *name, void *ind_phys_workstation)
{
  WorkstationVM2013LmmPtr ws = new WorkstationVM2013Lmm(this, name, NULL, static_cast<surf_resource_t>(ind_phys_workstation));

  xbt_lib_set(host_lib, name, SURF_WKS_LEVEL, ws);

  /* TODO:
   * - check how network requests are scheduled between distinct processes competing for the same card.
   */
}

static inline double get_solved_value(CpuActionLmmPtr cpu_action)
{
  return cpu_action->p_variable->value;
}

/* In the real world, processes on the guest operating system will be somewhat
 * degraded due to virtualization overhead. The total CPU share that these
 * processes get is smaller than that of the VM process gets on a host
 * operating system. */
// const double virt_overhead = 0.95;
const double virt_overhead = 1;

double WorkstationVMModel::shareResources(double now)
{
  /* TODO: udpate action's cost with the total cost of processes on the VM. */


  /* 0. Make sure that we already calculated the resource share at the physical
   * machine layer. */
  {
	ModelPtr ws_model = static_cast<ModelPtr>(surf_workstation_model);
	ModelPtr vm_ws_model = static_cast<ModelPtr>(surf_vm_workstation_model);
    unsigned int index_of_pm_ws_model = xbt_dynar_search(model_list_invoke, &ws_model);
    unsigned int index_of_vm_ws_model = xbt_dynar_search(model_list_invoke, &vm_ws_model);
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
    WorkstationCLM03Ptr ws_clm03 = dynamic_cast<WorkstationCLM03Ptr>(
                                   static_cast<ResourcePtr>(ind_host[SURF_WKS_LEVEL]));
    CpuCas01LmmPtr cpu_cas01 = dynamic_cast<CpuCas01LmmPtr>(
                               static_cast<ResourcePtr>(ind_host[SURF_CPU_LEVEL]));

    if (!ws_clm03)
      continue;
    /* skip if it is not a virtual machine */
    if (ws_clm03->p_model != static_cast<ModelPtr>(surf_vm_workstation_model))
      continue;
    xbt_assert(cpu_cas01, "cpu-less workstation");

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    WorkstationVM2013Ptr ws_vm2013 = dynamic_cast<WorkstationVM2013Ptr>(ws_clm03);

    double solved_value = get_solved_value(ws_vm2013->p_action);
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value,
        ws_clm03->m_name, ws_vm2013->p_subWs->m_name);

    // TODO: check lmm_update_constraint_bound() works fine instead of the below manual substitution.
    // cpu_cas01->constraint->bound = solved_value;
    xbt_assert(cpu_cas01->p_model == static_cast<ModelPtr>(surf_cpu_model_vm));
    lmm_system_t vcpu_system = cpu_cas01->p_model->p_maxminSystem;
    lmm_update_constraint_bound(vcpu_system, cpu_cas01->p_constraint, virt_overhead * solved_value);
  }


  /* 2. Calculate resource share at the virtual machine layer. */
  double ret = WorkstationModel::shareResources(now);


  /* FIXME: 3. do we have to re-initialize our cpu_action object? */
#if 0
  /* iterate for all hosts including virtual machines */
  xbt_lib_foreach(host_lib, cursor, key, ind_host) {
    WorkstationCLM03Ptr ws_clm03 = ind_host[SURF_WKS_LEVEL];

    /* skip if it is not a virtual machine */
    if (!ws_clm03)
      continue;
    if (ws_clm03->p_model != surf_vm_workstation_model)
      continue;

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    {
#if 0
      WorkstationVM2013Ptr ws_vm2013 = (workstation_VM2013_t) ws_clm03;
      XBT_INFO("cost %f remains %f start %f finish %f", ws_vm2013->cpu_action->cost,
          ws_vm2013->cpu_action->remains,
          ws_vm2013->cpu_action->start,
          ws_vm2013->cpu_action->finish
          );
#endif
#if 0
      void *ind_sub_host = xbt_lib_get_elm_or_null(host_lib, ws_vm2013->sub_ws->generic_resource.name);
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

/************
 * Resource *
 ************/

WorkstationVM2013Lmm::WorkstationVM2013Lmm(WorkstationVMModelPtr model, const char* name, xbt_dict_t props,
		                                   surf_resource_t ind_phys_workstation)
  :  WorkstationVM2013(model, name, props, NULL, NULL),
     WorkstationCLM03Lmm(model, name, props, NULL, NULL, NULL),
     WorkstationCLM03(model, name, props, NULL, NULL, NULL),
     Resource(model, name, props) {
  WorkstationCLM03Ptr sub_ws = dynamic_cast<WorkstationCLM03Ptr>(
                               static_cast<ResourcePtr>(
                                 surf_workstation_resource_priv(ind_phys_workstation)));

  /* Currently, we assume a VM has no storage. */
  p_storage = NULL;

  /* Currently, a VM uses the network resource of its physical host. In
   * host_lib, this network resource object is refered from two different keys.
   * When deregistering the reference that points the network resource object
   * from the VM name, we have to make sure that the system does not call the
   * free callback for the network resource object. The network resource object
   * is still used by the physical machine. */
  p_netElm = static_cast<RoutingEdgePtr>(xbt_lib_get_or_null(host_lib, sub_ws->m_name, ROUTING_HOST_LEVEL));
  xbt_lib_set(host_lib, name, ROUTING_HOST_LEVEL, p_netElm);

  p_subWs = sub_ws;
  p_currentState = SURF_VM_STATE_CREATED;

  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of the sub_cpu one.
  CpuCas01LmmPtr sub_cpu = dynamic_cast<CpuCas01LmmPtr>(
                   static_cast<ResourcePtr>(
                	 surf_cpu_resource_priv(ind_phys_workstation)));

  /* We can assume one core and cas01 cpu for the first step.
   * Do xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu) if you get the resource. */

  static_cast<CpuCas01ModelPtr>(surf_cpu_model_vm)->createResource(name, // name
      sub_cpu->p_powerPeakList,        // host->power_peak,
      sub_cpu->m_pstate,
      1,                          // host->power_scale,
      NULL,                       // host->power_trace,
      1,                          // host->core_amount,
      SURF_RESOURCE_ON,           // host->initial_state,
      NULL,                       // host->state_trace,
      NULL);                       // host->properties,

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* FIXME: TODO: we have to peridocally input GUESTOS_NOISE to the system? how ? */
  // vm_ws->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_phys_workstation, GUESTOS_NOISE);
  p_action = dynamic_cast<CpuActionLmmPtr>(sub_cpu->execute(0));

  /* The SURF_WKS_LEVEL at host_lib saves workstation_CLM03 objects. Please
   * note workstation_VM2013 objects, inheriting the workstation_CLM03
   * structure, are also saved there.
   *
   * If you want to get a workstation_VM2013 object from host_lib, see
   * ws->generic_resouce.model->type first. If it is
   * SURF_MODEL_TYPE_VM_WORKSTATION, you can cast ws to vm_ws. */
  XBT_INFO("Create VM(%s)@PM(%s) with %ld mounted disks", name, sub_ws->m_name, xbt_dynar_length(p_storage));
}

/*
 * A physical host does not disapper in the current SimGrid code, but a VM may
 * disapper during a simulation.
 */
WorkstationVM2013Lmm::~WorkstationVM2013Lmm()
{
  /* ind_phys_workstation equals to smx_host_t */
  surf_resource_t ind_vm_workstation = xbt_lib_get_elm_or_null(host_lib, m_name);

  /* Before clearing the entries in host_lib, we have to pick up resources. */
  CpuCas01LmmPtr cpu = dynamic_cast<CpuCas01LmmPtr>(
                    static_cast<ResourcePtr>(
       	              surf_cpu_resource_priv(ind_vm_workstation)));

  /* We deregister objects from host_lib, without invoking the freeing callback
   * of each level.
   *
   * Do not call xbt_lib_remove() here. It deletes all levels of the key,
   * including MSG_HOST_LEVEL and others. We should unregister only what we know.
   */
  xbt_lib_unset(host_lib, m_name, SURF_CPU_LEVEL, 0);
  xbt_lib_unset(host_lib, m_name, ROUTING_HOST_LEVEL, 0);
  xbt_lib_unset(host_lib, m_name, SURF_WKS_LEVEL, 0);

  /* TODO: comment out when VM stroage is implemented. */
  // xbt_lib_unset(host_lib, name, SURF_STORAGE_LEVEL, 0);


  /* Free the cpu_action of the VM. */
  int ret = p_action->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");

  /* Free the cpu resource of the VM. If using power_trace, we will have to
   * free other objects than lmm_constraint. */
  lmm_constraint_free(cpu->p_model->p_maxminSystem, cpu->p_constraint);
  {
    unsigned long i;
    for (i = 0; i < cpu->m_core; i++) {
      void *cnst_id = cpu->p_constraintCore[i]->id;
      lmm_constraint_free(cpu->p_model->p_maxminSystem, cpu->p_constraintCore[i]);
      xbt_free(cnst_id);
    }

    xbt_free(cpu->p_constraintCore);
  }

  delete cpu;

  /* Free the network resource of the VM. */
	// Nothing has to be done, because net_elmts is just a pointer on the physical one

  /* Free the storage resource of the VM. */
  // Not relevant yet

	/* Free the workstation resource of the VM. */
}

e_surf_resource_state_t WorkstationVM2013Lmm::getState()
{
  return (e_surf_resource_state_t) p_currentState;
}

void WorkstationVM2013Lmm::setState(e_surf_resource_state_t state)
{
  p_currentState = (e_surf_vm_state_t) state;
}

void WorkstationVM2013Lmm::suspend()
{
  p_action->suspend();
  p_currentState = SURF_VM_STATE_SUSPENDED;
}

void WorkstationVM2013Lmm::resume()
{
  p_action->resume();
  p_currentState = SURF_VM_STATE_RUNNING;
}

void WorkstationVM2013Lmm::save()
{
  p_currentState = SURF_VM_STATE_SAVING;

  /* FIXME: do something here */
  p_action->suspend();
  p_currentState = SURF_VM_STATE_SAVED;
}

void WorkstationVM2013Lmm::restore()
{
  p_currentState = SURF_VM_STATE_RESTORING;

  /* FIXME: do something here */
  p_action->resume();
  p_currentState = SURF_VM_STATE_RUNNING;
}

/*
 * Update the physical host of the given VM
 */
void WorkstationVM2013Lmm::migrate(surf_resource_t ind_dst_pm)
{
   /* ind_phys_workstation equals to smx_host_t */
   WorkstationCLM03Ptr ws_clm03_dst = dynamic_cast<WorkstationCLM03Ptr>(
	                                  static_cast<ResourcePtr>(
	                                   surf_workstation_resource_priv(ind_dst_pm)));
   const char *vm_name = m_name;
   const char *pm_name_src = p_subWs->m_name;
   const char *pm_name_dst = ws_clm03_dst->m_name;

   xbt_assert(ws_clm03_dst);

   /* do something */

   /* update net_elm with that of the destination physical host */
   RoutingEdgePtr old_net_elm = p_netElm;
   RoutingEdgePtr new_net_elm = static_cast<RoutingEdgePtr>(xbt_lib_get_or_null(host_lib, pm_name_dst, ROUTING_HOST_LEVEL));
   xbt_assert(new_net_elm);

   /* Unregister the current net_elm from host_lib. Do not call the free callback. */
   xbt_lib_unset(host_lib, vm_name, ROUTING_HOST_LEVEL, 0);

   /* Then, resister the new one. */
   p_netElm = new_net_elm;
   xbt_lib_set(host_lib, vm_name, ROUTING_HOST_LEVEL, p_netElm);

   p_subWs = ws_clm03_dst;

   /* Update vcpu's action for the new pm */
   {
#if 0
     XBT_INFO("cpu_action->remains %g", p_action->remains);
     XBT_INFO("cost %f remains %f start %f finish %f", p_action->cost,
         p_action->remains,
         p_action->start,
         p_action->finish
         );
     XBT_INFO("cpu_action state %d", surf_action_get_state(p_action));
#endif

     /* create a cpu action bound to the pm model at the destination. */
     CpuActionLmmPtr new_cpu_action = dynamic_cast<CpuActionLmmPtr>(
    		                            dynamic_cast<CpuCas01LmmPtr>(
                                        static_cast<ResourcePtr>(
                                        surf_cpu_resource_priv(ind_dst_pm)))->execute(0));

     e_surf_action_state_t state = surf_action_get_state(p_action);
     if (state != SURF_ACTION_DONE)
       XBT_CRITICAL("FIXME: may need a proper handling, %d", state);
     if (p_action->m_remains > 0)
       XBT_CRITICAL("FIXME: need copy the state(?), %f", p_action->m_remains);

     int ret = p_action->unref();
     xbt_assert(ret == 1, "Bug: some resource still remains");

     p_action = new_cpu_action;
   }

   XBT_DEBUG("migrate VM(%s): change net_elm (%p to %p)", vm_name, old_net_elm, new_net_elm);
   XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
}

void WorkstationVM2013Lmm::setBound(double bound){
 p_action->setBound(bound);
}

void WorkstationVM2013Lmm::setAffinity(CpuLmmPtr cpu, unsigned long mask){
 p_action->setAffinity(cpu, mask);
}

/*
 * A surf level object will be useless in the upper layer. Returing the
 * dict_elm of the host.
 **/
surf_resource_t WorkstationVM2013Lmm::getPm()
{
  return xbt_lib_get_elm_or_null(host_lib, p_subWs->m_name);
}

/* Adding a task to a VM updates the VCPU task on its physical machine. */
ActionPtr WorkstationVM2013Lmm::execute(double size)
{
  double old_cost = p_action->m_cost;
  double new_cost = old_cost + size;

  XBT_DEBUG("VM(%s)@PM(%s): update dummy action's cost (%f -> %f)",
      m_name, p_subWs->m_name,
      old_cost, new_cost);

  p_action->m_cost = new_cost;

  return WorkstationCLM03::execute(size);
}

/**********
 * Action *
 **********/

//FIME:: handle action cancel

