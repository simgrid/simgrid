/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "vm_workstation_hl13.hpp"
#include "cpu_cas01.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_vm_workstation);

void surf_vm_workstation_model_init_HL13(void){
  if (surf_cpu_model_vm) {
    surf_vm_workstation_model = new WorkstationVMHL13Model();
    ModelPtr model = surf_vm_workstation_model;

    xbt_dynar_push(model_list, &model);
    xbt_dynar_push(model_list_invoke, &model);
  }
}

/*********
 * Model *
 *********/

WorkstationVMHL13Model::WorkstationVMHL13Model() : WorkstationVMModel() {
  p_cpuModel = surf_cpu_model_vm;
}

void WorkstationVMHL13Model::updateActionsState(double /*now*/, double /*delta*/){
  return;
}

ActionPtr WorkstationVMHL13Model::communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate){
  return surf_network_model->communicate(src->p_netElm, dst->p_netElm, size, rate);
}

/* ind means ''indirect'' that this is a reference on the whole dict_elm
 * structure (i.e not on the surf_resource_private infos) */

WorkstationVMPtr WorkstationVMHL13Model::createWorkstationVM(const char *name, surf_resource_t ind_phys_workstation)
{
  WorkstationVMHL13Ptr ws = new WorkstationVMHL13(this, name, NULL, ind_phys_workstation);

  xbt_lib_set(host_lib, name, SURF_WKS_LEVEL, ws);

  /* TODO:
   * - check how network requests are scheduled between distinct processes competing for the same card.
   */
  return ws;
}

static inline double get_solved_value(CpuActionPtr cpu_action)
{
  return cpu_action->getVariable()->value;
}

/* In the real world, processes on the guest operating system will be somewhat
 * degraded due to virtualization overhead. The total CPU share that these
 * processes get is smaller than that of the VM process gets on a host
 * operating system. */
// const double virt_overhead = 0.95;
const double virt_overhead = 1;

