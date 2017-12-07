/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "network_cm02.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/sg_config.h"
#include "src/instr/instr_private.hpp" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals
#include "src/kernel/lmm/maxmin.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

double sg_latency_factor = 1.0; /* default value; can be set by model or from command line */
double sg_bandwidth_factor = 1.0;       /* default value; can be set by model or from command line */
double sg_weight_S_parameter = 0.0;     /* default value; can be set by model or from command line */

double sg_tcp_gamma = 0.0;
int sg_network_crosstraffic = 0;

/************************************************************************/
/* New model based on optimizations discussed during Pedro Velho's thesis*/
/************************************************************************/
/* @techreport{VELHO:2011:HAL-00646896:1, */
/*      url = {http://hal.inria.fr/hal-00646896/en/}, */
/*      title = {{Flow-level network models: have we reached the limits?}}, */
/*      author = {Velho, Pedro and Schnorr, Lucas and Casanova, Henri and Legrand, Arnaud}, */
/*      type = {Rapport de recherche}, */
/*      institution = {INRIA}, */
/*      number = {RR-7821}, */
/*      year = {2011}, */
/*      month = Nov, */
/*      pdf = {http://hal.inria.fr/hal-00646896/PDF/rr-validity.pdf}, */
/*  } */
void surf_network_model_init_LegrandVelho()
{
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  all_existing_models->push_back(surf_network_model);

  xbt_cfg_setdefault_double("network/latency-factor",      13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor",     0.97);
  xbt_cfg_setdefault_double("network/weight-S",         20537);
}

/***************************************************************************/
/* The nice TCP sharing model designed by Loris Marchal and Henri Casanova */
/***************************************************************************/
/* @TechReport{      rr-lip2002-40, */
/*   author        = {Henri Casanova and Loris Marchal}, */
/*   institution   = {LIP}, */
/*   title         = {A Network Model for Simulation of Grid Application}, */
/*   number        = {2002-40}, */
/*   month         = {oct}, */
/*   year          = {2002} */
/* } */
void surf_network_model_init_CM02()
{

  if (surf_network_model)
    return;

  xbt_cfg_setdefault_double("network/latency-factor",   1.0);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 1.0);
  xbt_cfg_setdefault_double("network/weight-S",         0.0);

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  all_existing_models->push_back(surf_network_model);
}

/***************************************************************************/
/* The models from Steven H. Low                                           */
/***************************************************************************/
/* @article{Low03,                                                         */
/*   author={Steven H. Low},                                               */
/*   title={A Duality Model of {TCP} and Queue Management Algorithms},     */
/*   year={2003},                                                          */
/*   journal={{IEEE/ACM} Transactions on Networking},                      */
/*    volume={11}, number={4},                                             */
/*  }                                                                      */
void surf_network_model_init_Reno()
{
  if (surf_network_model)
    return;

  set_default_protocol_function(simgrid::kernel::lmm::func_reno_f, simgrid::kernel::lmm::func_reno_fp,
                                simgrid::kernel::lmm::func_reno_fpi);

  xbt_cfg_setdefault_double("network/latency-factor", 13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 0.97);
  xbt_cfg_setdefault_double("network/weight-S", 20537);

  surf_network_model = new simgrid::surf::NetworkCm02Model(&simgrid::kernel::lmm::lagrange_solve);
  all_existing_models->push_back(surf_network_model);
}


void surf_network_model_init_Reno2()
{
  if (surf_network_model)
    return;

  set_default_protocol_function(simgrid::kernel::lmm::func_reno2_f, simgrid::kernel::lmm::func_reno2_fp,
                                simgrid::kernel::lmm::func_reno2_fpi);

  xbt_cfg_setdefault_double("network/latency-factor", 13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 0.97);
  xbt_cfg_setdefault_double("network/weight-S", 20537);

  surf_network_model = new simgrid::surf::NetworkCm02Model(&simgrid::kernel::lmm::lagrange_solve);
  all_existing_models->push_back(surf_network_model);
}

void surf_network_model_init_Vegas()
{
  if (surf_network_model)
    return;

  set_default_protocol_function(simgrid::kernel::lmm::func_vegas_f, simgrid::kernel::lmm::func_vegas_fp,
                                simgrid::kernel::lmm::func_vegas_fpi);

  xbt_cfg_setdefault_double("network/latency-factor", 13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 0.97);
  xbt_cfg_setdefault_double("network/weight-S", 20537);

  surf_network_model = new simgrid::surf::NetworkCm02Model(&simgrid::kernel::lmm::lagrange_solve);
  all_existing_models->push_back(surf_network_model);
}

