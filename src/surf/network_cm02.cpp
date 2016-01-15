/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "network_cm02.hpp"
#include "maxmin_private.hpp"
#include "simgrid/sg_config.h"
#include "src/surf/platform.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

double sg_sender_gap = 0.0;
double sg_latency_factor = 1.0; /* default value; can be set by model or from command line */
double sg_bandwidth_factor = 1.0;       /* default value; can be set by model or from command line */
double sg_weight_S_parameter = 0.0;     /* default value; can be set by model or from command line */

double sg_tcp_gamma = 0.0;
int sg_network_crosstraffic = 0;

/*************
 * CallBacks *
 *************/

void net_define_callbacks(void)
{
  /* Figuring out the network links */
  simgrid::surf::on_link.connect(netlink_parse_init);
  simgrid::surf::on_postparse.connect([]() {
    surf_network_model->addTraces();
  });
}

/*********
 * Model *
 *********/

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
void surf_network_model_init_LegrandVelho(void)
{
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  net_define_callbacks();
  simgrid::surf::Model *model = surf_network_model;
  xbt_dynar_push(all_existing_models, &model);

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor",
                            13.01);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor",
                            0.97);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 20537);
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
void surf_network_model_init_CM02(void)
{

  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  xbt_dynar_push(all_existing_models, &surf_network_model);
  net_define_callbacks();

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 1.0);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor", 1.0);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 0.0);
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
void surf_network_model_init_Reno(void)
{
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  xbt_dynar_push(all_existing_models, &surf_network_model);
  net_define_callbacks();
  lmm_set_default_protocol_function(func_reno_f, func_reno_fp, func_reno_fpi);
  surf_network_model->f_networkSolve = lagrange_solve;

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);
}


void surf_network_model_init_Reno2(void)
{
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  xbt_dynar_push(all_existing_models, &surf_network_model);
  net_define_callbacks();
  lmm_set_default_protocol_function(func_reno2_f, func_reno2_fp, func_reno2_fpi);
  surf_network_model->f_networkSolve = lagrange_solve;

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);
}

void surf_network_model_init_Vegas(void)
{
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkCm02Model();
  xbt_dynar_push(all_existing_models, &surf_network_model);
  net_define_callbacks();
  lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp, func_vegas_fpi);
  surf_network_model->f_networkSolve = lagrange_solve;

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor", 0.92);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);
}

