/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_interface.hpp"
#include "plugins/energy.hpp"

XBT_LOG_EXTERNAL_CATEGORY(surf_kernel);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF cpu module");

int autoload_surf_cpu_model = 1;
void_f_void_t surf_cpu_model_init_preparse = NULL;

simgrid::surf::CpuModel *surf_cpu_model_pm;
simgrid::surf::CpuModel *surf_cpu_model_vm;

namespace simgrid {
namespace surf {

simgrid::xbt::Extension<simgrid::Host, Cpu> Cpu::EXTENSION_ID;

void Cpu::init()
{
  if (!EXTENSION_ID.valid())
    EXTENSION_ID = simgrid::Host::extension_create<simgrid::surf::Cpu>();
}

/*************
 * Callbacks *
 *************/

Cpu *getActionCpu(CpuAction *action) {
  return static_cast<Cpu*>(lmm_constraint_id(lmm_get_cnst_from_var
		                	 (action->getModel()->getMaxminSystem(),
		                	 action->getVariable(), 0)));
}

simgrid::surf::signal<void(Cpu*)> cpuCreatedCallbacks;
simgrid::surf::signal<void(Cpu*)> cpuDestructedCallbacks;
simgrid::surf::signal<void(Cpu*, e_surf_resource_state_t, e_surf_resource_state_t)> cpuStateChangedCallbacks;
simgrid::surf::signal<void(CpuAction*, e_surf_action_state_t, e_surf_action_state_t)> cpuActionStateChangedCallbacks;
void cpu_add_traces(){
  surf_cpu_model_pm->addTraces();
}

/*********
 * Model *
 *********/

void CpuModel::updateActionsStateLazy(double now, double /*delta*/)
{
  CpuAction *action;
  while ((xbt_heap_size(getActionHeap()) > 0)
         && (double_equals(xbt_heap_maxkey(getActionHeap()), now, sg_surf_precision))) {
    action = static_cast<CpuAction*>(xbt_heap_pop(getActionHeap()));
    XBT_CDEBUG(surf_kernel, "Something happened to action %p", action);
    if (TRACE_is_enabled()) {
      Cpu *cpu = static_cast<Cpu*>(lmm_constraint_id(lmm_get_cnst_from_var(getMaxminSystem(), action->getVariable(), 0)));
      TRACE_surf_host_set_utilization(cpu->getName(), action->getCategory(),
                                      lmm_variable_getvalue(action->getVariable()),
                                      action->getLastUpdate(),
                                      now - action->getLastUpdate());
    }

    action->finish();
    XBT_CDEBUG(surf_kernel, "Action %p finished", action);

    /* set the remains to 0 due to precision problems when updating the remaining amount */
    action->setRemains(0);
    action->setState(SURF_ACTION_DONE);
    action->heapRemove(getActionHeap()); //FIXME: strange call since action was already popped
  }
  if (TRACE_is_enabled()) {
    //defining the last timestamp that we can safely dump to trace file
    //without losing the event ascending order (considering all CPU's)
    double smaller = -1;
    ActionList *actionSet = getRunningActionSet();
    for(ActionList::iterator it(actionSet->begin()), itend(actionSet->end())
       ; it != itend ; ++it) {
      action = static_cast<CpuAction*>(&*it);
        if (smaller < 0) {
          smaller = action->getLastUpdate();
          continue;
        }
        if (action->getLastUpdate() < smaller) {
          smaller = action->getLastUpdate();
        }
    }
    if (smaller > 0) {
      TRACE_last_timestamp_to_dump = smaller;
    }
  }
  return;
}

void CpuModel::updateActionsStateFull(double now, double delta)
{
  CpuAction *action = NULL;
  ActionList *running_actions = getRunningActionSet();

  for(ActionList::iterator it(running_actions->begin()), itNext=it, itend(running_actions->end())
     ; it != itend ; it=itNext) {
	++itNext;
    action = static_cast<CpuAction*>(&*it);
    if (TRACE_is_enabled()) {
      Cpu *x = static_cast<Cpu*> (lmm_constraint_id(lmm_get_cnst_from_var(getMaxminSystem(), action->getVariable(), 0)) );

      TRACE_surf_host_set_utilization(x->getName(),
                                      action->getCategory(),
                                      lmm_variable_getvalue(action->getVariable()),
                                      now - delta,
                                      delta);
      TRACE_last_timestamp_to_dump = now - delta;
    }

    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);


    if (action->getMaxDuration() != NO_MAX_DURATION)
      action->updateMaxDuration(delta);


    if ((action->getRemainsNoUpdate() <= 0) &&
        (lmm_get_variable_weight(action->getVariable()) > 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    } else if ((action->getMaxDuration() != NO_MAX_DURATION) &&
               (action->getMaxDuration() <= 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    }
  }

  return;
}

/************
 * Resource *
 ************/
Cpu::Cpu()
{
}


Cpu::Cpu(Model *model, const char *name,
         int core, double speedPeak, double speedScale,
         e_surf_resource_state_t stateInitial)
 : Resource(model, name, stateInitial)
 , m_core(core)
 , m_speedPeak(speedPeak)
 , m_speedScale(speedScale)
{

}

Cpu::Cpu(Model *model, const char *name,
        lmm_constraint_t constraint, int core, double speedPeak,
        double speedScale, e_surf_resource_state_t stateInitial)
 : Resource(model, name, constraint, stateInitial)
 , m_core(core)
 , m_speedPeak(speedPeak)
 , m_speedScale(speedScale)
{
  /* At now, we assume that a VM does not have a multicore CPU. */
  if (core > 1)
    xbt_assert(model == surf_cpu_model_pm);

  if (model->getUpdateMechanism() != UM_UNDEFINED) {
	p_constraintCore = xbt_new(lmm_constraint_t, core);
	p_constraintCoreId = xbt_new(void*, core);

    int i;
    for (i = 0; i < core; i++) {
      /* just for a unique id, never used as a string. */
      p_constraintCoreId[i] = bprintf("%s:%i", name, i);
      p_constraintCore[i] = lmm_constraint_new(model->getMaxminSystem(), p_constraintCoreId[i], m_speedScale * m_speedPeak);
    }
  }
}

Cpu::Cpu(Model *model, const char *name,
  lmm_constraint_t constraint, int core, double speedPeak, double speedScale)
: Cpu(model, name, constraint, core, speedPeak, speedScale, SURF_RESOURCE_ON)
{}

Cpu::Cpu(Model *model, const char *name,
  int core, double speedPeak, double speedScale)
: Cpu(model, name, core, speedPeak, speedScale, SURF_RESOURCE_ON)
{}

void Cpu::onDie()
{
  cpuDestructedCallbacks(this);
  Resource::onDie();
}

Cpu::~Cpu()
{
  this->die();
  if (p_constraintCoreId){
    for (int i = 0; i < m_core; i++) {
	  xbt_free(p_constraintCoreId[i]);
    }
    xbt_free(p_constraintCore);
  }
  if (p_constraintCoreId)
    xbt_free(p_constraintCoreId);
}

double Cpu::getCurrentPowerPeak()
{
  return m_speedPeak;
}

double Cpu::getSpeed(double load)
{
  return load * m_speedPeak;
}

double Cpu::getAvailableSpeed()
{
/* number between 0 and 1 */
  return m_speedScale;
}

int Cpu::getCore()
{
  return m_core;
}

void Cpu::setState(e_surf_resource_state_t state)
{
  e_surf_resource_state_t old = Resource::getState();
  Resource::setState(state);
  cpuStateChangedCallbacks(this, old, state);
}

void Cpu::plug(simgrid::Host* host)
{
  if (this->m_host != nullptr)
    xbt_die("Already plugged into host %s", host->id().c_str());
  host->extension_set(this);
  this->m_host = host;
  simgrid::surf::cpuCreatedCallbacks(this);
  simgrid::surf::cpuStateChangedCallbacks(this,
    SURF_RESOURCE_ON, this->getState());
}

/**********
 * Action *
 **********/

void CpuAction::updateRemainingLazy(double now)
{
  double delta = 0.0;

  xbt_assert(getStateSet() == getModel()->getRunningActionSet(),
      "You're updating an action that is not running.");

  /* bogus priority, skip it */
  xbt_assert(getPriority() > 0,
      "You're updating an action that seems suspended.");

  delta = now - m_lastUpdate;

  if (m_remains > 0) {
    XBT_CDEBUG(surf_kernel, "Updating action(%p): remains was %f, last_update was: %f", this, m_remains, m_lastUpdate);
    double_update(&(m_remains), m_lastValue * delta, sg_maxmin_precision*sg_surf_precision);

    if (TRACE_is_enabled()) {
      Cpu *cpu = static_cast<Cpu*>(lmm_constraint_id(lmm_get_cnst_from_var(getModel()->getMaxminSystem(), getVariable(), 0)));
      TRACE_surf_host_set_utilization(cpu->getName(), getCategory(), m_lastValue, m_lastUpdate, now - m_lastUpdate);
    }
    XBT_CDEBUG(surf_kernel, "Updating action(%p): remains is now %f", this, m_remains);
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(getVariable());
}

/*
 *
 * This function formulates a constraint problem that pins a given task to
 * particular cores. Currently, it is possible to pin a task to an exactly one
 * specific core. The system links the variable object of the task to the
 * per-core constraint object.
 *
 * But, the taskset command on Linux takes a mask value specifying a CPU
 * affinity setting of a given task. If the mask value is 0x03, the given task
 * will be executed on the first core (CPU0) or the second core (CPU1) on the
 * given PM. The schedular will determine appropriate placements of tasks,
 * considering given CPU affinities and task activities.
 *
 * How should the system formulate constraint problems for an affinity to
 * multiple cores?
 *
 * The cpu argument must be the host where the task is being executed. The
 * action object does not have the information about the location where the
 * action is being executed.
 */
void CpuAction::setAffinity(Cpu *cpu, unsigned long mask)
{
  lmm_variable_t var_obj = getVariable();
  XBT_IN("(%p,%lx)", this, mask);

  {
    unsigned long nbits = 0;

    /* FIXME: There is much faster algorithms doing this. */
    for (int i = 0; i < cpu->m_core; i++) {
      unsigned long has_affinity = (1UL << i) & mask;
      if (has_affinity)
        nbits += 1;
    }

    if (nbits > 1) {
      XBT_CRITICAL("Do not specify multiple cores for an affinity mask.");
      XBT_CRITICAL("See the comment in cpu_action_set_affinity().");
      DIE_IMPOSSIBLE;
    }
  }

  for (int i = 0; i < cpu->m_core; i++) {
    XBT_DEBUG("clear affinity %p to cpu-%d@%s", this, i,  cpu->getName());
    lmm_shrink(cpu->getModel()->getMaxminSystem(), cpu->p_constraintCore[i], var_obj);

    unsigned long has_affinity = (1UL << i) & mask;
    if (has_affinity) {
      /* This function only accepts an affinity setting on the host where the
       * task is now running. In future, a task might move to another host.
       * But, at this moment, this function cannot take an affinity setting on
       * that future host.
       *
       * It might be possible to extend the code to allow this function to
       * accept affinity settings on a future host. We might be able to assign
       * zero to elem->value to maintain such inactive affinity settings in the
       * system. But, this will make the system complex. */
      XBT_DEBUG("set affinity %p to cpu-%d@%s", this, i, cpu->getName());
      lmm_expand(cpu->getModel()->getMaxminSystem(), cpu->p_constraintCore[i], var_obj, 1.0);
    }
  }

  if (cpu->getModel()->getUpdateMechanism() == UM_LAZY) {
    /* FIXME (hypervisor): Do we need to do something for the LAZY mode? */
  }
  XBT_OUT();
}

void CpuAction::setState(e_surf_action_state_t state){
  e_surf_action_state_t old = getState();
  Action::setState(state);
  cpuActionStateChangedCallbacks(this, old, state);
}

}
}