namespace simgrid {
namespace surf {

NetworkCm02Model::NetworkCm02Model()
  :NetworkModel()
{
  std::string optim = xbt_cfg_get_string("network/optim");
  bool select = xbt_cfg_get_boolean("network/maxmin-selective-update");

  if (optim == "Full") {
    setUpdateMechanism(UM_FULL);
    selectiveUpdate_ = select;
  } else if (optim == "Lazy") {
    setUpdateMechanism(UM_LAZY);
    selectiveUpdate_ = true;
    xbt_assert(select || (xbt_cfg_is_default_value("network/maxmin-selective-update")),
               "You cannot disable selective update when using the lazy update mechanism");
  } else {
    xbt_die("Unsupported optimization (%s) for this model. Accepted: Full, Lazy.", optim.c_str());
  }

  maxminSystem_ = new simgrid::kernel::lmm::System(selectiveUpdate_);
  loopback_     = NetworkCm02Model::createLink("__loopback__", 498000000, 0.000015, SURF_LINK_FATPIPE);

  if (getUpdateMechanism() == UM_LAZY) {
    modifiedSet_ = new ActionLmmList();
    maxminSystem_->keep_track = modifiedSet_;
  }
}

NetworkCm02Model::NetworkCm02Model(void (*specificSolveFun)(lmm_system_t self)) : NetworkCm02Model()
{
  maxminSystem_->solve_fun = specificSolveFun;
}

LinkImpl* NetworkCm02Model::createLink(const std::string& name, double bandwidth, double latency,
                                       e_surf_link_sharing_policy_t policy)
{
  return new NetworkCm02Link(this, name, bandwidth, latency, policy, maxminSystem_);
}

void NetworkCm02Model::updateActionsStateLazy(double now, double /*delta*/)
{
  while (not actionHeapIsEmpty() && double_equals(actionHeapTopDate(), now, sg_surf_precision)) {

    NetworkCm02Action* action = static_cast<NetworkCm02Action*>(actionHeapPop());
    XBT_DEBUG("Something happened to action %p", action);
    if (TRACE_is_enabled()) {
      int n = action->getVariable()->get_number_of_constraint();

      for (int i = 0; i < n; i++){
        lmm_constraint_t constraint = action->getVariable()->get_constraint(i);
        NetworkCm02Link* link       = static_cast<NetworkCm02Link*>(constraint->get_id());
        double value = action->getVariable()->get_value() * action->getVariable()->get_constraint_weight(i);
        TRACE_surf_link_set_utilization(link->getCname(), action->getCategory(), value, action->getLastUpdate(),
                                        now - action->getLastUpdate());
      }
    }

    // if I am wearing a latency hat
    if (action->getHat() == LATENCY) {
      XBT_DEBUG("Latency paid for action %p. Activating", action);
      maxminSystem_->update_variable_weight(action->getVariable(), action->weight_);
      action->heapRemove(getActionHeap());
      action->refreshLastUpdate();

        // if I am wearing a max_duration or normal hat
    } else if (action->getHat() == MAX_DURATION || action->getHat() == NORMAL) {
        // no need to communicate anymore
        // assume that flows that reached max_duration have remaining of 0
      XBT_DEBUG("Action %p finished", action);
      action->setRemains(0);
      action->finish(Action::State::done);
      action->heapRemove(getActionHeap());
    }
  }
}


void NetworkCm02Model::updateActionsStateFull(double now, double delta)
{
  for (auto it = std::begin(*getRunningActionSet()); it != std::end(*getRunningActionSet());) {
    NetworkCm02Action& action = static_cast<NetworkCm02Action&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    XBT_DEBUG("Something happened to action %p", &action);
    double deltap = delta;
    if (action.latency_ > 0) {
      if (action.latency_ > deltap) {
        double_update(&action.latency_, deltap, sg_surf_precision);
        deltap = 0.0;
      } else {
        double_update(&deltap, action.latency_, sg_surf_precision);
        action.latency_ = 0.0;
      }
      if (action.latency_ <= 0.0 && not action.isSuspended())
        maxminSystem_->update_variable_weight(action.getVariable(), action.weight_);
    }
    if (TRACE_is_enabled()) {
      int n = action.getVariable()->get_number_of_constraint();
      for (int i = 0; i < n; i++) {
        lmm_constraint_t constraint = action.getVariable()->get_constraint(i);
        NetworkCm02Link* link = static_cast<NetworkCm02Link*>(constraint->get_id());
        TRACE_surf_link_set_utilization(
            link->getCname(), action.getCategory(),
            (action.getVariable()->get_value() * action.getVariable()->get_constraint_weight(i)),
            action.getLastUpdate(), now - action.getLastUpdate());
      }
    }
    if (not action.getVariable()->get_number_of_constraint()) {
      /* There is actually no link used, hence an infinite bandwidth. This happens often when using models like
       * vivaldi. In such case, just make sure that the action completes immediately.
       */
      action.updateRemains(action.getRemains());
    }
    action.updateRemains(action.getVariable()->get_value() * delta);

    if (action.getMaxDuration() > NO_MAX_DURATION)
      action.updateMaxDuration(delta);

    if (((action.getRemains() <= 0) && (action.getVariable()->get_weight() > 0)) ||
        ((action.getMaxDuration() > NO_MAX_DURATION) && (action.getMaxDuration() <= 0))) {
      action.finish(Action::State::done);
    }
  }
}

Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  int failed = 0;
  double latency = 0.0;
  std::vector<LinkImpl*> back_route;
  std::vector<LinkImpl*> route;

