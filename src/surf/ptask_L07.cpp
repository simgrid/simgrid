/* Copyright (c) 2007-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <algorithm>
#include <unordered_set>

#include "ptask_L07.hpp"

#include "cpu_interface.hpp"
#include "xbt/lib.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);
XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/
void surf_host_model_init_ptask_L07()
{
  XBT_CINFO(xbt_cfg,"Switching to the L07 model to handle parallel tasks.");
  xbt_assert(!surf_cpu_model_pm, "CPU model type already defined");
  xbt_assert(!surf_network_model, "network model type already defined");

  surf_host_model = new simgrid::surf::HostL07Model();
  all_existing_models->push_back(surf_host_model);
}


namespace simgrid {
namespace surf {

HostL07Model::HostL07Model() : HostModel() {
  maxminSystem_ = lmm_system_new(true /* lazy */);
  maxminSystem_->solve_fun = &bottleneck_solve;
  surf_network_model = new NetworkL07Model(this,maxminSystem_);
  surf_cpu_model_pm = new CpuL07Model(this,maxminSystem_);
}

HostL07Model::~HostL07Model() 
{
  lmm_system_free(maxminSystem_);
  maxminSystem_ = nullptr;
  delete surf_network_model;
  delete surf_cpu_model_pm;
}

CpuL07Model::CpuL07Model(HostL07Model *hmodel,lmm_system_t sys)
  : CpuModel()
  , hostModel_(hmodel)
{
  maxminSystem_ = sys;
}

CpuL07Model::~CpuL07Model()
{
  maxminSystem_ = nullptr;
}

NetworkL07Model::NetworkL07Model(HostL07Model *hmodel, lmm_system_t sys)
  : NetworkModel()
  , hostModel_(hmodel)
{
  maxminSystem_ = sys;
  loopback_     = createLink("__loopback__", 498000000, 0.000015, SURF_LINK_FATPIPE);
}

NetworkL07Model::~NetworkL07Model()
{
  maxminSystem_ = nullptr;
}

double HostL07Model::nextOccuringEvent(double now)
{
  double min = HostModel::nextOccuringEventFull(now);
  ActionList::iterator it(getRunningActionSet()->begin());
  ActionList::iterator itend(getRunningActionSet()->end());
  for (; it != itend; ++it) {
    L07Action *action = static_cast<L07Action*>(&*it);
    if (action->latency_ > 0 && (min < 0 || action->latency_ < min)) {
      min = action->latency_;
      XBT_DEBUG("Updating min with %p (start %f): %f", action, action->getStartTime(), min);
    }
  }
  XBT_DEBUG("min value: %f", min);

  return min;
}

void HostL07Model::updateActionsState(double /*now*/, double delta) {

  L07Action *action;
  ActionList *actionSet = getRunningActionSet();
  ActionList::iterator it(actionSet->begin());
  ActionList::iterator itNext = it;
  ActionList::iterator itend(actionSet->end());

  for (; it != itend; it = itNext) {
    ++itNext;
    action = static_cast<L07Action*>(&*it);
    if (action->latency_ > 0) {
      if (action->latency_ > delta) {
        double_update(&(action->latency_), delta, sg_surf_precision);
      } else {
        action->latency_ = 0.0;
      }
      if ((action->latency_ <= 0.0) && (action->isSuspended() == 0)) {
        action->updateBound();
        lmm_update_variable_weight(maxminSystem_, action->getVariable(), 1.0);
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.",
           action, action->getRemains(), lmm_variable_getvalue(action->getVariable()) * delta);
    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);

    if (action->getMaxDuration() > NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).", action, action->getRemains());

    /* In the next if cascade, the action can be finished either because:
     *  - The amount of remaining work reached 0
     *  - The max duration was reached
     * If it's not done, it may have failed.
     */

    if (((action->getRemains() <= 0) && (lmm_get_variable_weight(action->getVariable()) > 0)) ||
        ((action->getMaxDuration() > NO_MAX_DURATION) && (action->getMaxDuration() <= 0))) {
      action->finish();
      action->setState(Action::State::done);
    } else {
      /* Need to check that none of the model has failed */
      int i = 0;
      lmm_constraint_t cnst = lmm_get_cnst_from_var(maxminSystem_, action->getVariable(), i);
      while (cnst != nullptr) {
        i++;
        void *constraint_id = lmm_constraint_id(cnst);
        if (static_cast<simgrid::surf::Resource*>(constraint_id)->isOff()) {
          XBT_DEBUG("Action (%p) Failed!!", action);
          action->finish();
          action->setState(Action::State::failed);
          break;
        }
        cnst = lmm_get_cnst_from_var(maxminSystem_, action->getVariable(), i);
      }
    }
  }
}

