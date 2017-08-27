/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "maxmin_private.hpp"
#include "network_cm02.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/sg_config.h"
#include "src/instr/instr_private.h" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals

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

  lmm_set_default_protocol_function(func_reno_f, func_reno_fp, func_reno_fpi);

  xbt_cfg_setdefault_double("network/latency-factor", 13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 0.97);
  xbt_cfg_setdefault_double("network/weight-S", 20537);

  surf_network_model = new simgrid::surf::NetworkCm02Model(&lagrange_solve);
  all_existing_models->push_back(surf_network_model);
}


void surf_network_model_init_Reno2()
{
  if (surf_network_model)
    return;

  lmm_set_default_protocol_function(func_reno2_f, func_reno2_fp, func_reno2_fpi);

  xbt_cfg_setdefault_double("network/latency-factor", 13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 0.97);
  xbt_cfg_setdefault_double("network/weight-S", 20537);

  surf_network_model = new simgrid::surf::NetworkCm02Model(&lagrange_solve);
  all_existing_models->push_back(surf_network_model);
}

void surf_network_model_init_Vegas()
{
  if (surf_network_model)
    return;

  lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp, func_vegas_fpi);

  xbt_cfg_setdefault_double("network/latency-factor", 13.01);
  xbt_cfg_setdefault_double("network/bandwidth-factor", 0.97);
  xbt_cfg_setdefault_double("network/weight-S", 20537);

  surf_network_model = new simgrid::surf::NetworkCm02Model(&lagrange_solve);
  all_existing_models->push_back(surf_network_model);
}