  XBT_IN("(%s,%s,%g,%g)", src->getCname(), dst->getCname(), size, rate);

  src->routeTo(dst, route, &latency);
  xbt_assert(not route.empty() || latency,
             "You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
             src->getCname(), dst->getCname());

  for (auto const& link : route)
    if (link->isOff())
      failed = 1;

  if (sg_network_crosstraffic == 1) {
    dst->routeTo(src, back_route, nullptr);
    for (auto const& link : back_route)
      if (link->isOff())
        failed = 1;
  }

  NetworkCm02Action *action = new NetworkCm02Action(this, size, failed);
  action->weight_ = latency;
  action->latency_ = latency;
  action->rate_ = rate;
  if (getUpdateMechanism() == UM_LAZY) {
    action->refreshLastUpdate();
  }

  double bandwidth_bound = -1.0;
  if (sg_weight_S_parameter > 0)
    for (auto const& link : route)
      action->weight_ += sg_weight_S_parameter / link->bandwidth();

  for (auto const& link : route) {
    double bb       = bandwidthFactor(size) * link->bandwidth();
    bandwidth_bound = (bandwidth_bound < 0.0) ? bb : std::min(bandwidth_bound, bb);
  }

  action->latCurrent_ = action->latency_;
  action->latency_ *= latencyFactor(size);
  action->rate_ = bandwidthConstraint(action->rate_, bandwidth_bound, size);

  int constraints_per_variable = route.size();
  constraints_per_variable += back_route.size();

  if (action->latency_ > 0) {
    action->setVariable(maxminSystem_->variable_new(action, 0.0, -1.0, constraints_per_variable));
    if (getUpdateMechanism() == UM_LAZY) {
      // add to the heap the event when the latency is payed
      XBT_DEBUG("Added action (%p) one latency event at date %f", action, action->latency_ + action->getLastUpdate());
      action->heapInsert(getActionHeap(), action->latency_ + action->getLastUpdate(), route.empty() ? NORMAL : LATENCY);
    }
  } else
    action->setVariable(maxminSystem_->variable_new(action, 1.0, -1.0, constraints_per_variable));

  if (action->rate_ < 0) {
    maxminSystem_->update_variable_bound(action->getVariable(),
                                         (action->latCurrent_ > 0) ? sg_tcp_gamma / (2.0 * action->latCurrent_) : -1.0);
  } else {
    maxminSystem_->update_variable_bound(action->getVariable(),
                                         (action->latCurrent_ > 0)
                                             ? std::min(action->rate_, sg_tcp_gamma / (2.0 * action->latCurrent_))
                                             : action->rate_);
  }

  for (auto const& link : route)
    maxminSystem_->expand(link->constraint(), action->getVariable(), 1.0);

  if (not back_route.empty()) { //  sg_network_crosstraffic was activated
    XBT_DEBUG("Fullduplex active adding backward flow using 5%%");
    for (auto const& link : back_route)
      maxminSystem_->expand(link->constraint(), action->getVariable(), .05);

    // Change concurrency_share here, if you want that cross-traffic is included in the SURF concurrency
    // (You would also have to change simgrid::kernel::lmm::Element::get_concurrency())
    // action->getVariable()->set_concurrency_share(2)
  }
  XBT_OUT();