Action *HostL07Model::executeParallelTask(int host_nb, sg_host_t *host_list,
                                          double *flops_amount, double *bytes_amount,double rate) {
  return new L07Action(this, host_nb, host_list, flops_amount, bytes_amount, rate);
}

L07Action::L07Action(Model *model, int host_nb, sg_host_t *host_list,
                     double *flops_amount, double *bytes_amount, double rate)
  : CpuAction(model, 1, 0)
  , computationAmount_(flops_amount)
  , communicationAmount_(bytes_amount)
  , rate_(rate)
{
  int nb_link = 0;
  int nb_used_host = 0; /* Only the hosts with something to compute (>0 flops) are counted) */
  double latency = 0.0;

  this->hostList_->reserve(host_nb);
  for (int i = 0; i < host_nb; i++) {
    this->hostList_->push_back(host_list[i]);
    if (flops_amount[i] > 0)
      nb_used_host++;
  }

  /* Compute the number of affected resources... */
  if(bytes_amount != nullptr) {
    std::unordered_set<const char*> affected_links;

    for (int i = 0; i < host_nb; i++) {
      for (int j = 0; j < host_nb; j++) {

        if (bytes_amount[i * host_nb + j] > 0) {
          double lat=0.0;

          std::vector<LinkImpl*> route;
          hostList_->at(i)->routeTo(hostList_->at(j), &route, &lat);
          latency = MAX(latency, lat);

          for (auto link : route)
            affected_links.insert(link->cname());
        }
      }
    }

    nb_link = affected_links.size();
  }

  XBT_DEBUG("Creating a parallel task (%p) with %d hosts and %d unique links.", this, host_nb, nb_link);
  this->latency_ = latency;

  this->variable_ = lmm_variable_new(model->getMaxminSystem(), this, 1.0,
      (rate > 0 ? rate : -1.0),
      host_nb + nb_link);

  if (this->latency_ > 0)
    lmm_update_variable_weight(model->getMaxminSystem(), this->getVariable(), 0.0);

  for (int i = 0; i < host_nb; i++)
    lmm_expand(model->getMaxminSystem(), host_list[i]->pimpl_cpu->constraint(), this->getVariable(), flops_amount[i]);

  if(bytes_amount != nullptr) {
    for (int i = 0; i < host_nb; i++) {
      for (int j = 0; j < host_nb; j++) {
        if (bytes_amount[i * host_nb + j] > 0.0) {
          std::vector<LinkImpl*> route;
          hostList_->at(i)->routeTo(hostList_->at(j), &route, nullptr);

          for (auto link : route)
            lmm_expand_add(model->getMaxminSystem(), link->constraint(), this->getVariable(),
                           bytes_amount[i * host_nb + j]);
        }
      }
    }
  }

  if (nb_link + nb_used_host == 0) {
    this->setCost(1.0);
    this->setRemains(0.0);
  }
  xbt_free(host_list);
}

Action* NetworkL07Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  sg_host_t*host_list = xbt_new0(sg_host_t, 2);
  double *flops_amount = xbt_new0(double, 2);
  double *bytes_amount = xbt_new0(double, 4);

  host_list[0]    = src;
  host_list[1]    = dst;
  bytes_amount[1] = size;

  return hostModel_->executeParallelTask(2, host_list, flops_amount, bytes_amount, rate);
}

Cpu *CpuL07Model::createCpu(simgrid::s4u::Host *host,  std::vector<double> *speedPerPstate, int core)
{
  return new CpuL07(this, host, speedPerPstate, core);
}

LinkImpl* NetworkL07Model::createLink(const char* name, double bandwidth, double latency,
                                      e_surf_link_sharing_policy_t policy)
{
  return new LinkL07(this, name, bandwidth, latency, policy);
}

/************
 * Resource *
 ************/

CpuL07::CpuL07(CpuL07Model* model, simgrid::s4u::Host* host, std::vector<double>* speedPerPstate, int core)
    : Cpu(model, host, lmm_constraint_new(model->getMaxminSystem(), this, speedPerPstate->front()), speedPerPstate,
          core)
{
}

CpuL07::~CpuL07()=default;

LinkL07::LinkL07(NetworkL07Model* model, const char* name, double bandwidth, double latency,
                 e_surf_link_sharing_policy_t policy)
    : LinkImpl(model, name, lmm_constraint_new(model->getMaxminSystem(), this, bandwidth))
{
  bandwidth_.peak = bandwidth;
  latency_.peak   = latency;

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(constraint());

  s4u::Link::onCreation(this->piface_);
}

Action *CpuL07::execution_start(double size)
{
  sg_host_t*host_list = xbt_new0(sg_host_t, 1);
  double *flops_amount = xbt_new0(double, 1);

  host_list[0] = getHost();
  flops_amount[0] = size;

  return static_cast<CpuL07Model*>(model())->hostModel_->executeParallelTask(1, host_list, flops_amount, nullptr, -1);
}