double WorkstationVMHL13Model::shareResources(double now)
{
  /* TODO: udpate action's cost with the total cost of processes on the VM. */


  /* 0. Make sure that we already calculated the resource share at the physical
   * machine layer. */
  {
    _XBT_GNUC_UNUSED ModelPtr ws_model = surf_workstation_model;
    _XBT_GNUC_UNUSED ModelPtr vm_ws_model = surf_vm_workstation_model;
    _XBT_GNUC_UNUSED unsigned int index_of_pm_ws_model = xbt_dynar_search(model_list_invoke, &ws_model);
    _XBT_GNUC_UNUSED unsigned int index_of_vm_ws_model = xbt_dynar_search(model_list_invoke, &vm_ws_model);
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

  /* iterate for all virtual machines */
  for (WorkstationVMModel::vm_list_t::iterator iter =
         WorkstationVMModel::ws_vms.begin();
       iter !=  WorkstationVMModel::ws_vms.end(); ++iter) {

    WorkstationVMPtr ws_vm = &*iter;
    CpuPtr cpu = ws_vm->p_cpu;
    xbt_assert(cpu, "cpu-less workstation");

    double solved_value = get_solved_value(ws_vm->p_action);
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value,
        ws_vm->getName(), ws_vm->p_subWs->getName());

    // TODO: check lmm_update_constraint_bound() works fine instead of the below manual substitution.
    // cpu_cas01->constraint->bound = solved_value;
    xbt_assert(cpu->getModel() == surf_cpu_model_vm);
    lmm_system_t vcpu_system = cpu->getModel()->getMaxminSystem();
    lmm_update_constraint_bound(vcpu_system, cpu->getConstraint(), virt_overhead * solved_value);
  }


  /* 2. Calculate resource share at the virtual machine layer. */
  adjustWeightOfDummyCpuActions();

  double min_by_cpu = p_cpuModel->shareResources(now);
  double min_by_net = (strcmp(surf_network_model->getName(), "network NS3")) ? surf_network_model->shareResources(now) : -1;
  double min_by_sto = -1;
  if (p_cpuModel == surf_cpu_model_pm)
	min_by_sto = surf_storage_model->shareResources(now);

  XBT_DEBUG("model %p, %s min_by_cpu %f, %s min_by_net %f, %s min_by_sto %f",
      this, surf_cpu_model_pm->getName(), min_by_cpu,
            surf_network_model->getName(), min_by_net,
            surf_storage_model->getName(), min_by_sto);

  double ret = max(max(min_by_cpu, min_by_net), min_by_sto);
  if (min_by_cpu >= 0.0 && min_by_cpu < ret)
	ret = min_by_cpu;
  if (min_by_net >= 0.0 && min_by_net < ret)
	ret = min_by_net;
  if (min_by_sto >= 0.0 && min_by_sto < ret)
	ret = min_by_sto;

  /* FIXME: 3. do we have to re-initialize our cpu_action object? */
#if 0
  /* iterate for all virtual machines */
  for (WorkstationVMModel::vm_list_t::iterator iter =
         WorkstationVMModel::ws_vms.begin();
       iter !=  WorkstationVMModel::ws_vms.end(); ++iter) {

    {
#if 0
      WorkstationVM2013Ptr ws_vm2013 = static_cast<WorkstationVM2013Ptr>(&*iter);
      XBT_INFO("cost %f remains %f start %f finish %f", ws_vm2013->cpu_action->cost,
          ws_vm2013->cpu_action->remains,
          ws_vm2013->cpu_action->start,
          ws_vm2013->cpu_action->finish
          );
#endif
#if 0
      void *ind_sub_host = xbt_lib_get_elm_or_null(host_lib, ws_vm2013->sub_ws->generic_resource.getName);
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

ActionPtr WorkstationVMHL13Model::executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *flops_amount,
                                        double *bytes_amount,
                                        double rate){
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  if ((workstation_nb == 1)
      && (cost_or_zero(bytes_amount, 0) == 0.0))
    return ((WorkstationCLM03Ptr)workstation_list[0])->execute(flops_amount[0]);
  else if ((workstation_nb == 1)
           && (cost_or_zero(flops_amount, 0) == 0.0))
    return communicate((WorkstationCLM03Ptr)workstation_list[0], (WorkstationCLM03Ptr)workstation_list[0],bytes_amount[0], rate);
  else if ((workstation_nb == 2)
             && (cost_or_zero(flops_amount, 0) == 0.0)
             && (cost_or_zero(flops_amount, 1) == 0.0)) {
    int i,nb = 0;
    double value = 0.0;

    for (i = 0; i < workstation_nb * workstation_nb; i++) {
      if (cost_or_zero(bytes_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(bytes_amount, i);
      }
    }
    if (nb == 1)
      return communicate((WorkstationCLM03Ptr)workstation_list[0], (WorkstationCLM03Ptr)workstation_list[1],value, rate);
  }
#undef cost_or_zero

  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
  return NULL;
}

/************
 * Resource *
 ************/

WorkstationVMHL13::WorkstationVMHL13(WorkstationVMModelPtr model, const char* name, xbt_dict_t props,
		                                   surf_resource_t ind_phys_workstation)
 : WorkstationVM(model, name, props, NULL, NULL)
{
  WorkstationPtr sub_ws = static_cast<WorkstationPtr>(surf_workstation_resource_priv(ind_phys_workstation));

  /* Currently, we assume a VM has no storage. */
  p_storage = NULL;

  /* Currently, a VM uses the network resource of its physical host. In
   * host_lib, this network resource object is referred from two different keys.
   * When deregistering the reference that points the network resource object
   * from the VM name, we have to make sure that the system does not call the
   * free callback for the network resource object. The network resource object
   * is still used by the physical machine. */
  p_netElm = new RoutingEdgeWrapper(static_cast<RoutingEdgePtr>(xbt_lib_get_or_null(host_lib, sub_ws->getName(), ROUTING_HOST_LEVEL)));
  xbt_lib_set(host_lib, name, ROUTING_HOST_LEVEL, p_netElm);

  p_subWs = sub_ws;
  p_currentState = SURF_VM_STATE_CREATED;

  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of the sub_cpu one.
  CpuCas01Ptr sub_cpu = static_cast<CpuCas01Ptr>(surf_cpu_resource_priv(ind_phys_workstation));

  /* We can assume one core and cas01 cpu for the first step.
   * Do xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu) if you get the resource. */

  p_cpu = surf_cpu_model_vm->createCpu(name, // name
      sub_cpu->getPowerPeakList(),        // host->power_peak,
      sub_cpu->getPState(),
      1,                          // host->power_scale,
      NULL,                       // host->power_trace,
      1,                          // host->core_amount,
      SURF_RESOURCE_ON,           // host->initial_state,
      NULL,                       // host->state_trace,
      NULL);                       // host->properties,

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* FIXME: TODO: we have to peridocally input GUESTOS_NOISE to the system? how ? */
  // vm_ws->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_phys_workstation, GUESTOS_NOISE);
  p_action = sub_cpu->execute(0);

  /* The SURF_WKS_LEVEL at host_lib saves workstation_CLM03 objects. Please
   * note workstation_VM2013 objects, inheriting the workstation_CLM03
   * structure, are also saved there.
   *
   * If you want to get a workstation_VM2013 object from host_lib, see
   * ws->generic_resouce.model->type first. If it is
   * SURF_MODEL_TYPE_VM_WORKSTATION, you can cast ws to vm_ws. */
  XBT_INFO("Create VM(%s)@PM(%s) with %ld mounted disks", name, sub_ws->getName(), xbt_dynar_length(p_storage));
}

/*
 * A physical host does not disappear in the current SimGrid code, but a VM may
 * disappear during a simulation.
 */
WorkstationVMHL13::~WorkstationVMHL13()
{
  /* Free the cpu_action of the VM. */
  _XBT_GNUC_UNUSED int ret = p_action->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");
}

void WorkstationVMHL13::updateState(tmgr_trace_event_t /*event_type*/, double /*value*/, double /*date*/) {
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

bool WorkstationVMHL13::isUsed() {
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
  return -1;
}

e_surf_resource_state_t WorkstationVMHL13::getState()
{
  return (e_surf_resource_state_t) p_currentState;
}

void WorkstationVMHL13::setState(e_surf_resource_state_t state)
{
  p_currentState = (e_surf_vm_state_t) state;
}

void WorkstationVMHL13::suspend()
{
  p_action->suspend();
  p_currentState = SURF_VM_STATE_SUSPENDED;
}

void WorkstationVMHL13::resume()
{
  p_action->resume();
  p_currentState = SURF_VM_STATE_RUNNING;
}

void WorkstationVMHL13::save()
{
  p_currentState = SURF_VM_STATE_SAVING;

  /* FIXME: do something here */
  p_action->suspend();
  p_currentState = SURF_VM_STATE_SAVED;
}

void WorkstationVMHL13::restore()
{
  p_currentState = SURF_VM_STATE_RESTORING;

  /* FIXME: do something here */
  p_action->resume();
  p_currentState = SURF_VM_STATE_RUNNING;
}

/*
 * Update the physical host of the given VM
 */
void WorkstationVMHL13::migrate(surf_resource_t ind_dst_pm)
{
   /* ind_phys_workstation equals to smx_host_t */
   WorkstationPtr ws_dst = static_cast<WorkstationPtr>(surf_workstation_resource_priv(ind_dst_pm));
   const char *vm_name = getName();
   const char *pm_name_src = p_subWs->getName();
   const char *pm_name_dst = ws_dst->getName();

   xbt_assert(ws_dst);

   /* do something */

   /* update net_elm with that of the destination physical host */
   RoutingEdgePtr old_net_elm = p_netElm;
   RoutingEdgePtr new_net_elm = new RoutingEdgeWrapper(static_cast<RoutingEdgePtr>(xbt_lib_get_or_null(host_lib, pm_name_dst, ROUTING_HOST_LEVEL)));
   xbt_assert(new_net_elm);

   /* Unregister the current net_elm from host_lib. Do not call the free callback. */
   xbt_lib_unset(host_lib, vm_name, ROUTING_HOST_LEVEL, 0);

   /* Then, resister the new one. */
   p_netElm = new_net_elm;
   xbt_lib_set(host_lib, vm_name, ROUTING_HOST_LEVEL, p_netElm);

   p_subWs = ws_dst;

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
     CpuActionPtr new_cpu_action = static_cast<CpuActionPtr>(
    		                            static_cast<CpuPtr>(surf_cpu_resource_priv(ind_dst_pm))->execute(0));

     e_surf_action_state_t state = p_action->getState();
     if (state != SURF_ACTION_DONE)
       XBT_CRITICAL("FIXME: may need a proper handling, %d", state);
     if (p_action->getRemainsNoUpdate() > 0)
       XBT_CRITICAL("FIXME: need copy the state(?), %f", p_action->getRemainsNoUpdate());

     /* keep the bound value of the cpu action of the VM. */
     double old_bound = p_action->getBound();
     if (old_bound != 0) {
       XBT_DEBUG("migrate VM(%s): set bound (%f) at %s", vm_name, old_bound, pm_name_dst);
       new_cpu_action->setBound(old_bound);
     }

     _XBT_GNUC_UNUSED int ret = p_action->unref();
     xbt_assert(ret == 1, "Bug: some resource still remains");

     p_action = new_cpu_action;
   }

   XBT_DEBUG("migrate VM(%s): change net_elm (%p to %p)", vm_name, old_net_elm, new_net_elm);
   XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
   delete old_net_elm;
}

void WorkstationVMHL13::setBound(double bound){
 p_action->setBound(bound);
}

void WorkstationVMHL13::setAffinity(CpuPtr cpu, unsigned long mask){
 p_action->setAffinity(cpu, mask);
}

/*
 * A surf level object will be useless in the upper layer. Returning the
 * dict_elm of the host.
 **/
surf_resource_t WorkstationVMHL13::getPm()
{
  return xbt_lib_get_elm_or_null(host_lib, p_subWs->getName());
}

/* Adding a task to a VM updates the VCPU task on its physical machine. */
ActionPtr WorkstationVMHL13::execute(double size)
{
  double old_cost = p_action->getCost();
  double new_cost = old_cost + size;

  XBT_DEBUG("VM(%s)@PM(%s): update dummy action's cost (%f -> %f)",
      getName(), p_subWs->getName(),
      old_cost, new_cost);

  p_action->setCost(new_cost);

  return p_cpu->execute(size);
}

ActionPtr WorkstationVMHL13::sleep(double duration) {
  return p_cpu->sleep(duration);
}

/**********
 * Action *
 **********/

//FIME:: handle action cancel