  simgrid::s4u::Link::onCommunicate(action, src, dst);
  return action;
}

/************
 * Resource *
 ************/
NetworkCm02Link::NetworkCm02Link(NetworkCm02Model* model, const std::string& name, double bandwidth, double latency,
                                 e_surf_link_sharing_policy_t policy, lmm_system_t system)
    : LinkImpl(model, name, system->constraint_new(this, sg_bandwidth_factor * bandwidth))
{
  bandwidth_.scale = 1.0;
  bandwidth_.peak  = bandwidth;

  latency_.scale = 1.0;
  latency_.peak  = latency;

  if (policy == SURF_LINK_FATPIPE)
    constraint()->unshare();

  simgrid::s4u::Link::onCreation(this->piface_);
}

void NetworkCm02Link::apply_event(tmgr_trace_event_t triggered, double value)
{
  /* Find out which of my iterators was triggered, and react accordingly */
  if (triggered == bandwidth_.event) {
    setBandwidth(value);
    tmgr_trace_event_unref(&bandwidth_.event);

  } else if (triggered == latency_.event) {
    setLatency(value);
    tmgr_trace_event_unref(&latency_.event);

  } else if (triggered == stateEvent_) {
    if (value > 0)
      turnOn();
    else {
      lmm_variable_t var       = nullptr;
      const_lmm_element_t elem = nullptr;
      double now               = surf_get_clock();

      turnOff();
      while ((var = constraint()->get_variable(&elem))) {
        Action* action = static_cast<Action*>(var->get_id());

        if (action->getState() == Action::State::running ||
            action->getState() == Action::State::ready) {
          action->setFinishTime(now);
          action->setState(Action::State::failed);
        }
      }
    }
    tmgr_trace_event_unref(&stateEvent_);
  } else {
    xbt_die("Unknown event!\n");
  }

  XBT_DEBUG("There was a resource state event, need to update actions related to the constraint (%p)", constraint());
}

void NetworkCm02Link::setBandwidth(double value)
{
  bandwidth_.peak = value;

  model()->getMaxminSystem()->update_constraint_bound(constraint(),
                                                      sg_bandwidth_factor * (bandwidth_.peak * bandwidth_.scale));
  TRACE_surf_link_set_bandwidth(surf_get_clock(), getCname(), sg_bandwidth_factor * bandwidth_.peak * bandwidth_.scale);

  if (sg_weight_S_parameter > 0) {
    double delta = sg_weight_S_parameter / value - sg_weight_S_parameter / (bandwidth_.peak * bandwidth_.scale);

    lmm_variable_t var;
    const_lmm_element_t elem     = nullptr;
    const_lmm_element_t nextelem = nullptr;
    int numelem                  = 0;
    while ((var = constraint()->get_variable_safe(&elem, &nextelem, &numelem))) {
      NetworkCm02Action* action = static_cast<NetworkCm02Action*>(var->get_id());
      action->weight_ += delta;
      if (not action->isSuspended())
        model()->getMaxminSystem()->update_variable_weight(action->getVariable(), action->weight_);
    }
  }
}

void NetworkCm02Link::setLatency(double value)
{
  double delta                 = value - latency_.peak;
  lmm_variable_t var           = nullptr;
  const_lmm_element_t elem     = nullptr;
  const_lmm_element_t nextelem = nullptr;
  int numelem                  = 0;

  latency_.peak = value;

  while ((var = constraint()->get_variable_safe(&elem, &nextelem, &numelem))) {
    NetworkCm02Action* action = static_cast<NetworkCm02Action*>(var->get_id());
    action->latCurrent_ += delta;
    action->weight_ += delta;
    if (action->rate_ < 0)
      model()->getMaxminSystem()->update_variable_bound(action->getVariable(),
                                                        sg_tcp_gamma / (2.0 * action->latCurrent_));
    else {
      model()->getMaxminSystem()->update_variable_bound(
          action->getVariable(), std::min(action->rate_, sg_tcp_gamma / (2.0 * action->latCurrent_)));

      if (action->rate_ < sg_tcp_gamma / (2.0 * action->latCurrent_)) {
        XBT_INFO("Flow is limited BYBANDWIDTH");
      } else {
        XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f", action->latCurrent_);
      }
    }
    if (not action->isSuspended())
      model()->getMaxminSystem()->update_variable_weight(action->getVariable(), action->weight_);
  }
}

/**********
 * Action *
 **********/

void NetworkCm02Action::updateRemainingLazy(double now)
{
  if (suspended_ != 0)
    return;

  double delta        = now - getLastUpdate();
  double max_duration = getMaxDuration();

  if (getRemainsNoUpdate() > 0) {
    XBT_DEBUG("Updating action(%p): remains was %f, last_update was: %f", this, getRemainsNoUpdate(), getLastUpdate());
    updateRemains(getLastValue() * delta);

    XBT_DEBUG("Updating action(%p): remains is now %f", this, getRemainsNoUpdate());
  }

  if (max_duration > NO_MAX_DURATION) {
    double_update(&max_duration, delta, sg_surf_precision);
    setMaxDuration(max_duration);
  }

  if ((getRemainsNoUpdate() <= 0 && (getVariable()->get_weight() > 0)) ||
      ((max_duration > NO_MAX_DURATION) && (max_duration <= 0))) {
    finish(Action::State::done);
    heapRemove(getModel()->getActionHeap());
  }

  refreshLastUpdate();
  setLastValue(getVariable()->get_value());
}

}
}
