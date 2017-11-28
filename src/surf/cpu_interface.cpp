/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_interface.hpp"
#include "src/instr/instr_private.hpp" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals
#include <xbt/dynar.h>

XBT_LOG_EXTERNAL_CATEGORY(surf_kernel);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf, "Logging specific to the SURF cpu module");

simgrid::surf::CpuModel *surf_cpu_model_pm;
simgrid::surf::CpuModel *surf_cpu_model_vm;

namespace simgrid {
namespace surf {

/*********
 * Model *
 *********/

void CpuModel::updateActionsStateLazy(double now, double /*delta*/)
{
  while (not actionHeapIsEmpty() && double_equals(actionHeapTopDate(), now, sg_surf_precision)) {

    CpuAction* action = static_cast<CpuAction*>(actionHeapPop());
    XBT_CDEBUG(surf_kernel, "Something happened to action %p", action);
    if (TRACE_is_enabled()) {
      Cpu* cpu = static_cast<Cpu*>(action->getVariable()->get_constraint(0)->get_id());
      TRACE_surf_host_set_utilization(cpu->getCname(), action->getCategory(), action->getVariable()->get_value(),
                                      action->getLastUpdate(), now - action->getLastUpdate());
    }

    action->finish(Action::State::done);
    XBT_CDEBUG(surf_kernel, "Action %p finished", action);

    /* set the remains to 0 due to precision problems when updating the remaining amount */
    action->setRemains(0);
  }
  if (TRACE_is_enabled()) {
    //defining the last timestamp that we can safely dump to trace file
    //without losing the event ascending order (considering all CPU's)
    double smaller = -1;
    for (Action const& action : *getRunningActionSet()) {
      if (smaller < 0 || action.getLastUpdate() < smaller)
        smaller = action.getLastUpdate();
    }
    if (smaller > 0) {
      TRACE_last_timestamp_to_dump = smaller;
    }
  }
}

void CpuModel::updateActionsStateFull(double now, double delta)
{
  for (auto it = std::begin(*getRunningActionSet()); it != std::end(*getRunningActionSet());) {
    CpuAction& action = static_cast<CpuAction&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    if (TRACE_is_enabled()) {
      Cpu* cpu = static_cast<Cpu*>(action.getVariable()->get_constraint(0)->get_id());
      TRACE_surf_host_set_utilization(cpu->getCname(), action.getCategory(), action.getVariable()->get_value(),
                                      now - delta, delta);
      TRACE_last_timestamp_to_dump = now - delta;
    }

    action.updateRemains(action.getVariable()->get_value() * delta);

    if (action.getMaxDuration() != NO_MAX_DURATION)
      action.updateMaxDuration(delta);

    if (((action.getRemainsNoUpdate() <= 0) && (action.getVariable()->get_weight() > 0)) ||
        ((action.getMaxDuration() != NO_MAX_DURATION) && (action.getMaxDuration() <= 0))) {
      action.finish(Action::State::done);
    }
  }
}

/************
 * Resource *
 ************/
Cpu::Cpu(Model *model, simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core)
 : Cpu(model, host, nullptr/*constraint*/, speedPerPstate, core)
{
}

Cpu::Cpu(Model* model, simgrid::s4u::Host* host, lmm_constraint_t constraint, std::vector<double>* speedPerPstate,
         int core)
    : Resource(model, host->getCname(), constraint), coresAmount_(core), host_(host)
{
  xbt_assert(core > 0, "Host %s must have at least one core, not 0.", host->getCname());

  speed_.peak = speedPerPstate->front();
  speed_.scale = 1;
  host->pimpl_cpu = this;
  xbt_assert(speed_.scale > 0, "Speed of host %s must be >0", host->getCname());

  // Copy the power peak array:
  for (double const& value : *speedPerPstate) {
    speedPerPstate_.push_back(value);
  }
}

Cpu::~Cpu() = default;

int Cpu::getNbPStates()
{
  return speedPerPstate_.size();
}

void Cpu::setPState(int pstate_index)
{
  xbt_assert(pstate_index <= static_cast<int>(speedPerPstate_.size()),
             "Invalid parameters for CPU %s (pstate %d > length of pstates %d). Please fix your platform file, or your "
             "call to change the pstate.",
             getCname(), pstate_index, static_cast<int>(speedPerPstate_.size()));

  double new_peak_speed = speedPerPstate_[pstate_index];
  pstate_ = pstate_index;
  speed_.peak = new_peak_speed;

  onSpeedChange();
}

int Cpu::getPState()
{
  return pstate_;
}

double Cpu::getPstateSpeed(int pstate_index)
{
  xbt_assert((pstate_index <= static_cast<int>(speedPerPstate_.size())), "Invalid parameters (pstate index out of bounds)");

  return speedPerPstate_[pstate_index];
}

double Cpu::getSpeed(double load)
{
  return load * speed_.peak;
}

double Cpu::getAvailableSpeed()
{
/* number between 0 and 1 */
  return speed_.scale;
}

void Cpu::onSpeedChange() {
  TRACE_surf_host_set_speed(surf_get_clock(), getCname(), coresAmount_ * speed_.scale * speed_.peak);
  s4u::Host::onSpeedChange(*host_);
}

int Cpu::coreCount()
{
  return coresAmount_;
}

void Cpu::setStateTrace(tmgr_trace_t trace)
{
  xbt_assert(stateEvent_ == nullptr, "Cannot set a second state trace to Host %s", host_->getCname());

  stateEvent_ = future_evt_set->add_trace(trace, this);
}
void Cpu::setSpeedTrace(tmgr_trace_t trace)
{
  xbt_assert(speed_.event == nullptr, "Cannot set a second speed trace to Host %s", host_->getCname());

  speed_.event = future_evt_set->add_trace(trace, this);
}


/**********
 * Action *
 **********/

void CpuAction::updateRemainingLazy(double now)
{
  xbt_assert(getStateSet() == getModel()->getRunningActionSet(), "You're updating an action that is not running.");
  xbt_assert(getPriority() > 0, "You're updating an action that seems suspended.");

  double delta = now - getLastUpdate();

  if (getRemainsNoUpdate() > 0) {
    XBT_CDEBUG(surf_kernel, "Updating action(%p): remains was %f, last_update was: %f", this, getRemainsNoUpdate(),
               getLastUpdate());
    updateRemains(getLastValue() * delta);

    if (TRACE_is_enabled()) {
      Cpu* cpu = static_cast<Cpu*>(getVariable()->get_constraint(0)->get_id());
      TRACE_surf_host_set_utilization(cpu->getCname(), getCategory(), getLastValue(), getLastUpdate(),
                                      now - getLastUpdate());
    }
    XBT_CDEBUG(surf_kernel, "Updating action(%p): remains is now %f", this, getRemainsNoUpdate());
  }

  refreshLastUpdate();
  setLastValue(getVariable()->get_value());
}

simgrid::xbt::signal<void(simgrid::surf::CpuAction*, Action::State)> CpuAction::onStateChange;

void CpuAction::suspend(){
  Action::State previous = getState();
  onStateChange(this, previous);
  Action::suspend();
}

void CpuAction::resume(){
  Action::State previous = getState();
  onStateChange(this, previous);
  Action::resume();
}

void CpuAction::setState(Action::State state){
  Action::State previous = getState();
  Action::setState(state);
  onStateChange(this, previous);
}
/** @brief returns a list of all CPUs that this action is using */
std::list<Cpu*> CpuAction::cpus() {
  std::list<Cpu*> retlist;
  int llen = getVariable()->get_number_of_constraint();

  for (int i = 0; i < llen; i++) {
    /* Beware of composite actions: ptasks put links and cpus together */
    // extra pb: we cannot dynamic_cast from void*...
    Resource* resource = static_cast<Resource*>(getVariable()->get_constraint(i)->get_id());
    Cpu* cpu           = dynamic_cast<Cpu*>(resource);
    if (cpu != nullptr)
      retlist.push_back(cpu);
  }

  return retlist;
}

}
}
