/* Copyright (c) 2007-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "workstation_ptask_L07.hpp"
#include "cpu_interface.hpp"
#include "surf_routing.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_workstation);

static int ptask_host_count = 0;
static xbt_dict_t ptask_parallel_task_link_set = NULL;
lmm_system_t ptask_maxmin_system = NULL;

WorkstationL07Model::WorkstationL07Model() : WorkstationModel("Workstation ptask_L07") {
  if (!ptask_maxmin_system)
	ptask_maxmin_system = lmm_system_new(1);
  surf_workstation_model = NULL;
  surf_network_model = new NetworkL07Model();
  surf_cpu_model_pm = new CpuL07Model();
  routing_model_create(static_cast<ResourcePtr>(surf_network_model->createResource("__loopback__",
	                                                  498000000, NULL,
	                                                  0.000015, NULL,
	                                                  SURF_RESOURCE_ON, NULL,
	                                                  SURF_LINK_FATPIPE, NULL)));
  p_cpuModel = surf_cpu_model_pm;
}

WorkstationL07Model::~WorkstationL07Model() {
  xbt_dict_free(&ptask_parallel_task_link_set);

  delete surf_cpu_model_pm;
  delete surf_network_model;
  ptask_host_count = 0;

  if (ptask_maxmin_system) {
    lmm_system_free(ptask_maxmin_system);
    ptask_maxmin_system = NULL;
  }
}

double WorkstationL07Model::shareResources(double /*now*/)
{
  WorkstationL07ActionPtr action;

  ActionListPtr running_actions = getRunningActionSet();
  double min = this->shareResourcesMaxMin(running_actions,
                                              ptask_maxmin_system,
                                              bottleneck_solve);

  for(ActionList::iterator it(running_actions->begin()), itend(running_actions->end())
	 ; it != itend ; ++it) {
	action = static_cast<WorkstationL07ActionPtr>(&*it);
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

void WorkstationL07Model::updateActionsState(double /*now*/, double delta)
{
  double deltap = 0.0;
  WorkstationL07ActionPtr action;

  ActionListPtr actionSet = getRunningActionSet();

  for(ActionList::iterator it(actionSet->begin()), itNext = it, itend(actionSet->end())
	 ; it != itend ; it=itNext) {
	++itNext;
    action = static_cast<WorkstationL07ActionPtr>(&*it);
    deltap = delta;
    if (action->m_latency > 0) {
      if (action->m_latency > deltap) {
        double_update(&(action->m_latency), deltap, sg_surf_precision);
        deltap = 0.0;
      } else {
        double_update(&(deltap), action->m_latency, sg_surf_precision);
        action->m_latency = 0.0;
      }
      if ((action->m_latency == 0.0) && (action->isSuspended() == 0)) {
        action->updateBound();
        lmm_update_variable_weight(ptask_maxmin_system, action->getVariable(), 1.0);
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.",
           action, action->getRemains(), lmm_variable_getvalue(action->getVariable()) * delta);
    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);

    if (action->getMaxDuration() != NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).",
           action, action->getRemains());
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
      void *constraint_id = NULL;

      while ((cnst = lmm_get_cnst_from_var(ptask_maxmin_system, action->getVariable(),
                                    i++))) {
        constraint_id = lmm_constraint_id(cnst);

        if (static_cast<WorkstationPtr>(constraint_id)->getState() == SURF_RESOURCE_OFF) {
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

ActionPtr WorkstationL07Model::executeParallelTask(int workstation_nb,
                                                   void **workstation_list,
                                                 double
                                                 *computation_amount, double
                                                 *communication_amount,
                                                 double rate)
{
  WorkstationL07ActionPtr action;
  int i, j;
  unsigned int cpt;
  int nb_link = 0;
  int nb_host = 0;
  double latency = 0.0;

  if (ptask_parallel_task_link_set == NULL)
    ptask_parallel_task_link_set = xbt_dict_new_homogeneous(NULL);

  xbt_dict_reset(ptask_parallel_task_link_set);

  /* Compute the number of affected resources... */
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      xbt_dynar_t route=NULL;

      if (communication_amount[i * workstation_nb + j] > 0) {
        double lat=0.0;
        unsigned int cpt;
        void *_link;
        LinkL07Ptr link;

        routing_platf->getRouteAndLatency(static_cast<WorkstationL07Ptr>(workstation_list[i])->p_netElm,
        		                          static_cast<WorkstationL07Ptr>(workstation_list[j])->p_netElm,
        		                          &route,
        		                          &lat);
        latency = MAX(latency, lat);

        xbt_dynar_foreach(route, cpt, _link) {
           link = static_cast<LinkL07Ptr>(_link);
           xbt_dict_set(ptask_parallel_task_link_set, link->getName(), link, NULL);
        }
      }
    }
  }

  nb_link = xbt_dict_length(ptask_parallel_task_link_set);
  xbt_dict_reset(ptask_parallel_task_link_set);

  for (i = 0; i < workstation_nb; i++)
    if (computation_amount[i] > 0)
      nb_host++;

  action = new WorkstationL07Action(this, 1, 0);
  XBT_DEBUG("Creating a parallel task (%p) with %d cpus and %d links.",
         action, workstation_nb, nb_link);
  action->m_suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */
  action->m_workstationNb = workstation_nb;
  action->p_workstationList = (WorkstationPtr *) workstation_list;
  action->p_computationAmount = computation_amount;
  action->p_communicationAmount = communication_amount;
  action->m_latency = latency;
  action->m_rate = rate;

  action->p_variable = lmm_variable_new(ptask_maxmin_system, action, 1.0,
                       (action->m_rate > 0) ? action->m_rate : -1.0,
                       workstation_nb + nb_link);

  if (action->m_latency > 0)
    lmm_update_variable_weight(ptask_maxmin_system, action->getVariable(), 0.0);

  for (i = 0; i < workstation_nb; i++)
    lmm_expand(ptask_maxmin_system,
    	       static_cast<CpuPtr>(static_cast<WorkstationL07Ptr>(workstation_list[i])->p_cpu)->getConstraint(),
               action->getVariable(), computation_amount[i]);

  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      void *_link;
      LinkL07Ptr link;

      xbt_dynar_t route=NULL;
      if (communication_amount[i * workstation_nb + j] == 0.0)
        continue;

      routing_platf->getRouteAndLatency(static_cast<WorkstationL07Ptr>(workstation_list[i])->p_netElm,
                                        static_cast<WorkstationL07Ptr>(workstation_list[j])->p_netElm,
    		                            &route, NULL);

      xbt_dynar_foreach(route, cpt, _link) {
        link = static_cast<LinkL07Ptr>(_link);
        lmm_expand_add(ptask_maxmin_system, link->getConstraint(),
                       action->getVariable(),
                       communication_amount[i * workstation_nb + j]);
      }
    }
  }

  if (nb_link + nb_host == 0) {
    action->setCost(1.0);
    action->setRemains(0.0);
  }

  return static_cast<ActionPtr>(action);
}

ResourcePtr WorkstationL07Model::createResource(const char *name, double /*power_scale*/,
                                                double /*power_initial*/,
                                                tmgr_trace_t /*power_trace*/,
                                                e_surf_resource_state_t /*state_initial*/,
                                                tmgr_trace_t /*state_trace*/,
                                                xbt_dict_t /*cpu_properties*/)
{
  WorkstationL07Ptr wk = NULL;
  xbt_assert(!surf_workstation_resource_priv(surf_workstation_resource_by_name(name)),
              "Host '%s' declared several times in the platform file.",
              name);

  wk = new WorkstationL07(this, name, NULL,
		                  static_cast<RoutingEdgePtr>(xbt_lib_get_or_null(host_lib, name, ROUTING_HOST_LEVEL)),
		                  static_cast<CpuPtr>(xbt_lib_get_or_null(host_lib, name, SURF_CPU_LEVEL)));

  xbt_lib_set(host_lib, name, SURF_WKS_LEVEL, static_cast<ResourcePtr>(wk));

  return wk;//FIXME:xbt_lib_get_elm_or_null(host_lib, name);
}

ActionPtr WorkstationL07Model::communicate(WorkstationPtr src, WorkstationPtr dst,
                                       double size, double rate)
{
  void **workstation_list = xbt_new0(void *, 2);
  double *computation_amount = xbt_new0(double, 2);
  double *communication_amount = xbt_new0(double, 4);
  ActionPtr res = NULL;

  workstation_list[0] = static_cast<ResourcePtr>(src);
  workstation_list[1] = static_cast<ResourcePtr>(dst);
  communication_amount[1] = size;

  res = executeParallelTask(2, workstation_list,
                                    computation_amount,
                                    communication_amount, rate);

  return res;
}

xbt_dynar_t WorkstationL07Model::getRoute(WorkstationPtr src, WorkstationPtr dst)
{
  xbt_dynar_t route=NULL;
  routing_platf->getRouteAndLatency(src->p_netElm, dst->p_netElm, &route, NULL);
  return route;
}

ResourcePtr CpuL07Model::createResource(const char *name, double power_scale,
                               double power_initial,
                               tmgr_trace_t power_trace,
                               e_surf_resource_state_t state_initial,
                               tmgr_trace_t state_trace,
                               xbt_dict_t cpu_properties)
{
  xbt_assert(!surf_workstation_resource_priv(surf_workstation_resource_by_name(name)),
              "Host '%s' declared several times in the platform file.",
              name);

  CpuL07Ptr cpu = new CpuL07(this, name, cpu_properties,
		                     power_scale, power_initial, power_trace,state_initial, state_trace);

  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, static_cast<ResourcePtr>(cpu));

  return cpu;//FIXME:xbt_lib_get_elm_or_null(host_lib, name);
}

