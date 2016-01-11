/* Copyright (c) 2007-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <algorithm>

#include "host_ptask_L07.hpp"

#include "cpu_interface.hpp"
#include "surf_routing.hpp"
#include "xbt/lib.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/

static void ptask_netlink_parse_init(sg_platf_link_cbarg_t link)
{
  netlink_parse_init(link);
  current_property_set = NULL;
}

void surf_host_model_init_ptask_L07(void)
{
  XBT_INFO("Switching to the L07 model to handle parallel tasks.");
  xbt_assert(!surf_cpu_model_pm, "CPU model type already defined");
  xbt_assert(!surf_network_model, "network model type already defined");

  // Define the callbacks to parse the XML
  sg_platf_link_add_cb(ptask_netlink_parse_init);
  sg_platf_postparse_add_cb(host_add_traces);

  surf_host_model = new simgrid::surf::HostL07Model();
  xbt_dynar_push(all_existing_models, &surf_host_model);
}


namespace simgrid {
namespace surf {

HostL07Model::HostL07Model() : HostModel() {
  p_maxminSystem = lmm_system_new(1);
  surf_network_model = new NetworkL07Model(this,p_maxminSystem);
  surf_cpu_model_pm = new CpuL07Model(this,p_maxminSystem);

  routing_model_create(surf_network_model->createLink("__loopback__",
	                                                  498000000, NULL,
	                                                  0.000015, NULL,
	                                                  1/*ON*/, NULL,
	                                                  SURF_LINK_FATPIPE, NULL));
}

HostL07Model::~HostL07Model() {
  delete surf_cpu_model_pm;
  delete surf_network_model;
}

CpuL07Model::CpuL07Model(HostL07Model *hmodel,lmm_system_t sys)
	: CpuModel()
	, p_hostModel(hmodel)
	{
	  p_maxminSystem = sys;
	}
CpuL07Model::~CpuL07Model() {
	surf_cpu_model_pm = NULL;
	p_maxminSystem = NULL; // Avoid multi-free
}
NetworkL07Model::NetworkL07Model(HostL07Model *hmodel, lmm_system_t sys)
	: NetworkModel()
	, p_hostModel(hmodel)
	{
	  p_maxminSystem = sys;
	}
NetworkL07Model::~NetworkL07Model()
{
	surf_network_model = NULL;
	p_maxminSystem = NULL; // Avoid multi-free
}


