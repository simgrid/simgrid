/* Copyright (c) 2007-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <algorithm>

#include "ptask_L07.hpp"

#include "cpu_interface.hpp"
#include "surf_routing.hpp"
#include "xbt/lib.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);
XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/
void surf_host_model_init_ptask_L07(void)
{
  XBT_CINFO(xbt_cfg,"Switching to the L07 model to handle parallel tasks.");
  xbt_assert(!surf_cpu_model_pm, "CPU model type already defined");
  xbt_assert(!surf_network_model, "network model type already defined");

  // Define the callbacks to parse the XML
  simgrid::surf::on_link.connect(netlink_parse_init);

  surf_host_model = new simgrid::surf::HostL07Model();
  xbt_dynar_push(all_existing_models, &surf_host_model);
}


namespace simgrid {
namespace surf {

HostL07Model::HostL07Model() : HostModel() {
  maxminSystem_ = lmm_system_new(1);
  surf_network_model = new NetworkL07Model(this,maxminSystem_);
  surf_cpu_model_pm = new CpuL07Model(this,maxminSystem_);

  routing_model_create(surf_network_model->createLink("__loopback__", 498000000, 0.000015, SURF_LINK_FATPIPE, NULL));
}

HostL07Model::~HostL07Model() {
  delete surf_cpu_model_pm;
  delete surf_network_model;
}

CpuL07Model::CpuL07Model(HostL07Model *hmodel,lmm_system_t sys)
  : CpuModel()
  , p_hostModel(hmodel)
  {
    maxminSystem_ = sys;
  }
CpuL07Model::~CpuL07Model() {
  surf_cpu_model_pm = NULL;
  lmm_system_free(maxminSystem_);
  maxminSystem_ = NULL;
}
NetworkL07Model::NetworkL07Model(HostL07Model *hmodel, lmm_system_t sys)
  : NetworkModel()
  , p_hostModel(hmodel)
  {
    maxminSystem_ = sys;
  }
NetworkL07Model::~NetworkL07Model()
{
  surf_network_model = NULL;
  maxminSystem_ = NULL; // Avoid multi-free
}


double HostL07Model::next_occuring_event(double /*now*/)
{
  L07Action *action;

  ActionList *running_actions = getRunningActionSet();
  double min = this->shareResourcesMaxMin(running_actions,
                                              maxminSystem_,
                                              bottleneck_solve);

  for(ActionList::iterator it(running_actions->begin()), itend(running_actions->end())
   ; it != itend ; ++it) {
  action = static_cast<L07Action*>(&*it);
    if (action->m_latency > 0) {
      if (min < 0) {
        min = action->m_latency;
        XBT_DEBUG("Updating min (value) with %p (start %f): %f", action,
               action->getStartTime(), min);
      } else if (action->m_latency < min) {
        min = action->m_latency;
        XBT_DEBUG("Updating min (latency) with %p (start %f): %f", action,
               action->getStartTime(), min);
      }
    }
  }

  XBT_DEBUG("min value : %f", min);

  return min;
}