NetworkLinkPtr NetworkL07Model::createResource(const char *name,
                                 double bw_initial,
                                 tmgr_trace_t bw_trace,
                                 double lat_initial,
                                 tmgr_trace_t lat_trace,
                                 e_surf_resource_state_t
                                 state_initial,
                                 tmgr_trace_t state_trace,
                                 e_surf_link_sharing_policy_t policy,
                                 xbt_dict_t properties)
{
  xbt_assert(!xbt_lib_get_or_null(link_lib, name, SURF_LINK_LEVEL),
	              "Link '%s' declared several times in the platform file.",
	              name);

  LinkL07Ptr nw_link = new LinkL07(this, name, properties,
	                               bw_initial, bw_trace,
	                               lat_initial, lat_trace,
	                               state_initial, state_trace,
	                               policy);

  xbt_lib_set(link_lib, name, SURF_LINK_LEVEL, static_cast<ResourcePtr>(nw_link));
  return nw_link;
}

void WorkstationL07Model::addTraces()
{
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  if (!trace_connect_list_host_avail)
    return;

  /* Connect traces relative to cpu */
  xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuL07Ptr host = static_cast<CpuL07Ptr>(surf_cpu_resource_priv(surf_cpu_resource_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(host));
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuL07Ptr host = static_cast<CpuL07Ptr>(surf_cpu_resource_priv(surf_cpu_resource_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_power.event = tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(host));
  }

  /* Connect traces relative to network */
  xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07Ptr link = static_cast<LinkL07Ptr>(xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(link));
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07Ptr link = static_cast<LinkL07Ptr>(xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_bwEvent = tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(link));
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07Ptr link = static_cast<LinkL07Ptr>(xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_latEvent = tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(link));
  }
}