double HostL07Model::shareResources(double /*now*/)
{
  L07Action *action;

  ActionList *running_actions = getRunningActionSet();
  double min = this->shareResourcesMaxMin(running_actions,
                                              p_maxminSystem,
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
        lmm_update_variable_weight(p_maxminSystem, action->getVariable(), 1.0);
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

      while ((cnst = lmm_get_cnst_from_var(p_maxminSystem, action->getVariable(), i++))) {
        void *constraint_id = lmm_constraint_id(cnst);

        if (static_cast<Host*>(constraint_id)->isOff()) {
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


L07Action::L07Action(Model *model, int host_nb,
		sg_host_t*host_list,
		double *flops_amount,
		double *bytes_amount,
		double rate)
	: CpuAction(model, 1, 0)
{
  unsigned int cpt;
  int nb_link = 0;
  int nb_used_host = 0; /* Only the hosts with something to compute (>0 flops) are counted) */
  double latency = 0.0;

  xbt_dict_t ptask_parallel_task_link_set = xbt_dict_new_homogeneous(NULL);

  this->p_netcardList->reserve(host_nb);
  for (int i = 0; i<host_nb; i++)
	  this->p_netcardList->push_back(host_list[i]->pimpl_netcard);

  /* Compute the number of affected resources... */
  for (int i = 0; i < host_nb; i++) {
    for (int j = 0; j < host_nb; j++) {
      xbt_dynar_t route=NULL;

      if (bytes_amount[i * host_nb + j] > 0) {
        double lat=0.0;
        unsigned int cpt;
        void *_link;
        LinkL07 *link;

        routing_platf->getRouteAndLatency((*this->p_netcardList)[i], (*this->p_netcardList)[j],
        		                          &route, &lat);
        latency = MAX(latency, lat);

        xbt_dynar_foreach(route, cpt, _link) {
           link = static_cast<LinkL07*>(_link);
           xbt_dict_set(ptask_parallel_task_link_set, link->getName(), link, NULL);
        }
      }
    }
  }

  nb_link = xbt_dict_length(ptask_parallel_task_link_set);
  xbt_dict_free(&ptask_parallel_task_link_set);

  for (int i = 0; i < host_nb; i++)
    if (flops_amount[i] > 0)
      nb_used_host++;

  XBT_DEBUG("Creating a parallel task (%p) with %d cpus and %d links.",
         this, host_nb, nb_link);
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
    lmm_expand(model->getMaxminSystem(),
    	       host_list[i]->pimpl_cpu->getConstraint(),
               this->getVariable(), flops_amount[i]);

  for (int i = 0; i < host_nb; i++) {
    for (int j = 0; j < host_nb; j++) {
      void *_link;

      xbt_dynar_t route=NULL;
      if (bytes_amount[i * host_nb + j] == 0.0)
        continue;

      routing_platf->getRouteAndLatency((*this->p_netcardList)[i], (*this->p_netcardList)[j],
    		                            &route, NULL);

      xbt_dynar_foreach(route, cpt, _link) {
        LinkL07 *link = static_cast<LinkL07*>(_link);
        lmm_expand_add(model->getMaxminSystem(), link->getConstraint(),
                       this->getVariable(),
                       bytes_amount[i * host_nb + j]);
      }
    }
  }

  if (nb_link + nb_used_host == 0) {
    this->setCost(1.0);
    this->setRemains(0.0);
  }
}

Action *NetworkL07Model::communicate(NetCard *src, NetCard *dst,
                                       double size, double rate)
{
  sg_host_t*host_list = xbt_new0(sg_host_t, 2);
  double *flops_amount = xbt_new0(double, 2);
  double *bytes_amount = xbt_new0(double, 4);
  Action *res = NULL;

  host_list[0] = sg_host_by_name(src->getName());
  host_list[1] = sg_host_by_name(dst->getName());
  bytes_amount[1] = size;

  res = p_hostModel->executeParallelTask(2, host_list,
                                    flops_amount,
                                    bytes_amount, rate);

  return res;
}

Cpu *CpuL07Model::createCpu(simgrid::Host *host,  xbt_dynar_t powerPeakList,
                          int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          int initiallyOn,
                          tmgr_trace_t state_trace)
{
  CpuL07 *cpu = new CpuL07(this, host, powerPeakList, pstate, power_scale, power_trace,
                         core, initiallyOn, state_trace);
  return cpu;
}

Link* NetworkL07Model::createLink(const char *name,
                                 double bw_initial,
                                 tmgr_trace_t bw_trace,
                                 double lat_initial,
                                 tmgr_trace_t lat_trace,
                                 int initiallyOn,
                                 tmgr_trace_t state_trace,
                                 e_surf_link_sharing_policy_t policy,
                                 xbt_dict_t properties)
{
  xbt_assert(!Link::byName(name),
	         "Link '%s' declared several times in the platform file.", name);

  Link* link = new LinkL07(this, name, properties,
		             bw_initial, bw_trace,
					 lat_initial, lat_trace,
					 initiallyOn, state_trace,
					 policy);
  Link::onCreation(link);
  return link;
}

void HostL07Model::addTraces()
{
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  if (!trace_connect_list_host_avail)
    return;

  /* Connect traces relative to cpu */
  xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuL07 *host = static_cast<CpuL07*>(sg_host_by_name(elm)->pimpl_cpu);

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuL07 *host = static_cast<CpuL07*>(sg_host_by_name(elm)->pimpl_cpu);

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_speedEvent = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  /* Connect traces relative to network */
  xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07 *link = static_cast<LinkL07*>(Link::byName(elm));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07 *link = static_cast<LinkL07*>(Link::byName(elm));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_bwEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07 *link = static_cast<LinkL07*>(Link::byName(elm));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_latEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

/************
 * Resource *
 ************/

CpuL07::CpuL07(CpuL07Model *model, simgrid::Host *host,
	             xbt_dynar_t speedPeakList, int pstate,
				 double speedScale, tmgr_trace_t speedTrace,
		         int core, int initiallyOn, tmgr_trace_t state_trace)
 : Cpu(model, host, speedPeakList, pstate,
	   core, xbt_dynar_get_as(speedPeakList,pstate,double), speedScale, initiallyOn)
{
  p_constraint = lmm_constraint_new(model->getMaxminSystem(), this, xbt_dynar_get_as(speedPeakList,pstate,double) * speedScale);

  if (speedTrace)
    p_speedEvent = tmgr_history_add_trace(history, speedTrace, 0.0, 0, this);
  else
    p_speedEvent = NULL;

  if (state_trace)
	p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, this);
}

CpuL07::~CpuL07()
{
}

LinkL07::LinkL07(NetworkL07Model *model, const char* name, xbt_dict_t props,
		         double bw_initial,
		         tmgr_trace_t bw_trace,
		         double lat_initial,
		         tmgr_trace_t lat_trace,
		         int initiallyOn,
		         tmgr_trace_t state_trace,
		         e_surf_link_sharing_policy_t policy)
 : Link(model, name, props, lmm_constraint_new(model->getMaxminSystem(), this, bw_initial), history, state_trace)
{
  m_bwCurrent = bw_initial;
  if (bw_trace)
    p_bwEvent = tmgr_history_add_trace(history, bw_trace, 0.0, 0, this);

  if (initiallyOn)
    turnOn();
  else
    turnOff();
  m_latCurrent = lat_initial;

  if (lat_trace)
	p_latEvent = tmgr_history_add_trace(history, lat_trace, 0.0, 0, this);

  if (policy == SURF_LINK_FATPIPE)
	lmm_constraint_shared(getConstraint());
}

Action *CpuL07::execute(double size)
{
  sg_host_t*host_list = xbt_new0(sg_host_t, 1);
  double *flops_amount = xbt_new0(double, 1);
  double *bytes_amount = xbt_new0(double, 1);

  host_list[0] = getHost();
  flops_amount[0] = size;

  return static_cast<CpuL07Model*>(getModel())
    ->p_hostModel
    ->executeParallelTask( 1, host_list, flops_amount, bytes_amount, -1);
}

Action *CpuL07::sleep(double duration)
{
  L07Action *action = NULL;

  XBT_IN("(%s,%g)", getName(), duration);

  action = static_cast<L07Action*>(execute(1.0));
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  lmm_update_variable_weight(getModel()->getMaxminSystem(), action->getVariable(), 0.0);

  XBT_OUT();
  return action;
}

bool CpuL07::isUsed(){
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuL07::onSpeedChange() {
	lmm_variable_t var = NULL;
	lmm_element_t elem = NULL;

    lmm_update_constraint_bound(getModel()->getMaxminSystem(), getConstraint(), m_speedPeak * m_speedScale);
    while ((var = lmm_get_var_from_cnst
            (getModel()->getMaxminSystem(), getConstraint(), &elem))) {
      Action *action = static_cast<Action*>(lmm_variable_id(var));

      lmm_update_variable_bound(getModel()->getMaxminSystem(),
                                action->getVariable(),
                                m_speedScale * m_speedPeak);
    }

	Cpu::onSpeedChange();
}


bool LinkL07::isUsed(){
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

void CpuL07::updateState(tmgr_trace_event_t event_type, double value, double /*date*/){
  XBT_DEBUG("Updating cpu %s (%p) with value %g", getName(), this, value);
  if (event_type == p_speedEvent) {
	m_speedScale = value;
	onSpeedChange();
    if (tmgr_trace_event_free(event_type))
      p_speedEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      turnOn();
    else
      turnOff();
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }
  return;
}

void LinkL07::updateState(tmgr_trace_event_t event_type, double value, double date) {
  XBT_DEBUG("Updating link %s (%p) with value=%f for date=%g", getName(), this, value, date);
  if (event_type == p_bwEvent) {
    updateBandwidth(value, date);
    if (tmgr_trace_event_free(event_type))
      p_bwEvent = NULL;
  } else if (event_type == p_latEvent) {
    updateLatency(value, date);
    if (tmgr_trace_event_free(event_type))
      p_latEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      turnOn();
    else
      turnOff();
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }
  return;
}

double LinkL07::getBandwidth()
{
  return m_bwCurrent;
}

void LinkL07::updateBandwidth(double value, double date)
{
  m_bwCurrent = value;
  lmm_update_constraint_bound(getModel()->getMaxminSystem(), getConstraint(), m_bwCurrent);
}

void LinkL07::updateLatency(double value, double date)
{
  lmm_variable_t var = NULL;
  L07Action *action;
  lmm_element_t elem = NULL;

  m_latCurrent = value;
  while ((var = lmm_get_var_from_cnst(getModel()->getMaxminSystem(), getConstraint(), &elem))) {
    action = static_cast<L07Action*>(lmm_variable_id(var));
    action->updateBound();
  }
}

/**********
 * Action *
 **********/

L07Action::~L07Action(){
  free(p_communicationAmount);
  free(p_computationAmount);
}

void L07Action::updateBound()
{
  double lat_current = 0.0;
  double lat_bound = -1.0;
  int i, j;

  int hostNb = p_netcardList->size();

  for (i = 0; i < hostNb; i++) {
    for (j = 0; j < hostNb; j++) {
      xbt_dynar_t route=NULL;

      if (p_communicationAmount[i * hostNb + j] > 0) {
        double lat = 0.0;
        routing_platf->getRouteAndLatency((*p_netcardList)[i], (*p_netcardList)[j],
                				          &route, &lat);

        lat_current = MAX(lat_current, lat * p_communicationAmount[i * hostNb + j]);
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

void L07Action::suspend()
{
  XBT_IN("(%p))", this);
  if (m_suspended != 2) {
    m_suspended = 1;
    lmm_update_variable_weight(getModel()->getMaxminSystem(), getVariable(), 0.0);
  }
  XBT_OUT();
}

void L07Action::resume()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(getModel()->getMaxminSystem(), getVariable(), 1.0);
    m_suspended = 0;
  }
  XBT_OUT();
}

void L07Action::setMaxDuration(double duration)
{                               /* FIXME: should inherit */
  XBT_IN("(%p,%g)", this, duration);
  m_maxDuration = duration;
  XBT_OUT();
}

void L07Action::setPriority(double priority)
{                               /* FIXME: should inherit */
  XBT_IN("(%p,%g)", this, priority);
  m_priority = priority;
  XBT_OUT();
}

double L07Action::getRemains()
{
  XBT_IN("(%p)", this);
  XBT_OUT();
  return m_remains;
}

}
}