void HostL07Model::updateActionsState(double /*now*/, double delta) {

  L07Action *action;
  ActionList *actionSet = getRunningActionSet();

  for(ActionList::iterator it = actionSet->begin(), itNext = it
   ; it != actionSet->end()
   ; it =  itNext) {
  ++itNext;
    action = static_cast<L07Action*>(&*it);
    if (action->m_latency > 0) {
      if (action->m_latency > delta) {
        double_update(&(action->m_latency), delta, sg_surf_precision);
      } else {
        action->m_latency = 0.0;
      }
      if ((action->m_latency == 0.0) && (action->isSuspended() == 0)) {
        action->updateBound();
        lmm_update_variable_weight(maxminSystem_, action->getVariable(), 1.0);
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.",
           action, action->getRemains(), lmm_variable_getvalue(action->getVariable()) * delta);
    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);

    if (action->getMaxDuration() != NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).", action, action->getRemains());

    /* In the next if cascade, the action can be finished either because:
     *  - The amount of remaining work reached 0
     *  - The max duration was reached
     * If it's not done, it may have failed.
     */

    if ((action->getRemains() <= 0) &&
        (lmm_get_variable_weight(action->getVariable()) > 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    } else if ((action->getMaxDuration() != NO_MAX_DURATION) &&
               (action->getMaxDuration() <= 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    } else {
      /* Need to check that none of the model has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;

      while ((cnst = lmm_get_cnst_from_var(maxminSystem_, action->getVariable(), i++))) {
        void *constraint_id = lmm_constraint_id(cnst);

        if (static_cast<HostImpl*>(constraint_id)->isOff()) {
          XBT_DEBUG("Action (%p) Failed!!", action);
          action->finish();
          action->setState(SURF_ACTION_FAILED);
          break;
        }
      }
    }
  }
  return;
}

Action *HostL07Model::executeParallelTask(int host_nb, sg_host_t *host_list,
      double *flops_amount, double *bytes_amount,
      double rate) {
  return new L07Action(this, host_nb, host_list, flops_amount, bytes_amount, rate);
}


L07Action::L07Action(Model *model, int host_nb, sg_host_t*host_list,
    double *flops_amount, double *bytes_amount, double rate)
  : CpuAction(model, 1, 0)
{
  int nb_link = 0;
  int nb_used_host = 0; /* Only the hosts with something to compute (>0 flops) are counted) */
  double latency = 0.0;

  this->p_netcardList->reserve(host_nb);
  for (int i = 0; i<host_nb; i++)
    this->p_netcardList->push_back(host_list[i]->pimpl_netcard);

  /* Compute the number of affected resources... */
  if(bytes_amount != NULL) {
    xbt_dict_t ptask_parallel_task_link_set = xbt_dict_new_homogeneous(NULL);

    for (int i = 0; i < host_nb; i++) {
      for (int j = 0; j < host_nb; j++) {

        if (bytes_amount[i * host_nb + j] > 0) {
          double lat=0.0;
          std::vector<Link*> *route = new std::vector<Link*>();

          routing_platf->getRouteAndLatency((*p_netcardList)[i], (*p_netcardList)[j], route, &lat);
          latency = MAX(latency, lat);

          for (auto link : *route)
            xbt_dict_set(ptask_parallel_task_link_set, link->getName(), link, NULL);
          delete route;
        }
      }
    }

    nb_link = xbt_dict_length(ptask_parallel_task_link_set);
    xbt_dict_free(&ptask_parallel_task_link_set);
  }

  for (int i = 0; i < host_nb; i++)
    if (flops_amount[i] > 0)
      nb_used_host++;

  XBT_DEBUG("Creating a parallel task (%p) with %d hosts and %d unique links.", this, host_nb, nb_link);
  this->p_computationAmount = flops_amount;
  this->p_communicationAmount = bytes_amount;
  this->m_latency = latency;
  this->m_rate = rate;

  this->p_variable = lmm_variable_new(model->getMaxminSystem(), this, 1.0,
      (rate > 0 ? rate : -1.0),
      host_nb + nb_link);

  if (this->m_latency > 0)
    lmm_update_variable_weight(model->getMaxminSystem(), this->getVariable(), 0.0);

  for (int i = 0; i < host_nb; i++)
    lmm_expand(model->getMaxminSystem(), host_list[i]->pimpl_cpu->getConstraint(),
        this->getVariable(), flops_amount[i]);

  if(bytes_amount != NULL) {
    for (int i = 0; i < host_nb; i++) {
      for (int j = 0; j < host_nb; j++) {

        if (bytes_amount[i * host_nb + j] == 0.0)
          continue;
        std::vector<Link*> *route = new std::vector<Link*>();

        routing_platf->getRouteAndLatency((*p_netcardList)[i], (*p_netcardList)[j], route, NULL);

        for (auto link : *route)
          lmm_expand_add(model->getMaxminSystem(), link->getConstraint(), this->getVariable(), bytes_amount[i * host_nb + j]);

        delete route;
      }
    }
  }

  if (nb_link + nb_used_host == 0) {
    this->setCost(1.0);
    this->setRemains(0.0);
  }
  xbt_free(host_list);
}

Action *NetworkL07Model::communicate(NetCard *src, NetCard *dst,
                                       double size, double rate)
{
  sg_host_t*host_list = xbt_new0(sg_host_t, 2);
  double *flops_amount = xbt_new0(double, 2);
  double *bytes_amount = xbt_new0(double, 4);
  Action *res = NULL;

  host_list[0] = sg_host_by_name(src->name());
  host_list[1] = sg_host_by_name(dst->name());
  bytes_amount[1] = size;

  res = p_hostModel->executeParallelTask(2, host_list, flops_amount, bytes_amount, rate);

  return res;
}

Cpu *CpuL07Model::createCpu(simgrid::s4u::Host *host,  xbt_dynar_t powerPeakList,
    tmgr_trace_t power_trace, int core, tmgr_trace_t state_trace)
{
  CpuL07 *cpu = new CpuL07(this, host, powerPeakList, power_trace, core, state_trace);
  return cpu;
}

Link* NetworkL07Model::createLink(const char *name, double bandwidth, double latency,
    e_surf_link_sharing_policy_t policy, xbt_dict_t properties)
{
  xbt_assert(!Link::byName(name),
           "Link '%s' declared several times in the platform.", name);

  Link* link = new LinkL07(this, name, properties, bandwidth, latency, policy);
  Link::onCreation(link);
  return link;
}

/************
 * Resource *
 ************/

CpuL07::CpuL07(CpuL07Model *model, simgrid::s4u::Host *host,
    xbt_dynar_t speedPeakList,
    tmgr_trace_t speedTrace,
    int core, tmgr_trace_t state_trace)
 : Cpu(model, host, speedPeakList, core, xbt_dynar_get_as(speedPeakList,0,double))
{
  p_constraint = lmm_constraint_new(model->getMaxminSystem(), this, xbt_dynar_get_as(speedPeakList,0,double));

  if (speedTrace)
    speed_.event = future_evt_set->add_trace(speedTrace, 0.0, this);

  if (state_trace)
    stateEvent_ = future_evt_set->add_trace(state_trace, 0.0, this);
}

CpuL07::~CpuL07()
{
}

LinkL07::LinkL07(NetworkL07Model *model, const char* name, xbt_dict_t props, double bandwidth, double latency,
             e_surf_link_sharing_policy_t policy)
 : Link(model, name, props, lmm_constraint_new(model->getMaxminSystem(), this, bandwidth))
{
  m_bandwidth.peak = bandwidth;
  m_latency.peak = latency;

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(getConstraint());
}

Action *CpuL07::execution_start(double size)
{
  sg_host_t*host_list = xbt_new0(sg_host_t, 1);
  double *flops_amount = xbt_new0(double, 1);

  host_list[0] = getHost();
  flops_amount[0] = size;

  return static_cast<CpuL07Model*>(getModel())->p_hostModel
    ->executeParallelTask( 1, host_list, flops_amount, NULL, -1);
}

Action *CpuL07::sleep(double duration)
{
  L07Action *action = static_cast<L07Action*>(execution_start(1.0));
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  lmm_update_variable_weight(getModel()->getMaxminSystem(), action->getVariable(), 0.0);

  return action;
}

bool CpuL07::isUsed(){
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuL07::onSpeedChange() {
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;

    lmm_update_constraint_bound(getModel()->getMaxminSystem(), getConstraint(), speed_.peak * speed_.scale);
    while ((var = lmm_get_var_from_cnst
            (getModel()->getMaxminSystem(), getConstraint(), &elem))) {
      Action *action = static_cast<Action*>(lmm_variable_id(var));

      lmm_update_variable_bound(getModel()->getMaxminSystem(),
                                action->getVariable(),
                                speed_.scale * speed_.peak);
    }

  Cpu::onSpeedChange();
}


bool LinkL07::isUsed(){
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

void CpuL07::apply_event(tmgr_trace_iterator_t triggered, double value){
  XBT_DEBUG("Updating cpu %s (%p) with value %g", getName(), this, value);
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
  XBT_DEBUG("Updating link %s (%p) with value=%f", getName(), this, value);
  if (triggered == m_bandwidth.event) {
    updateBandwidth(value);
    tmgr_trace_event_unref(&m_bandwidth.event);

  } else if (triggered == m_latency.event) {
    updateLatency(value);
    tmgr_trace_event_unref(&m_latency.event);

  } else if (triggered == m_stateEvent) {
    if (value > 0)
      turnOn();
    else
      turnOff();
    tmgr_trace_event_unref(&m_stateEvent);

  } else {
    xbt_die("Unknown event ! \n");
  }
}

void LinkL07::updateBandwidth(double value)
{
  m_bandwidth.peak = value;
  lmm_update_constraint_bound(getModel()->getMaxminSystem(), getConstraint(), m_bandwidth.peak * m_bandwidth.scale);
}

void LinkL07::updateLatency(double value)
{
  lmm_variable_t var = NULL;
  L07Action *action;
  lmm_element_t elem = NULL;

  m_latency.peak = value;
  while ((var = lmm_get_var_from_cnst(getModel()->getMaxminSystem(), getConstraint(), &elem))) {
    action = static_cast<L07Action*>(lmm_variable_id(var));
    action->updateBound();
  }
}

/**********
 * Action *
 **********/

L07Action::~L07Action(){
  delete p_netcardList;
  free(p_communicationAmount);
  free(p_computationAmount);
}

void L07Action::updateBound()
{
  double lat_current = 0.0;
  double lat_bound = -1.0;
  int i, j;

  int hostNb = p_netcardList->size();

  if (p_communicationAmount != NULL) {
    for (i = 0; i < hostNb; i++) {
      for (j = 0; j < hostNb; j++) {

        if (p_communicationAmount[i * hostNb + j] > 0) {
          double lat = 0.0;
          std::vector<Link*> *route = new std::vector<Link*>();
          routing_platf->getRouteAndLatency((*p_netcardList)[i], (*p_netcardList)[j], route, &lat);

          lat_current = MAX(lat_current, lat * p_communicationAmount[i * hostNb + j]);
          delete route;
        }
      }
    }
  }
  lat_bound = sg_tcp_gamma / (2.0 * lat_current);
  XBT_DEBUG("action (%p) : lat_bound = %g", this, lat_bound);
  if ((m_latency == 0.0) && (m_suspended == 0)) {
    if (m_rate < 0)
      lmm_update_variable_bound(getModel()->getMaxminSystem(), getVariable(), lat_bound);
    else
      lmm_update_variable_bound(getModel()->getMaxminSystem(), getVariable(),
        std::min(m_rate, lat_bound));
  }
}

int L07Action::unref()
{
  m_refcount--;
  if (!m_refcount) {
    if (action_hook.is_linked())
      p_stateSet->erase(p_stateSet->iterator_to(*this));
    if (getVariable())
      lmm_variable_free(getModel()->getMaxminSystem(), getVariable());
    delete this;
    return 1;
  }
  return 0;
}

}
}