/************
 * Resource *
 ************/

WorkstationL07::WorkstationL07(WorkstationModelPtr model, const char* name, xbt_dict_t props, RoutingEdgePtr netElm, CpuPtr cpu)
  : Workstation(model, name, props, NULL, netElm, cpu)
{
}

double WorkstationL07::getPowerPeakAt(int /*pstate_index*/)
{
	XBT_DEBUG("[ws_get_power_peak_at] Not implemented for workstation_ptask_L07");
	return 0.0;
}

int WorkstationL07::getNbPstates()
{
	XBT_DEBUG("[ws_get_nb_pstates] Not implemented for workstation_ptask_L07");
	return 0.0;
}

void WorkstationL07::setPowerPeakAt(int /*pstate_index*/)
{
	XBT_DEBUG("[ws_set_power_peak_at] Not implemented for workstation_ptask_L07");
}

double WorkstationL07::getConsumedEnergy()
{
	XBT_DEBUG("[ws_get_consumed_energy] Not implemented for workstation_ptask_L07");
	return 0.0;
}

CpuL07::CpuL07(CpuL07ModelPtr model, const char* name, xbt_dict_t props,
	           double power_scale,
		       double power_initial, tmgr_trace_t power_trace,
		       e_surf_resource_state_t state_initial, tmgr_trace_t state_trace)
 : Cpu(model, name, props, lmm_constraint_new(ptask_maxmin_system, this, power_initial * power_scale),
	   1, 0, 0)
{
  p_power.scale = power_scale;
  xbt_assert(p_power.scale > 0, "Power has to be >0");

  m_powerCurrent = power_initial;
  if (power_trace)
    p_power.event = tmgr_history_add_trace(history, power_trace, 0.0, 0,
                                           static_cast<ResourcePtr>(this));
  else
    p_power.event = NULL;

  setState(state_initial);
  if (state_trace)
	p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, static_cast<ResourcePtr>(this));
}