Action *CpuL07::sleep(double duration)
{
  L07Action *action = static_cast<L07Action*>(execution_start(1.0));
  action->maxDuration_ = duration;
  action->suspended_ = 2;
  lmm_update_variable_weight(model()->getMaxminSystem(), action->getVariable(), 0.0);

  return action;
}

bool CpuL07::isUsed(){
  return lmm_constraint_used(model()->getMaxminSystem(), constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuL07::onSpeedChange() {
  lmm_variable_t var = nullptr;
  lmm_element_t elem = nullptr;

  lmm_update_constraint_bound(model()->getMaxminSystem(), constraint(), speed_.peak * speed_.scale);
  while ((var = lmm_get_var_from_cnst(model()->getMaxminSystem(), constraint(), &elem))) {
    Action* action = static_cast<Action*>(lmm_variable_id(var));

    lmm_update_variable_bound(model()->getMaxminSystem(), action->getVariable(), speed_.scale * speed_.peak);
    }

  Cpu::onSpeedChange();
}


bool LinkL07::isUsed(){
  return lmm_constraint_used(model()->getMaxminSystem(), constraint());
}

void CpuL07::apply_event(tmgr_trace_iterator_t triggered, double value){
  XBT_DEBUG("Updating cpu %s (%p) with value %g", cname(), this, value);
  if (triggered == speed_.event) {
    speed_.scale = value;
    onSpeedChange();
    tmgr_trace_event_unref(&speed_.event);

  } else if (triggered == stateEvent_) {
    if (value > 0)
      turnOn();
    else
      turnOff();
    tmgr_trace_event_unref(&stateEvent_);

  } else {
    xbt_die("Unknown event!\n");
  }
}

void LinkL07::apply_event(tmgr_trace_iterator_t triggered, double value) {
  XBT_DEBUG("Updating link %s (%p) with value=%f", cname(), this, value);
  if (triggered == bandwidth_.event) {
    setBandwidth(value);
    tmgr_trace_event_unref(&bandwidth_.event);

  } else if (triggered == latency_.event) {
    setLatency(value);
    tmgr_trace_event_unref(&latency_.event);

  } else if (triggered == stateEvent_) {
    if (value > 0)
      turnOn();
    else
      turnOff();
    tmgr_trace_event_unref(&stateEvent_);

  } else {
    xbt_die("Unknown event ! \n");
  }
}

void LinkL07::setBandwidth(double value)
{
  bandwidth_.peak = value;
  lmm_update_constraint_bound(model()->getMaxminSystem(), constraint(), bandwidth_.peak * bandwidth_.scale);
}

void LinkL07::setLatency(double value)
{
  lmm_variable_t var = nullptr;
  L07Action *action;
  lmm_element_t elem = nullptr;

  latency_.peak = value;
  while ((var = lmm_get_var_from_cnst(model()->getMaxminSystem(), constraint(), &elem))) {
    action = static_cast<L07Action*>(lmm_variable_id(var));
    action->updateBound();
  }
}
LinkL07::~LinkL07() = default;

/**********
 * Action *
 **********/

L07Action::~L07Action(){
  delete hostList_;
  free(communicationAmount_);
  free(computationAmount_);
}

void L07Action::updateBound()
{
  double lat_current = 0.0;

  int hostNb = hostList_->size();

  if (communicationAmount_ != nullptr) {
    for (int i = 0; i < hostNb; i++) {
      for (int j = 0; j < hostNb; j++) {

        if (communicationAmount_[i * hostNb + j] > 0) {
          double lat = 0.0;
          std::vector<LinkImpl*> route;
          hostList_->at(i)->routeTo(hostList_->at(j), &route, &lat);

          lat_current = MAX(lat_current, lat * communicationAmount_[i * hostNb + j]);
        }
      }
    }
  }
  double lat_bound = sg_tcp_gamma / (2.0 * lat_current);
  XBT_DEBUG("action (%p) : lat_bound = %g", this, lat_bound);
  if ((latency_ <= 0.0) && (suspended_ == 0)) {
    if (rate_ < 0)
      lmm_update_variable_bound(getModel()->getMaxminSystem(), getVariable(), lat_bound);
    else
      lmm_update_variable_bound(getModel()->getMaxminSystem(), getVariable(), std::min(rate_, lat_bound));
  }
}

int L07Action::unref()
{
  refcount_--;
  if (!refcount_) {
    if (action_hook.is_linked())
      stateSet_->erase(stateSet_->iterator_to(*this));
    if (getVariable())
      lmm_variable_free(getModel()->getMaxminSystem(), getVariable());
    delete this;
    return 1;
  }
  return 0;
}

}
}