namespace simgrid {
namespace surf {

NetworkCm02Model::NetworkCm02Model()
	:NetworkModel()
{
  char *optim = xbt_cfg_get_string(_sg_cfg_set, "network/optim");
  int select =
      xbt_cfg_get_boolean(_sg_cfg_set, "network/maxmin_selective_update");

  if (!strcmp(optim, "Full")) {
    p_updateMechanism = UM_FULL;
    m_selectiveUpdate = select;
  } else if (!strcmp(optim, "Lazy")) {
    p_updateMechanism = UM_LAZY;
    m_selectiveUpdate = 1;
    xbt_assert((select == 1)
               ||
               (xbt_cfg_is_default_value
                (_sg_cfg_set, "network/maxmin_selective_update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }

  if (!p_maxminSystem)
	p_maxminSystem = lmm_system_new(m_selectiveUpdate);

  routing_model_create(createLink("__loopback__",
	                              498000000, NULL, 0.000015, NULL,
	                              1 /*SURF_RESOURCE_ON*/, NULL,
	                              SURF_LINK_FATPIPE, NULL));

  if (p_updateMechanism == UM_LAZY) {
	p_actionHeap = xbt_heap_new(8, NULL);
	xbt_heap_set_update_callback(p_actionHeap, surf_action_lmm_update_index_heap);
	p_modifiedSet = new ActionLmmList();
	p_maxminSystem->keep_track = p_modifiedSet;
  }
}

Link* NetworkCm02Model::createLink(const char *name,
                                 double bw_initial,
                                 tmgr_trace_t bw_trace,
                                 double lat_initial,
                                 tmgr_trace_t lat_trace,
                                 int initiallyOn,
                                 tmgr_trace_t state_trace,
                                 e_surf_link_sharing_policy_t policy,
                                 xbt_dict_t properties)
{
  xbt_assert(NULL == Link::byName(name),
             "Link '%s' declared several times in the platform",
             name);

  Link* link = new NetworkCm02Link(this, name, properties, p_maxminSystem, sg_bandwidth_factor * bw_initial, history,
				             initiallyOn, state_trace, bw_initial, bw_trace, lat_initial, lat_trace, policy);
  Link::onCreation(link);
  return link;
}

void NetworkCm02Model::updateActionsStateLazy(double now, double /*delta*/)
{
  NetworkCm02Action *action;
  while ((xbt_heap_size(p_actionHeap) > 0)
         && (double_equals(xbt_heap_maxkey(p_actionHeap), now, sg_surf_precision))) {
    action = static_cast<NetworkCm02Action*> (xbt_heap_pop(p_actionHeap));
    XBT_DEBUG("Something happened to action %p", action);
    if (TRACE_is_enabled()) {
      int n = lmm_get_number_of_cnst_from_var(p_maxminSystem, action->getVariable());
      int i;
      for (i = 0; i < n; i++){
        lmm_constraint_t constraint = lmm_get_cnst_from_var(p_maxminSystem,
                                                            action->getVariable(),
                                                            i);
        NetworkCm02Link *link = static_cast<NetworkCm02Link*>(lmm_constraint_id(constraint));
        TRACE_surf_link_set_utilization(link->getName(),
                                        action->getCategory(),
                                        (lmm_variable_getvalue(action->getVariable())*
                                            lmm_get_cnst_weight_from_var(p_maxminSystem,
                                                action->getVariable(),
                                                i)),
                                        action->getLastUpdate(),
                                        now - action->getLastUpdate());
      }
    }

    // if I am wearing a latency hat
    if (action->getHat() == LATENCY) {
      XBT_DEBUG("Latency paid for action %p. Activating", action);
      lmm_update_variable_weight(p_maxminSystem, action->getVariable(), action->m_weight);
      action->heapRemove(p_actionHeap);
      action->refreshLastUpdate();

        // if I am wearing a max_duration or normal hat
    } else if (action->getHat() == MAX_DURATION ||
        action->getHat() == NORMAL) {
        // no need to communicate anymore
        // assume that flows that reached max_duration have remaining of 0
      XBT_DEBUG("Action %p finished", action);
      action->setRemains(0);
      action->finish();
      action->setState(SURF_ACTION_DONE);
      action->heapRemove(p_actionHeap);

      action->gapRemove();
    }
  }
  return;
}


void NetworkCm02Model::updateActionsStateFull(double now, double delta)
{
  NetworkCm02Action *action;
  ActionList *running_actions = getRunningActionSet();

  for(ActionList::iterator it(running_actions->begin()), itNext=it, itend(running_actions->end())
     ; it != itend ; it=itNext) {
	++itNext;

    action = static_cast<NetworkCm02Action*> (&*it);
    XBT_DEBUG("Something happened to action %p", action);
      double deltap = delta;
      if (action->m_latency > 0) {
        if (action->m_latency > deltap) {
          double_update(&(action->m_latency), deltap, sg_surf_precision);
          deltap = 0.0;
        } else {
          double_update(&(deltap), action->m_latency, sg_surf_precision);
          action->m_latency = 0.0;
        }
        if (action->m_latency == 0.0 && !(action->isSuspended()))
          lmm_update_variable_weight(p_maxminSystem, action->getVariable(),
              action->m_weight);
      }
      if (TRACE_is_enabled()) {
        int n = lmm_get_number_of_cnst_from_var(p_maxminSystem, action->getVariable());
        int i;
        for (i = 0; i < n; i++){
          lmm_constraint_t constraint = lmm_get_cnst_from_var(p_maxminSystem,
                                                            action->getVariable(),
                                                            i);
          NetworkCm02Link* link = static_cast<NetworkCm02Link*>(lmm_constraint_id(constraint));
          TRACE_surf_link_set_utilization(link->getName(),
                                        action->getCategory(),
                                        (lmm_variable_getvalue(action->getVariable())*
                                            lmm_get_cnst_weight_from_var(p_maxminSystem,
                                                action->getVariable(),
                                                i)),
                                        action->getLastUpdate(),
                                        now - action->getLastUpdate());
        }
      }
      if (!lmm_get_number_of_cnst_from_var
          (p_maxminSystem, action->getVariable())) {
        /* There is actually no link used, hence an infinite bandwidth.
         * This happens often when using models like vivaldi.
         * In such case, just make sure that the action completes immediately.
         */
        action->updateRemains(action->getRemains());
      }
    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);
                  
    if (action->getMaxDuration() != NO_MAX_DURATION)
      action->updateMaxDuration(delta);
      
    if ((action->getRemains() <= 0) &&
        (lmm_get_variable_weight(action->getVariable()) > 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
      action->gapRemove();
    } else if (((action->getMaxDuration() != NO_MAX_DURATION)
        && (action->getMaxDuration() <= 0))) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
      action->gapRemove();
    }
  }
  return;
}

Action *NetworkCm02Model::communicate(NetCard *src, NetCard *dst,
                                                double size, double rate)
{
  unsigned int i;
  void *_link;
  NetworkCm02Link *link;
  int failed = 0;
  NetworkCm02Action *action = NULL;
  double bandwidth_bound;
  double latency = 0.0;
  xbt_dynar_t back_route = NULL;
  int constraints_per_variable = 0;

  xbt_dynar_t route = xbt_dynar_new(sizeof(NetCard*), NULL);

  XBT_IN("(%s,%s,%g,%g)", src->getName(), dst->getName(), size, rate);

  routing_platf->getRouteAndLatency(src, dst, &route, &latency);
  xbt_assert(!xbt_dynar_is_empty(route) || latency,
             "You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
             src->getName(), dst->getName());

  xbt_dynar_foreach(route, i, _link) {
	link = static_cast<NetworkCm02Link*>(_link);
    if (link->isOff()) {
      failed = 1;
      break;
    }
  }
  if (sg_network_crosstraffic == 1) {
	  routing_platf->getRouteAndLatency(dst, src, &back_route, NULL);
    xbt_dynar_foreach(back_route, i, _link) {
      link = static_cast<NetworkCm02Link*>(_link);
      if (link->isOff()) {
        failed = 1;
        break;
      }
    }
  }

  action = new NetworkCm02Action(this, size, failed);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  action->m_latencyLimited = 0;
#endif
  action->m_weight = action->m_latency = latency;

  action->m_rate = rate;
  if (p_updateMechanism == UM_LAZY) {
    action->m_indexHeap = -1;
    action->m_lastUpdate = surf_get_clock();
  }

  bandwidth_bound = -1.0;
  if (sg_weight_S_parameter > 0) {
    xbt_dynar_foreach(route, i, _link) {
      link = static_cast<NetworkCm02Link*>(_link);
      action->m_weight += sg_weight_S_parameter / link->getBandwidth();
    }
  }
  xbt_dynar_foreach(route, i, _link) {
	link = static_cast<NetworkCm02Link*>(_link);
    double bb = bandwidthFactor(size) * link->getBandwidth();
    bandwidth_bound =
        (bandwidth_bound < 0.0) ? bb : std::min(bandwidth_bound, bb);
  }

  action->m_latCurrent = action->m_latency;
  action->m_latency *= latencyFactor(size);
  action->m_rate = bandwidthConstraint(action->m_rate, bandwidth_bound, size);
  if (m_haveGap) {
    xbt_assert(!xbt_dynar_is_empty(route),
               "Using a model with a gap (e.g., SMPI) with a platform without links (e.g. vivaldi)!!!");

    link = *static_cast<NetworkCm02Link **>(xbt_dynar_get_ptr(route, 0));
    gapAppend(size, link, action);
    XBT_DEBUG("Comm %p: %s -> %s gap=%f (lat=%f)",
              action, src->getName(), dst->getName(), action->m_senderGap,
              action->m_latency);
  }

  constraints_per_variable = xbt_dynar_length(route);
  if (back_route != NULL)
    constraints_per_variable += xbt_dynar_length(back_route);

  if (action->m_latency > 0) {
    action->p_variable = lmm_variable_new(p_maxminSystem, action, 0.0, -1.0,
                         constraints_per_variable);
    if (p_updateMechanism == UM_LAZY) {
      // add to the heap the event when the latency is payed
      XBT_DEBUG("Added action (%p) one latency event at date %f", action,
                action->m_latency + action->m_lastUpdate);
      action->heapInsert(p_actionHeap, action->m_latency + action->m_lastUpdate, xbt_dynar_is_empty(route) ? NORMAL : LATENCY);
    }
  } else
    action->p_variable = lmm_variable_new(p_maxminSystem, action, 1.0, -1.0, constraints_per_variable);

  if (action->m_rate < 0) {
    lmm_update_variable_bound(p_maxminSystem, action->getVariable(), (action->m_latCurrent > 0) ? sg_tcp_gamma / (2.0 * action->m_latCurrent) : -1.0);
  } else {
    lmm_update_variable_bound(p_maxminSystem, action->getVariable(), (action->m_latCurrent > 0) ? std::min(action->m_rate, sg_tcp_gamma / (2.0 * action->m_latCurrent)) : action->m_rate);
  }

  xbt_dynar_foreach(route, i, _link) {
	link = static_cast<NetworkCm02Link*>(_link);
    lmm_expand(p_maxminSystem, link->getConstraint(), action->getVariable(), 1.0);
  }

  if (sg_network_crosstraffic == 1) {
    XBT_DEBUG("Fullduplex active adding backward flow using 5%%");
    xbt_dynar_foreach(back_route, i, _link) {
      link = static_cast<NetworkCm02Link*>(_link);
      lmm_expand(p_maxminSystem, link->getConstraint(), action->getVariable(), .05);
    }
  }

  xbt_dynar_free(&route);
  XBT_OUT();

  networkCommunicateCallbacks(action, src, dst, size, rate);
  return action;
}

void NetworkCm02Model::addTraces(){
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  static int called = 0;
  if (called)
    return;
  called = 1;

  /* connect all traces relative to network */
  xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    NetworkCm02Link *link = static_cast<NetworkCm02Link*>( Link::byName(elm) );

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
               trace_name, elm);
    xbt_assert(trace,
               "Cannot connect trace %s to link %s: trace undefined",
               trace_name, elm);

    link->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    NetworkCm02Link *link = static_cast<NetworkCm02Link*>( Link::byName(elm) );

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
               trace_name, elm);
    xbt_assert(trace,
               "Cannot connect trace %s to link %s: trace undefined",
               trace_name, elm);

    link->p_speed.event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    NetworkCm02Link *link = static_cast<NetworkCm02Link*>(Link::byName(elm));;

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
               trace_name, elm);
    xbt_assert(trace,
               "Cannot connect trace %s to link %s: trace undefined",
               trace_name, elm);

    link->p_latEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

/************
 * Resource *
 ************/
NetworkCm02Link::NetworkCm02Link(NetworkCm02Model *model, const char *name, xbt_dict_t props,
	                           lmm_system_t system,
	                           double constraint_value,
	                           tmgr_history_t history,
	                           int initiallyOn,
	                           tmgr_trace_t state_trace,
	                           double metric_peak,
	                           tmgr_trace_t metric_trace,
	                           double lat_initial,
	                           tmgr_trace_t lat_trace,
	                           e_surf_link_sharing_policy_t policy)
: Link(model, name, props, lmm_constraint_new(system, this, constraint_value), history, state_trace)
{
  if (initiallyOn)
    turnOn();
  else
    turnOff();

  p_speed.scale = 1.0;
  p_speed.peak = metric_peak;
  if (metric_trace)
    p_speed.event = tmgr_history_add_trace(history, metric_trace, 0.0, 0, this);
  else
    p_speed.event = NULL;

  m_latCurrent = lat_initial;
  if (lat_trace)
	p_latEvent = tmgr_history_add_trace(history, lat_trace, 0.0, 0, this);

  if (policy == SURF_LINK_FATPIPE)
	lmm_constraint_shared(getConstraint());
}



void NetworkCm02Link::updateState(tmgr_trace_event_t event_type,
                                      double value, double date)
{
  /*   printf("[" "%g" "] Asking to update network card \"%s\" with value " */
  /*     "%g" " for event %p\n", surf_get_clock(), nw_link->name, */
  /*     value, event_type); */

  if (event_type == p_speed.event) {
    updateBandwidth(value, date);
    if (tmgr_trace_event_free(event_type))
      p_speed.event = NULL;
  } else if (event_type == p_latEvent) {
    updateLatency(value, date);
    if (tmgr_trace_event_free(event_type))
      p_latEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      turnOn();
    else {
      lmm_constraint_t cnst = getConstraint();
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;

      turnOff();
      while ((var = lmm_get_var_from_cnst(getModel()->getMaxminSystem(), cnst, &elem))) {
        Action *action = static_cast<Action*>( lmm_variable_id(var) );

        if (action->getState() == SURF_ACTION_RUNNING ||
            action->getState() == SURF_ACTION_READY) {
          action->setFinishTime(date);
          action->setState(SURF_ACTION_FAILED);
        }
      }
    }
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }

  XBT_DEBUG
      ("There were a resource state event, need to update actions related to the constraint (%p)",
       getConstraint());
  return;
}

void NetworkCm02Link::updateBandwidth(double value, double date){
  double delta = sg_weight_S_parameter / value - sg_weight_S_parameter /
                 (p_speed.peak * p_speed.scale);
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;
  lmm_element_t nextelem = NULL;
  int numelem = 0;

  NetworkCm02Action *action = NULL;

  p_speed.peak = value;
  lmm_update_constraint_bound(getModel()->getMaxminSystem(),
                              getConstraint(),
                              sg_bandwidth_factor *
                              (p_speed.peak * p_speed.scale));
  TRACE_surf_link_set_bandwidth(date, getName(), sg_bandwidth_factor * p_speed.peak * p_speed.scale);
  if (sg_weight_S_parameter > 0) {
    while ((var = lmm_get_var_from_cnst_safe(getModel()->getMaxminSystem(), getConstraint(), &elem, &nextelem, &numelem))) {
      action = (NetworkCm02Action*) lmm_variable_id(var);
      action->m_weight += delta;
      if (!action->isSuspended())
        lmm_update_variable_weight(getModel()->getMaxminSystem(), action->getVariable(), action->m_weight);
    }
  }
}

void NetworkCm02Link::updateLatency(double value, double date){
  double delta = value - m_latCurrent;
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;
  lmm_element_t nextelem = NULL;
  int numelem = 0;
  NetworkCm02Action *action = NULL;

  m_latCurrent = value;
  while ((var = lmm_get_var_from_cnst_safe(getModel()->getMaxminSystem(), getConstraint(), &elem, &nextelem, &numelem))) {
    action = (NetworkCm02Action*) lmm_variable_id(var);
    action->m_latCurrent += delta;
    action->m_weight += delta;
    if (action->m_rate < 0)
      lmm_update_variable_bound(getModel()->getMaxminSystem(), action->getVariable(), sg_tcp_gamma / (2.0 * action->m_latCurrent));
    else {
      lmm_update_variable_bound(getModel()->getMaxminSystem(), action->getVariable(),
                                std::min(action->m_rate, sg_tcp_gamma / (2.0 * action->m_latCurrent)));

      if (action->m_rate < sg_tcp_gamma / (2.0 * action->m_latCurrent)) {
        XBT_INFO("Flow is limited BYBANDWIDTH");
      } else {
        XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f",
                 action->m_latCurrent);
      }
    }
    if (!action->isSuspended())
      lmm_update_variable_weight(getModel()->getMaxminSystem(), action->getVariable(), action->m_weight);
  }
}

/**********
 * Action *
 **********/
void NetworkCm02Action::updateRemainingLazy(double now)
{
  double delta = 0.0;

  if (m_suspended != 0)
    return;

  delta = now - m_lastUpdate;

  if (m_remains > 0) {
    XBT_DEBUG("Updating action(%p): remains was %f, last_update was: %f", this, m_remains, m_lastUpdate);
    double_update(&(m_remains), m_lastValue * delta, sg_maxmin_precision*sg_surf_precision);

    XBT_DEBUG("Updating action(%p): remains is now %f", this, m_remains);
  }

  if (m_maxDuration != NO_MAX_DURATION)
    double_update(&m_maxDuration, delta, sg_surf_precision);

  if (m_remains <= 0 &&
      (lmm_get_variable_weight(getVariable()) > 0)) {
    finish();
    setState(SURF_ACTION_DONE);

    heapRemove(getModel()->getActionHeap());
  } else if (((m_maxDuration != NO_MAX_DURATION)
      && (m_maxDuration <= 0))) {
    finish();
    setState(SURF_ACTION_DONE);
    heapRemove(getModel()->getActionHeap());
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(getVariable());
}

}
}