LinkL07::LinkL07(NetworkL07ModelPtr model, const char* name, xbt_dict_t props,
		         double bw_initial,
		         tmgr_trace_t bw_trace,
		         double lat_initial,
		         tmgr_trace_t lat_trace,
		         e_surf_resource_state_t state_initial,
		         tmgr_trace_t state_trace,
		         e_surf_link_sharing_policy_t policy)
 : NetworkLink(model, name, props, lmm_constraint_new(ptask_maxmin_system, this, bw_initial), history, state_trace)
{
  m_bwCurrent = bw_initial;
  if (bw_trace)
    p_bwEvent = tmgr_history_add_trace(history, bw_trace, 0.0, 0, static_cast<ResourcePtr>(this));

  setState(state_initial);
  m_latCurrent = lat_initial;

  if (lat_trace)
	p_latEvent = tmgr_history_add_trace(history, lat_trace, 0.0, 0, static_cast<ResourcePtr>(this));

  if (policy == SURF_LINK_FATPIPE)
	lmm_constraint_shared(getConstraint());
}

bool CpuL07::isUsed(){
  return lmm_constraint_used(ptask_maxmin_system, getConstraint());
}

bool LinkL07::isUsed(){
  return lmm_constraint_used(ptask_maxmin_system, getConstraint());
}

void CpuL07::updateState(tmgr_trace_event_t event_type, double value, double /*date*/){
  XBT_DEBUG("Updating cpu %s (%p) with value %g", getName(), this, value);
  if (event_type == p_power.event) {
	m_powerCurrent = value;
    lmm_update_constraint_bound(ptask_maxmin_system, getConstraint(), m_powerCurrent * p_power.scale);
    if (tmgr_trace_event_free(event_type))
      p_power.event = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      setState(SURF_RESOURCE_ON);
    else
      setState(SURF_RESOURCE_OFF);
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }
  return;
}

void LinkL07::updateState(tmgr_trace_event_t event_type, double value, double date){
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
      setState(SURF_RESOURCE_ON);
    else
      setState(SURF_RESOURCE_OFF);
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }
  return;
}

e_surf_resource_state_t WorkstationL07::getState()
{
  return p_cpu->getState();
}

double CpuL07::getSpeed(double load)
{
  return load * p_power.scale;
}

double CpuL07::getAvailableSpeed()
{
  return m_powerCurrent;
}

ActionPtr WorkstationL07::execute(double size)
{
  void **workstation_list = xbt_new0(void *, 1);
  double *computation_amount = xbt_new0(double, 1);
  double *communication_amount = xbt_new0(double, 1);

  workstation_list[0] = static_cast<ResourcePtr>(this);
  communication_amount[0] = 0.0;
  computation_amount[0] = size;

  return static_cast<WorkstationL07ModelPtr>(getModel())->executeParallelTask(1, workstation_list,
		                              computation_amount,
                                     communication_amount, -1);
}