namespace simgrid {
namespace surf {

NetworkCm02Model::NetworkCm02Model()
  :NetworkModel()
{
  char *optim = xbt_cfg_get_string("network/optim");
  bool select = xbt_cfg_get_boolean("network/maxmin-selective-update");

  if (not strcmp(optim, "Full")) {
    updateMechanism_ = UM_FULL;
    selectiveUpdate_ = select;
  } else if (not strcmp(optim, "Lazy")) {
    updateMechanism_ = UM_LAZY;
    selectiveUpdate_ = true;
    xbt_assert(select || (xbt_cfg_is_default_value("network/maxmin-selective-update")),
               "You cannot disable selective update when using the lazy update mechanism");
  } else {
    xbt_die("Unsupported optimization (%s) for this model. Accepted: Full, Lazy.", optim);
  }

  maxminSystem_ = lmm_system_new(selectiveUpdate_);
  loopback_     = createLink("__loopback__", 498000000, 0.000015, SURF_LINK_FATPIPE);

  if (updateMechanism_ == UM_LAZY) {
    actionHeap_ = xbt_heap_new(8, nullptr);
    xbt_heap_set_update_callback(actionHeap_, surf_action_lmm_update_index_heap);
    modifiedSet_ = new ActionLmmList();
    maxminSystem_->keep_track = modifiedSet_;
  }
}
NetworkCm02Model::NetworkCm02Model(void (*specificSolveFun)(lmm_system_t self))
  : NetworkCm02Model()
{
  maxminSystem_->solve_fun = specificSolveFun;
}

LinkImpl* NetworkCm02Model::createLink(const char* name, double bandwidth, double latency,
                                       e_surf_link_sharing_policy_t policy)
{
  return new NetworkCm02Link(this, name, bandwidth, latency, policy, maxminSystem_);
}

void NetworkCm02Model::updateActionsStateLazy(double now, double /*delta*/)
{
  while ((xbt_heap_size(actionHeap_) > 0)
         && (double_equals(xbt_heap_maxkey(actionHeap_), now, sg_surf_precision))) {

    NetworkCm02Action *action = static_cast<NetworkCm02Action*> (xbt_heap_pop(actionHeap_));
    XBT_DEBUG("Something happened to action %p", action);
    if (TRACE_is_enabled()) {
      int n = lmm_get_number_of_cnst_from_var(maxminSystem_, action->getVariable());

      for (int i = 0; i < n; i++){
        lmm_constraint_t constraint = lmm_get_cnst_from_var(maxminSystem_, action->getVariable(), i);
        NetworkCm02Link *link = static_cast<NetworkCm02Link*>(lmm_constraint_id(constraint));
        double value = lmm_variable_getvalue(action->getVariable())*
            lmm_get_cnst_weight_from_var(maxminSystem_, action->getVariable(), i);
        TRACE_surf_link_set_utilization(link->cname(), action->getCategory(), value, action->getLastUpdate(),
                                        now - action->getLastUpdate());
      }
    }

    // if I am wearing a latency hat
    if (action->getHat() == LATENCY) {
      XBT_DEBUG("Latency paid for action %p. Activating", action);
      lmm_update_variable_weight(maxminSystem_, action->getVariable(), action->weight_);
      action->heapRemove(actionHeap_);
      action->refreshLastUpdate();

        // if I am wearing a max_duration or normal hat
    } else if (action->getHat() == MAX_DURATION || action->getHat() == NORMAL) {
        // no need to communicate anymore
        // assume that flows that reached max_duration have remaining of 0
      XBT_DEBUG("Action %p finished", action);
      action->setRemains(0);
      action->finish();
      action->setState(Action::State::done);
      action->heapRemove(actionHeap_);
    }
  }
}


void NetworkCm02Model::updateActionsStateFull(double now, double delta)
{
  ActionList *running_actions = getRunningActionSet();

  for(ActionList::iterator it(running_actions->begin()), itNext=it, itend(running_actions->end())
     ; it != itend ; it=itNext) {
    ++itNext;

    NetworkCm02Action *action = static_cast<NetworkCm02Action*> (&*it);
    XBT_DEBUG("Something happened to action %p", action);
      double deltap = delta;
      if (action->latency_ > 0) {
        if (action->latency_ > deltap) {
          double_update(&(action->latency_), deltap, sg_surf_precision);
          deltap = 0.0;
        } else {
          double_update(&(deltap), action->latency_, sg_surf_precision);
          action->latency_ = 0.0;
        }
        if (action->latency_ <= 0.0 && not action->isSuspended())
          lmm_update_variable_weight(maxminSystem_, action->getVariable(), action->weight_);
      }
      if (TRACE_is_enabled()) {
        int n = lmm_get_number_of_cnst_from_var(maxminSystem_, action->getVariable());
        for (int i = 0; i < n; i++){
          lmm_constraint_t constraint = lmm_get_cnst_from_var(maxminSystem_, action->getVariable(), i);

          NetworkCm02Link* link = static_cast<NetworkCm02Link*>(lmm_constraint_id(constraint));
          TRACE_surf_link_set_utilization(link->cname(), action->getCategory(),
                                          (lmm_variable_getvalue(action->getVariable()) *
                                           lmm_get_cnst_weight_from_var(maxminSystem_, action->getVariable(), i)),
                                          action->getLastUpdate(), now - action->getLastUpdate());
        }
      }
      if (not lmm_get_number_of_cnst_from_var(maxminSystem_, action->getVariable())) {
        /* There is actually no link used, hence an infinite bandwidth. This happens often when using models like
         * vivaldi. In such case, just make sure that the action completes immediately.
         */
        action->updateRemains(action->getRemains());
      }
    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);

    if (action->getMaxDuration() > NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    if (((action->getRemains() <= 0) && (lmm_get_variable_weight(action->getVariable()) > 0)) ||
        ((action->getMaxDuration() > NO_MAX_DURATION) && (action->getMaxDuration() <= 0))) {
      action->finish();
      action->setState(Action::State::done);
    }
  }
}

Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  int failed = 0;
  double latency = 0.0;
  std::vector<LinkImpl*>* back_route = nullptr;
  std::vector<LinkImpl*>* route = new std::vector<LinkImpl*>();

  XBT_IN("(%s,%s,%g,%g)", src->getCname(), dst->getCname(), size, rate);

