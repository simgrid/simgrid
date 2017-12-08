/* Copyright (c) 2007-2010, 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <algorithm>
#include <unordered_set>

#include "ptask_L07.hpp"

#include "cpu_interface.hpp"
#include "xbt/utility.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);
XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/
void surf_host_model_init_ptask_L07()
{
  XBT_CINFO(xbt_cfg,"Switching to the L07 model to handle parallel tasks.");
  xbt_assert(not surf_cpu_model_pm, "CPU model type already defined");
  xbt_assert(not surf_network_model, "network model type already defined");

  surf_host_model = new simgrid::surf::HostL07Model();
  all_existing_models->push_back(surf_host_model);
}


namespace simgrid {
namespace surf {

HostL07Model::HostL07Model() : HostModel() {
  maxminSystem_            = new simgrid::kernel::lmm::System(true /* lazy */);
  maxminSystem_->solve_fun = &simgrid::kernel::lmm::bottleneck_solve;
  surf_network_model = new NetworkL07Model(this,maxminSystem_);
  surf_cpu_model_pm = new CpuL07Model(this,maxminSystem_);
}

HostL07Model::~HostL07Model()
{
  delete surf_network_model;
  delete surf_cpu_model_pm;
}

CpuL07Model::CpuL07Model(HostL07Model* hmodel, lmm_system_t sys) : CpuModel(), hostModel_(hmodel)
{
  maxminSystem_ = sys;
}

CpuL07Model::~CpuL07Model()
{
  maxminSystem_ = nullptr;
}

NetworkL07Model::NetworkL07Model(HostL07Model* hmodel, lmm_system_t sys) : NetworkModel(), hostModel_(hmodel)
{
  maxminSystem_ = sys;
  loopback_     = NetworkL07Model::createLink("__loopback__", 498000000, 0.000015, SURF_LINK_FATPIPE);
}

NetworkL07Model::~NetworkL07Model()
{
  maxminSystem_ = nullptr;
}

double HostL07Model::nextOccuringEvent(double now)
{
  double min = HostModel::nextOccuringEventFull(now);
  for (Action const& action : *getRunningActionSet()) {
    const L07Action& net_action = static_cast<const L07Action&>(action);
    if (net_action.latency_ > 0 && (min < 0 || net_action.latency_ < min)) {
      min = net_action.latency_;
      XBT_DEBUG("Updating min with %p (start %f): %f", &net_action, net_action.getStartTime(), min);
    }
  }
  XBT_DEBUG("min value: %f", min);

  return min;
}

void HostL07Model::updateActionsState(double /*now*/, double delta)
{
  for (auto it = std::begin(*getRunningActionSet()); it != std::end(*getRunningActionSet());) {
    L07Action& action = static_cast<L07Action&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    if (action.latency_ > 0) {
      if (action.latency_ > delta) {
        double_update(&(action.latency_), delta, sg_surf_precision);
      } else {
        action.latency_ = 0.0;
      }
      if ((action.latency_ <= 0.0) && (action.isSuspended() == 0)) {
        action.updateBound();
        maxminSystem_->update_variable_weight(action.getVariable(), 1.0);
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.", &action, action.getRemains(),
              action.getVariable()->get_value() * delta);
    action.updateRemains(action.getVariable()->get_value() * delta);

    if (action.getMaxDuration() > NO_MAX_DURATION)
      action.updateMaxDuration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).", &action, action.getRemains());

    /* In the next if cascade, the action can be finished either because:
     *  - The amount of remaining work reached 0
     *  - The max duration was reached
     * If it's not done, it may have failed.
     */

    if (((action.getRemains() <= 0) && (action.getVariable()->get_weight() > 0)) ||
        ((action.getMaxDuration() > NO_MAX_DURATION) && (action.getMaxDuration() <= 0))) {
      action.finish(Action::State::done);
    } else {
      /* Need to check that none of the model has failed */
      int i = 0;
      lmm_constraint_t cnst = action.getVariable()->get_constraint(i);
      while (cnst != nullptr) {
        i++;
        void* constraint_id = cnst->get_id();
        if (static_cast<simgrid::surf::Resource*>(constraint_id)->isOff()) {
          XBT_DEBUG("Action (%p) Failed!!", &action);
          action.finish(Action::State::failed);
          break;
        }
        cnst = action.getVariable()->get_constraint(i);
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
          hostList_->at(i)->routeTo(hostList_->at(j), route, &lat);
          latency = std::max(latency, lat);

          for (auto const& link : route)
            affected_links.insert(link->getCname());
        }
      }
    }

    nb_link = affected_links.size();
  }

  XBT_DEBUG("Creating a parallel task (%p) with %d hosts and %d unique links.", this, host_nb, nb_link);
  latency_ = latency;

  setVariable(model->getMaxminSystem()->variable_new(this, 1.0, (rate > 0 ? rate : -1.0), host_nb + nb_link));

  if (latency_ > 0)
    model->getMaxminSystem()->update_variable_weight(getVariable(), 0.0);

  for (int i = 0; i < host_nb; i++)
    model->getMaxminSystem()->expand(host_list[i]->pimpl_cpu->constraint(), getVariable(), flops_amount[i]);

  if(bytes_amount != nullptr) {
    for (int i = 0; i < host_nb; i++) {
      for (int j = 0; j < host_nb; j++) {
        if (bytes_amount[i * host_nb + j] > 0.0) {
          std::vector<LinkImpl*> route;
          hostList_->at(i)->routeTo(hostList_->at(j), route, nullptr);

          for (auto const& link : route)
            model->getMaxminSystem()->expand_add(link->constraint(), this->getVariable(),
                                                 bytes_amount[i * host_nb + j]);
        }
      }
    }
  }

  if (nb_link + nb_used_host == 0) {
    this->setCost(1.0);
    this->setRemains(0.0);
  }
  delete[] host_list;
}