ActionPtr WorkstationL07::sleep(double duration)
{
  WorkstationL07ActionPtr action = NULL;

  XBT_IN("(%s,%g)", getName(), duration);

  action = static_cast<WorkstationL07ActionPtr>(execute(1.0));
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  lmm_update_variable_weight(ptask_maxmin_system, action->getVariable(), 0.0);

  XBT_OUT();
  return action;
}

double LinkL07::getBandwidth()
{
  return m_bwCurrent;
}

void LinkL07::updateBandwidth(double value, double date)
{
  m_bwCurrent = value;
  lmm_update_constraint_bound(ptask_maxmin_system, getConstraint(), m_bwCurrent);
}

double LinkL07::getLatency()
{
  return m_latCurrent;
}

void LinkL07::updateLatency(double value, double date)
{
  lmm_variable_t var = NULL;
  WorkstationL07ActionPtr action;
  lmm_element_t elem = NULL;

  m_latCurrent = value;
  while ((var = lmm_get_var_from_cnst(ptask_maxmin_system, getConstraint(), &elem))) {
    action = (WorkstationL07ActionPtr) lmm_variable_id(var);
    action->updateBound();
  }
}


bool LinkL07::isShared()
{
  return lmm_constraint_is_shared(getConstraint());
}

/**********
 * Action *
 **********/

WorkstationL07Action::~WorkstationL07Action(){
  free(p_workstationList);
  free(p_communicationAmount);
  free(p_computationAmount);
}

void WorkstationL07Action::updateBound()
{
  double lat_current = 0.0;
  double lat_bound = -1.0;
  int i, j;

  for (i = 0; i < m_workstationNb; i++) {
    for (j = 0; j < m_workstationNb; j++) {
      xbt_dynar_t route=NULL;

      if (p_communicationAmount[i * m_workstationNb + j] > 0) {
        double lat = 0.0;
        routing_platf->getRouteAndLatency(static_cast<WorkstationL07Ptr>(((void**)p_workstationList)[i])->p_netElm,
                                          static_cast<WorkstationL07Ptr>(((void**)p_workstationList)[j])->p_netElm,
                				          &route, &lat);

        lat_current = MAX(lat_current, lat * p_communicationAmount[i * m_workstationNb + j]);
      }
    }
  }
  lat_bound = sg_tcp_gamma / (2.0 * lat_current);
  XBT_DEBUG("action (%p) : lat_bound = %g", this, lat_bound);
  if ((m_latency == 0.0) && (m_suspended == 0)) {
    if (m_rate < 0)
      lmm_update_variable_bound(ptask_maxmin_system, getVariable(), lat_bound);
    else
      lmm_update_variable_bound(ptask_maxmin_system, getVariable(), min(m_rate, lat_bound));
  }
}

int WorkstationL07Action::unref()
{
  m_refcount--;
  if (!m_refcount) {
    if (actionHook::is_linked())
	  p_stateSet->erase(p_stateSet->iterator_to(*this));
    if (getVariable())
      lmm_variable_free(ptask_maxmin_system, getVariable());
    delete this;
    return 1;
  }
  return 0;
}

void WorkstationL07Action::cancel()
{
  setState(SURF_ACTION_FAILED);
  return;
}

void WorkstationL07Action::suspend()
{
  XBT_IN("(%p))", this);
  if (m_suspended != 2) {
    m_suspended = 1;
    lmm_update_variable_weight(ptask_maxmin_system, getVariable(), 0.0);
  }
  XBT_OUT();
}

void WorkstationL07Action::resume()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(ptask_maxmin_system, getVariable(), 1.0);
    m_suspended = 0;
  }
  XBT_OUT();
}

bool WorkstationL07Action::isSuspended()
{
  return m_suspended == 1;
}

void WorkstationL07Action::setMaxDuration(double duration)
{                               /* FIXME: should inherit */
  XBT_IN("(%p,%g)", this, duration);
  m_maxDuration = duration;
  XBT_OUT();
}

void WorkstationL07Action::setPriority(double priority)
{                               /* FIXME: should inherit */
  XBT_IN("(%p,%g)", this, priority);
  m_priority = priority;
  XBT_OUT();
}