  src->routeTo(dst, route, &latency);
  xbt_assert(not route->empty() || latency,
             "You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
             src->getCname(), dst->getCname());

  for (auto const& link : *route)
    if (link->isOff())
      failed = 1;

  if (sg_network_crosstraffic == 1) {
    back_route = new std::vector<LinkImpl*>();
    dst->routeTo(src, back_route, nullptr);
    for (auto const& link : *back_route)
      if (link->isOff())
        failed = 1;
  }

  NetworkCm02Action *action = new NetworkCm02Action(this, size, failed);
  action->weight_ = latency;
  action->latency_ = latency;
  action->rate_ = rate;
  if (updateMechanism_ == UM_LAZY) {
    action->indexHeap_ = -1;
    action->lastUpdate_ = surf_get_clock();
  }

  double bandwidth_bound = -1.0;
  if (sg_weight_S_parameter > 0)
    for (auto const& link : *route)
      action->weight_ += sg_weight_S_parameter / link->bandwidth();

  for (auto const& link : *route) {
    double bb       = bandwidthFactor(size) * link->bandwidth();
    bandwidth_bound = (bandwidth_bound < 0.0) ? bb : std::min(bandwidth_bound, bb);
  }

  action->latCurrent_ = action->latency_;
  action->latency_ *= latencyFactor(size);
  action->rate_ = bandwidthConstraint(action->rate_, bandwidth_bound, size);

  int constraints_per_variable = route->size();
  if (back_route != nullptr)
    constraints_per_variable += back_route->size();

  if (action->latency_ > 0) {
    action->variable_ = lmm_variable_new(maxminSystem_, action, 0.0, -1.0, constraints_per_variable);
    if (updateMechanism_ == UM_LAZY) {
      // add to the heap the event when the latency is payed
      XBT_DEBUG("Added action (%p) one latency event at date %f", action, action->latency_ + action->lastUpdate_);
      action->heapInsert(actionHeap_, action->latency_ + action->lastUpdate_, route->empty() ? NORMAL : LATENCY);
    }
  } else
    action->variable_ = lmm_variable_new(maxminSystem_, action, 1.0, -1.0, constraints_per_variable);

  if (action->rate_ < 0) {
    lmm_update_variable_bound(maxminSystem_, action->getVariable(), (action->latCurrent_ > 0) ? sg_tcp_gamma / (2.0 * action->latCurrent_) : -1.0);
  } else {
    lmm_update_variable_bound(maxminSystem_, action->getVariable(), (action->latCurrent_ > 0) ? std::min(action->rate_, sg_tcp_gamma / (2.0 * action->latCurrent_)) : action->rate_);
  }

  for (auto const& link : *route)
    lmm_expand(maxminSystem_, link->constraint(), action->getVariable(), 1.0);

  if (back_route != nullptr) { //  sg_network_crosstraffic was activated
    XBT_DEBUG("Fullduplex active adding backward flow using 5%%");
    for (auto const& link : *back_route)
      lmm_expand(maxminSystem_, link->constraint(), action->getVariable(), .05);

    //Change concurrency_share here, if you want that cross-traffic is included in the SURF concurrency
    //(You would also have to change lmm_element_concurrency())
    //lmm_variable_concurrency_share_set(action->getVariable(),2);
  }

  delete route;
  delete back_route;
  XBT_OUT();