Action* NetworkL07Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  sg_host_t* host_list = new sg_host_t[2]();
  double* flops_amount = new double[2]();
  double* bytes_amount = new double[4]();

  host_list[0]    = src;
  host_list[1]    = dst;
  bytes_amount[1] = size;

  return hostModel_->executeParallelTask(2, host_list, flops_amount, bytes_amount, rate);
}

Cpu *CpuL07Model::createCpu(simgrid::s4u::Host *host,  std::vector<double> *speedPerPstate, int core)
{
  return new CpuL07(this, host, speedPerPstate, core);
}

LinkImpl* NetworkL07Model::createLink(const std::string& name, double bandwidth, double latency,
                                      e_surf_link_sharing_policy_t policy)
{
  return new LinkL07(this, name, bandwidth, latency, policy);
}

/************
 * Resource *
 ************/

CpuL07::CpuL07(CpuL07Model* model, simgrid::s4u::Host* host, std::vector<double>* speedPerPstate, int core)
    : Cpu(model, host, model->getMaxminSystem()->constraint_new(this, speedPerPstate->front()), speedPerPstate, core)
{
}

CpuL07::~CpuL07()=default;

LinkL07::LinkL07(NetworkL07Model* model, const std::string& name, double bandwidth, double latency,
                 e_surf_link_sharing_policy_t policy)
    : LinkImpl(model, name, model->getMaxminSystem()->constraint_new(this, bandwidth))
{
  bandwidth_.peak = bandwidth;
  latency_.peak   = latency;

  if (policy == SURF_LINK_FATPIPE)
    constraint()->unshare();

  s4u::Link::onCreation(this->piface_);
}

Action *CpuL07::execution_start(double size)
{
  sg_host_t* host_list = new sg_host_t[1]();
  double* flops_amount = new double[1]();

  host_list[0] = getHost();
  flops_amount[0] = size;

  return static_cast<CpuL07Model*>(model())->hostModel_->executeParallelTask(1, host_list, flops_amount, nullptr, -1);
}

Action *CpuL07::sleep(double duration)
{
  L07Action *action = static_cast<L07Action*>(execution_start(1.0));
  action->setMaxDuration(duration);
  action->suspended_ = 2;
  model()->getMaxminSystem()->update_variable_weight(action->getVariable(), 0.0);

  return action;
}

bool CpuL07::isUsed(){
  return model()->getMaxminSystem()->constraint_used(constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuL07::onSpeedChange() {
  lmm_variable_t var       = nullptr;
  const_lmm_element_t elem = nullptr;

  model()->getMaxminSystem()->update_constraint_bound(constraint(), speed_.peak * speed_.scale);
  while ((var = constraint()->get_variable(&elem))) {
    Action* action = static_cast<Action*>(var->get_id());

    model()->getMaxminSystem()->update_variable_bound(action->getVariable(), speed_.scale * speed_.peak);
  }

  Cpu::onSpeedChange();
}


bool LinkL07::isUsed(){
  return model()->getMaxminSystem()->constraint_used(constraint());
}

void CpuL07::apply_event(tmgr_trace_event_t triggered, double value)
{
  XBT_DEBUG("Updating cpu %s (%p) with value %g", getCname(), this, value);
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

void LinkL07::apply_event(tmgr_trace_event_t triggered, double value)
{
  XBT_DEBUG("Updating link %s (%p) with value=%f", getCname(), this, value);
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
  model()->getMaxminSystem()->update_constraint_bound(constraint(), bandwidth_.peak * bandwidth_.scale);
}

void LinkL07::setLatency(double value)
{
  lmm_variable_t var = nullptr;
  L07Action *action;
  const_lmm_element_t elem = nullptr;

  latency_.peak = value;
  while ((var = constraint()->get_variable(&elem))) {
    action = static_cast<L07Action*>(var->get_id());
    action->updateBound();
  }
}
LinkL07::~LinkL07() = default;

/**********
 * Action *
 **********/

L07Action::~L07Action(){
  delete hostList_;
  delete[] communicationAmount_;
  delete[] computationAmount_;
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
          hostList_->at(i)->routeTo(hostList_->at(j), route, &lat);

          lat_current = std::max(lat_current, lat * communicationAmount_[i * hostNb + j]);
        }
      }
    }
  }
  double lat_bound = sg_tcp_gamma / (2.0 * lat_current);
  XBT_DEBUG("action (%p) : lat_bound = %g", this, lat_bound);
  if ((latency_ <= 0.0) && (suspended_ == 0)) {
    if (rate_ < 0)
      getModel()->getMaxminSystem()->update_variable_bound(getVariable(), lat_bound);
    else
      getModel()->getMaxminSystem()->update_variable_bound(getVariable(), std::min(rate_, lat_bound));
  }
}

int L07Action::unref()
{
  refcount_--;
  if (not refcount_) {
    if (action_hook.is_linked())
      simgrid::xbt::intrusive_erase(*stateSet_, *this);
    if (getVariable())
      getModel()->getMaxminSystem()->variable_free(getVariable());
    delete this;
    return 1;
  }
  return 0;
}

}
}