double WorkstationL07Action::getRemains()
{
  XBT_IN("(%p)", this);
  XBT_OUT();
  return m_remains;
}

/*FIXME:remove static void ptask_finalize(void)
{
  xbt_dict_free(&ptask_parallel_task_link_set);

  delete surf_workstation_model;
  surf_workstation_model = NULL;
  delete surf_network_model;
  surf_network_model = NULL;

  ptask_host_count = 0;

  if (ptask_maxmin_system) {
    lmm_system_free(ptask_maxmin_system);
    ptask_maxmin_system = NULL;
  }
  }*/

/**************************************/
/******* Resource Private    **********/
/**************************************/

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/

static void ptask_parse_workstation_init(sg_platf_host_cbarg_t host)
{
  double power_peak = xbt_dynar_get_as(host->power_peak, host->pstate, double);
  //cpu->power_peak = power_peak;
  xbt_dynar_free(&(host->power_peak));  /* kill memory leak */
  static_cast<WorkstationL07ModelPtr>(surf_workstation_model)->createResource(
      host->id,
      power_peak,
      host->power_scale,
      host->power_trace,
      host->initial_state,
      host->state_trace,
      host->properties);
}

static void ptask_parse_cpu_init(sg_platf_host_cbarg_t host)
{
  double power_peak = xbt_dynar_get_as(host->power_peak, host->pstate, double);
  static_cast<CpuL07ModelPtr>(surf_cpu_model_pm)->createResource(
      host->id,
      power_peak,
      host->power_scale,
      host->power_trace,
      host->initial_state,
      host->state_trace,
      host->properties);
}



static void ptask_parse_link_init(sg_platf_link_cbarg_t link)
{
  if (link->policy == SURF_LINK_FULLDUPLEX) {
    char *link_id;
    link_id = bprintf("%s_UP", link->id);
    static_cast<NetworkL07ModelPtr>(surf_network_model)->createResource(link_id,
                               link->bandwidth,
                               link->bandwidth_trace,
                               link->latency,
                               link->latency_trace,
                               link->state,
                               link->state_trace,
                               link->policy,
                               link->properties);
    xbt_free(link_id);
    link_id = bprintf("%s_DOWN", link->id);
    static_cast<NetworkL07ModelPtr>(surf_network_model)->createResource(link_id,
                               link->bandwidth,
                               link->bandwidth_trace,
                               link->latency,
                               link->latency_trace,
                               link->state,
                               link->state_trace,
                               link->policy,
                               NULL); /* FIXME: We need to deep copy the
                                       * properties or we won't be able to free
                                       * it */
    xbt_free(link_id);
  } else {
	  static_cast<NetworkL07ModelPtr>(surf_network_model)->createResource(link->id,
                               link->bandwidth,
                               link->bandwidth_trace,
                               link->latency,
                               link->latency_trace,
                               link->state,
                               link->state_trace,
                               link->policy,
                               link->properties);
  }

  current_property_set = NULL;
}

static void ptask_add_traces(){
  static_cast<WorkstationL07ModelPtr>(surf_workstation_model)->addTraces();
}

static void ptask_define_callbacks()
{
  sg_platf_host_add_cb(ptask_parse_cpu_init);
  sg_platf_host_add_cb(ptask_parse_workstation_init);
  sg_platf_link_add_cb(ptask_parse_link_init);
  sg_platf_postparse_add_cb(ptask_add_traces);
}

/**************************************/
/*************** Generic **************/
/**************************************/
void surf_workstation_model_init_ptask_L07(void)
{
  XBT_INFO("surf_workstation_model_init_ptask_L07");
  xbt_assert(!surf_cpu_model_pm, "CPU model type already defined");
  xbt_assert(!surf_network_model, "network model type already defined");
  ptask_define_callbacks();
  surf_workstation_model = new WorkstationL07Model();
  ModelPtr model = static_cast<ModelPtr>(surf_workstation_model);
  xbt_dynar_push(model_list, &model);
  xbt_dynar_push(model_list_invoke, &model);
}