  simgrid::s4u::Link::onCommunicate(action, src, dst);
  return action;
}

/************
 * Resource *
 ************/
NetworkCm02Link::NetworkCm02Link(NetworkCm02Model* model, const char* name, double bandwidth, double latency,
                                 e_surf_link_sharing_policy_t policy, lmm_system_t system)
    : LinkImpl(model, name, lmm_constraint_new(system, this, sg_bandwidth_factor * bandwidth))
{
  bandwidth_.scale = 1.0;
  bandwidth_.peak  = bandwidth;

  latency_.scale = 1.0;
  latency_.peak  = latency;

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(constraint());

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
      lmm_variable_t var = nullptr;
      lmm_element_t elem = nullptr;
      double now = surf_get_clock();

      turnOff();
      while ((var = lmm_get_var_from_cnst(model()->getMaxminSystem(), constraint(), &elem))) {
        Action *action = static_cast<Action*>( lmm_variable_id(var) );

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

  lmm_update_constraint_bound(model()->getMaxminSystem(), constraint(),
                              sg_bandwidth_factor * (bandwidth_.peak * bandwidth_.scale));
  TRACE_surf_link_set_bandwidth(surf_get_clock(), cname(), sg_bandwidth_factor * bandwidth_.peak * bandwidth_.scale);

  if (sg_weight_S_parameter > 0) {
    double delta = sg_weight_S_parameter / value - sg_weight_S_parameter / (bandwidth_.peak * bandwidth_.scale);

    lmm_variable_t var;
    lmm_element_t elem = nullptr;
    lmm_element_t nextelem = nullptr;
    int numelem = 0;
    while ((var = lmm_get_var_from_cnst_safe(model()->getMaxminSystem(), constraint(), &elem, &nextelem, &numelem))) {
      NetworkCm02Action *action = static_cast<NetworkCm02Action*>(lmm_variable_id(var));
      action->weight_ += delta;
      if (not action->isSuspended())
        lmm_update_variable_weight(model()->getMaxminSystem(), action->getVariable(), action->weight_);
    }
  }
}

void NetworkCm02Link::setLatency(double value)
{
  double delta           = value - latency_.peak;
  lmm_variable_t var = nullptr;
  lmm_element_t elem = nullptr;
  lmm_element_t nextelem = nullptr;
  int numelem = 0;

  latency_.peak = value;

  while ((var = lmm_get_var_from_cnst_safe(model()->getMaxminSystem(), constraint(), &elem, &nextelem, &numelem))) {
    NetworkCm02Action *action = static_cast<NetworkCm02Action*>(lmm_variable_id(var));
    action->latCurrent_ += delta;
    action->weight_ += delta;
    if (action->rate_ < 0)
      lmm_update_variable_bound(model()->getMaxminSystem(), action->getVariable(),
                                sg_tcp_gamma / (2.0 * action->latCurrent_));
    else {
      lmm_update_variable_bound(model()->getMaxminSystem(), action->getVariable(),
                                std::min(action->rate_, sg_tcp_gamma / (2.0 * action->latCurrent_)));

      if (action->rate_ < sg_tcp_gamma / (2.0 * action->latCurrent_)) {
        XBT_INFO("Flow is limited BYBANDWIDTH");
      } else {
        XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f", action->latCurrent_);
      }
    }
    if (not action->isSuspended())
      lmm_update_variable_weight(model()->getMaxminSystem(), action->getVariable(), action->weight_);
  }
}

/**********
 * Action *
 **********/

void NetworkCm02Action::updateRemainingLazy(double now)
{
  if (suspended_ != 0)
    return;

  double delta = now - lastUpdate_;

  if (remains_ > 0) {
    XBT_DEBUG("Updating action(%p): remains was %f, last_update was: %f", this, remains_, lastUpdate_);
    double_update(&(remains_), lastValue_ * delta, sg_maxmin_precision*sg_surf_precision);

    XBT_DEBUG("Updating action(%p): remains is now %f", this, remains_);
  }

  if (maxDuration_ > NO_MAX_DURATION)
    double_update(&maxDuration_, delta, sg_surf_precision);

  if ((remains_ <= 0 && (lmm_get_variable_weight(getVariable()) > 0)) ||
      ((maxDuration_ > NO_MAX_DURATION) && (maxDuration_ <= 0))) {
    finish();
    setState(Action::State::done);
    heapRemove(getModel()->getActionHeap());
  }

  lastUpdate_ = now;
  lastValue_ = lmm_variable_getvalue(getVariable());
}

}
}
