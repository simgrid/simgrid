#include "network.hpp"
#include "maxmin_private.h"
#include "simgrid/sg_config.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf,
                                "Logging specific to the SURF network module");
}

NetworkCm02ModelPtr surf_network_model = NULL;

double sg_sender_gap = 0.0;
double sg_latency_factor = 1.0; /* default value; can be set by model or from command line */
double sg_bandwidth_factor = 1.0;       /* default value; can be set by model or from command line */
double sg_weight_S_parameter = 0.0;     /* default value; can be set by model or from command line */

double sg_tcp_gamma = 0.0;
int sg_network_crosstraffic = 0;

/*************
 * CallBacks *
 *************/

static void net_parse_link_init(sg_platf_link_cbarg_t link){
  if (link->policy == SURF_LINK_FULLDUPLEX) {
    char *link_id;
    link_id = bprintf("%s_UP", link->id);
    surf_network_model->createResource(link_id,
                      link->bandwidth,
                      link->bandwidth_trace,
                      link->latency,
                      link->latency_trace,
                      link->state,
                      link->state_trace, link->policy, link->properties);
    xbt_free(link_id);
    link_id = bprintf("%s_DOWN", link->id);
    surf_network_model->createResource(link_id,
                      link->bandwidth,
                      link->bandwidth_trace,
                      link->latency,
                      link->latency_trace,
                      link->state,
                      link->state_trace, link->policy, link->properties);
    xbt_free(link_id);
  } else {
	surf_network_model->createResource(link->id,
                      link->bandwidth,
                      link->bandwidth_trace,
                      link->latency,
                      link->latency_trace,
                      link->state,
                      link->state_trace, link->policy, link->properties);
  }
}

static void net_add_traces(void){
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  static int called = 0;
  if (called)
    return;
  called = 1;

  /* connect all traces relative to network */
  xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    NetworkCm02LinkLmmPtr link = static_cast<NetworkCm02LinkLmmPtr>(
    		                     static_cast<NetworkCm02LinkPtr>(
    		                		  xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL)));

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
               trace_name, elm);
    xbt_assert(trace,
               "Cannot connect trace %s to link %s: trace undefined",
               trace_name, elm);

    link->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    NetworkCm02LinkLmmPtr link = static_cast<NetworkCm02LinkLmmPtr>(
                                 static_cast<NetworkCm02LinkPtr>(
       	                              xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL)));

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
               trace_name, elm);
    xbt_assert(trace,
               "Cannot connect trace %s to link %s: trace undefined",
               trace_name, elm);

    link->p_power.event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    NetworkCm02LinkLmmPtr link = static_cast<NetworkCm02LinkLmmPtr>(
                                 static_cast<NetworkCm02LinkPtr>(
       	                              xbt_lib_get_or_null(link_lib, elm, SURF_LINK_LEVEL)));

    xbt_assert(link, "Cannot connect trace %s to link %s: link undefined",
               trace_name, elm);
    xbt_assert(trace,
               "Cannot connect trace %s to link %s: trace undefined",
               trace_name, elm);

    link->p_latEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

static void net_define_callbacks(void)
{
  /* Figuring out the network links */
  sg_platf_link_add_cb(net_parse_link_init);
  sg_platf_postparse_add_cb(net_add_traces);
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

  surf_network_model = new NetworkCm02Model();
  net_define_callbacks();
  ModelPtr model = static_cast<ModelPtr>(surf_network_model);
  xbt_dynar_push(model_list, &model);

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

  surf_network_model = new NetworkCm02Model();
  net_define_callbacks();
  ModelPtr model = static_cast<ModelPtr>(surf_network_model);
  xbt_dynar_push(model_list, &model);

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 1.0);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor",
                            1.0);
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

  surf_network_model = new NetworkCm02Model();
  net_define_callbacks();
  ModelPtr model = static_cast<ModelPtr>(surf_network_model);
  xbt_dynar_push(model_list, &model);
  lmm_set_default_protocol_function(func_reno_f, func_reno_fp,
                                    func_reno_fpi);
  surf_network_model->f_networkSolve = lagrange_solve;

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor",
                            0.92);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);
}


void surf_network_model_init_Reno2(void)
{
  if (surf_network_model)
    return;

  surf_network_model = new NetworkCm02Model();
  net_define_callbacks();
  ModelPtr model = static_cast<ModelPtr>(surf_network_model);
  xbt_dynar_push(model_list, &model);
  lmm_set_default_protocol_function(func_reno2_f, func_reno2_fp,
                                    func_reno2_fpi);
  surf_network_model->f_networkSolve = lagrange_solve;

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor",
                            0.92);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S_parameter",
                            8775);
}

void surf_network_model_init_Vegas(void)
{
  if (surf_network_model)
    return;

  surf_network_model = new NetworkCm02Model();
  net_define_callbacks();
  ModelPtr model = static_cast<ModelPtr>(surf_network_model);
  xbt_dynar_push(model_list, &model);
  lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
                                    func_vegas_fpi);
  surf_network_model->f_networkSolve = lagrange_solve;

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/latency_factor", 10.4);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/bandwidth_factor",
                            0.92);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);
}

NetworkCm02Model::NetworkCm02Model() : NetworkCm02Model("network"){
}

NetworkCm02Model::NetworkCm02Model(string name) : Model(name){
  NetworkCm02ActionLmmPtr comm;

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

  routing_model_create(createResource("__loopback__",
	                                           498000000, NULL, 0.000015, NULL,
	                                           SURF_RESOURCE_ON, NULL,
	                                           SURF_LINK_FATPIPE, NULL));

  if (p_updateMechanism == UM_LAZY) {
	p_actionHeap = xbt_heap_new(8, NULL);
	xbt_heap_set_update_callback(p_actionHeap, surf_action_lmm_update_index_heap);
	p_modifiedSet = xbt_swag_new(xbt_swag_offset((*comm), p_actionListHookup));
	p_maxminSystem->keep_track = p_modifiedSet;
  }
}

NetworkCm02LinkLmmPtr NetworkCm02Model::createResource(const char *name,
                                 double bw_initial,
                                 tmgr_trace_t bw_trace,
                                 double lat_initial,
                                 tmgr_trace_t lat_trace,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 e_surf_link_sharing_policy_t policy,
                                 xbt_dict_t properties)
{
  xbt_assert(!xbt_lib_get_or_null(link_lib, name, SURF_LINK_LEVEL),
             "Link '%s' declared several times in the platform file.",
             name);

  NetworkCm02LinkLmmPtr nw_link =
		  new NetworkCm02LinkLmm(this, name, properties, p_maxminSystem, sg_bandwidth_factor * bw_initial, history,
				                 state_initial, state_trace, bw_initial, bw_trace, lat_initial, lat_trace, policy);


  xbt_lib_set(link_lib, name, SURF_LINK_LEVEL, static_cast<NetworkCm02LinkPtr>(nw_link));
  XBT_DEBUG("Create link '%s'",name);

  return nw_link;
}

void NetworkCm02Model::updateActionsStateLazy(double now, double delta)
{
  NetworkCm02ActionLmmPtr action;
  while ((xbt_heap_size(p_actionHeap) > 0)
         && (double_equals(xbt_heap_maxkey(p_actionHeap), now))) {
    action = (NetworkCm02ActionLmmPtr) xbt_heap_pop(p_actionHeap);
    XBT_DEBUG("Something happened to action %p", action);
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      int n = lmm_get_number_of_cnst_from_var(p_maxminSystem, action->p_variable);
      unsigned int i;
      for (i = 0; i < n; i++){
        lmm_constraint_t constraint = lmm_get_cnst_from_var(p_maxminSystem,
                                                            action->p_variable,
                                                            i);
        NetworkCm02LinkPtr link = (NetworkCm02LinkPtr) lmm_constraint_id(constraint);
        TRACE_surf_link_set_utilization(link->m_name,
                                        action->p_category,
                                        (lmm_variable_getvalue(action->p_variable)*
                                            lmm_get_cnst_weight_from_var(p_maxminSystem,
                                                action->p_variable,
                                                i)),
                                        action->m_lastUpdate,
                                        now - action->m_lastUpdate);
      }
    }
#endif

    // if I am wearing a latency hat
    if (action->m_hat == LATENCY) {
      XBT_DEBUG("Latency paid for action %p. Activating", action);
      lmm_update_variable_weight(p_maxminSystem, action->p_variable, action->m_weight);
      action->heapRemove(p_actionHeap);
      action->m_lastUpdate = surf_get_clock();

        // if I am wearing a max_duration or normal hat
    } else if (action->m_hat == MAX_DURATION ||
        action->m_hat == NORMAL) {
        // no need to communicate anymore
        // assume that flows that reached max_duration have remaining of 0
      action->m_finish = surf_get_clock();
      XBT_DEBUG("Action %p finished", action);
      action->m_remains = 0;
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
      action->heapRemove(p_actionHeap);

      action->gapRemove();
    }
  }
  return;
}

xbt_dynar_t NetworkCm02Model::getRoute(RoutingEdgePtr src, RoutingEdgePtr dst)
{
  xbt_dynar_t route = NULL;
  routing_platf->getRouteAndLatency(src, dst, &route, NULL);
  return route;
}

NetworkCm02ActionPtr NetworkCm02Model::communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
                                                double size, double rate)
{
  unsigned int i;
  void *_link;
  NetworkCm02LinkLmmPtr link;
  int failed = 0;
  NetworkCm02ActionLmmPtr action = NULL;
  double bandwidth_bound;
  double latency = 0.0;
  xbt_dynar_t back_route = NULL;
  int constraints_per_variable = 0;

  xbt_dynar_t route = xbt_dynar_new(sizeof(RoutingEdgePtr), NULL);

  XBT_IN("(%s,%s,%g,%g)", src->p_name, dst->p_name, size, rate);

  routing_platf->getRouteAndLatency(src, dst, &route, &latency);
  xbt_assert(!xbt_dynar_is_empty(route) || latency,
             "You're trying to send data from %s to %s but there is no connection at all between these two hosts.",
             src->p_name, dst->p_name);

  xbt_dynar_foreach(route, i, _link) {
	link = static_cast<NetworkCm02LinkLmmPtr>(static_cast<NetworkCm02LinkPtr>(_link));
    if (link->p_stateCurrent == SURF_RESOURCE_OFF) {
      failed = 1;
      break;
    }
  }
  if (sg_network_crosstraffic == 1) {
	  routing_platf->getRouteAndLatency(dst, src, &back_route, NULL);
    xbt_dynar_foreach(back_route, i, _link) {
      link = static_cast<NetworkCm02LinkLmmPtr>(static_cast<NetworkCm02LinkPtr>(_link));
      if (link->p_stateCurrent == SURF_RESOURCE_OFF) {
        failed = 1;
        break;
      }
    }
  }

  action = new NetworkCm02ActionLmm(this, size, failed);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  action->m_latencyLimited = 0;
#endif
  action->m_weight = action->m_latency = latency;

  //FIXME:REMOVxbt_swag_insert(action, action->p_stateSet);
  action->m_rate = rate;
  if (p_updateMechanism == UM_LAZY) {
    action->m_indexHeap = -1;
    action->m_lastUpdate = surf_get_clock();
  }

  bandwidth_bound = -1.0;
  if (sg_weight_S_parameter > 0) {
    xbt_dynar_foreach(route, i, link) {
      action->m_weight +=
         sg_weight_S_parameter /
         (link->p_power.peak * link->p_power.scale);
    }
  }
  xbt_dynar_foreach(route, i, _link) {
	link = static_cast<NetworkCm02LinkLmmPtr>(static_cast<NetworkCm02LinkPtr>(_link));
    double bb = bandwidthFactor(size) * (link->p_power.peak * link->p_power.scale);
    bandwidth_bound =
        (bandwidth_bound < 0.0) ? bb : min(bandwidth_bound, bb);
  }

  action->m_latCurrent = action->m_latency;
  action->m_latency *= latencyFactor(size);
  action->m_rate = bandwidthConstraint(action->m_rate, bandwidth_bound, size);
  if (m_haveGap) {
    xbt_assert(!xbt_dynar_is_empty(route),
               "Using a model with a gap (e.g., SMPI) with a platform without links (e.g. vivaldi)!!!");

    //link = *(NetworkCm02LinkLmmPtr *) xbt_dynar_get_ptr(route, 0);
    link = static_cast<NetworkCm02LinkLmmPtr>(*static_cast<NetworkCm02LinkPtr *>(xbt_dynar_get_ptr(route, 0)));
    gapAppend(size, link, action);
    XBT_DEBUG("Comm %p: %s -> %s gap=%f (lat=%f)",
              action, src->p_name, dst->p_name, action->m_senderGap,
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
    lmm_update_variable_bound(p_maxminSystem, action->p_variable, (action->m_latCurrent > 0) ? sg_tcp_gamma / (2.0 * action->m_latCurrent) : -1.0);
  } else {
    lmm_update_variable_bound(p_maxminSystem, action->p_variable, (action->m_latCurrent > 0) ? min(action->m_rate, sg_tcp_gamma / (2.0 * action->m_latCurrent)) : action->m_rate);
  }

  xbt_dynar_foreach(route, i, _link) {
	link = static_cast<NetworkCm02LinkLmmPtr>(static_cast<NetworkCm02LinkPtr>(_link));
    lmm_expand(p_maxminSystem, link->p_constraint, action->p_variable, 1.0);
  }

  if (sg_network_crosstraffic == 1) {
    XBT_DEBUG("Fullduplex active adding backward flow using 5%%");
    xbt_dynar_foreach(back_route, i, _link) {
      link = static_cast<NetworkCm02LinkLmmPtr>(static_cast<NetworkCm02LinkPtr>(_link));
      lmm_expand(p_maxminSystem, link->p_constraint, action->p_variable, .05);
    }
  }

  xbt_dynar_free(&route);
  XBT_OUT();

  return action;
}

double NetworkCm02Model::latencyFactor(double size) {
  return sg_latency_factor;
}

double NetworkCm02Model::bandwidthFactor(double size) {
  return sg_bandwidth_factor;
}

double NetworkCm02Model::bandwidthConstraint(double rate, double bound, double size) {
  return rate;
}

/************
 * Resource *
 ************/
NetworkCm02LinkLmm::NetworkCm02LinkLmm(NetworkCm02ModelPtr model, const char *name, xbt_dict_t props,
	                           lmm_system_t system,
	                           double constraint_value,
	                           tmgr_history_t history,
	                           e_surf_resource_state_t state_init,
	                           tmgr_trace_t state_trace,
	                           double metric_peak,
	                           tmgr_trace_t metric_trace,
	                           double lat_initial,
	                           tmgr_trace_t lat_trace,
	                           e_surf_link_sharing_policy_t policy)
: Resource(model, name, props),
  ResourceLmm(model, name, props, system, constraint_value, history, state_init, state_trace, metric_peak, metric_trace),
  NetworkCm02Link(model, name, props)
{
  m_latCurrent = lat_initial;
  if (lat_trace)
	p_latEvent = tmgr_history_add_trace(history, lat_trace, 0.0, 0, this);

  if (policy == SURF_LINK_FATPIPE)
	lmm_constraint_shared(p_constraint);
}

bool NetworkCm02LinkLmm::isUsed()
{
  return lmm_constraint_used(p_model->p_maxminSystem, p_constraint);
}

double NetworkCm02Link::getLatency()
{
  return m_latCurrent;
}

double NetworkCm02LinkLmm::getBandwidth()
{
  return p_power.peak * p_power.scale;
}

bool NetworkCm02LinkLmm::isShared()
{
  return lmm_constraint_is_shared(p_constraint);
}

void NetworkCm02LinkLmm::updateState(tmgr_trace_event_t event_type,
                                      double value, double date)
{
  /*   printf("[" "%lg" "] Asking to update network card \"%s\" with value " */
  /*     "%lg" " for event %p\n", surf_get_clock(), nw_link->name, */
  /*     value, event_type); */

  if (event_type == p_power.event) {
    double delta =
        sg_weight_S_parameter / value - sg_weight_S_parameter /
        (p_power.peak * p_power.scale);
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    NetworkCm02ActionLmmPtr action = NULL;

    p_power.peak = value;
    lmm_update_constraint_bound(p_model->p_maxminSystem,
                                p_constraint,
                                sg_bandwidth_factor *
                                (p_power.peak * p_power.scale));
#ifdef HAVE_TRACING
    TRACE_surf_link_set_bandwidth(date, m_name, sg_bandwidth_factor * p_power.peak * p_power.scale);
#endif
    if (sg_weight_S_parameter > 0) {
      while ((var = lmm_get_var_from_cnst(p_model->p_maxminSystem, p_constraint, &elem))) {
        action = (NetworkCm02ActionLmmPtr) lmm_variable_id(var);
        action->m_weight += delta;
        if (!action->m_suspended)
          lmm_update_variable_weight(p_model->p_maxminSystem, action->p_variable, action->m_weight);
      }
    }
    if (tmgr_trace_event_free(event_type))
      p_power.event = NULL;
  } else if (event_type == p_latEvent) {
    double delta = value - m_latCurrent;
    lmm_variable_t var = NULL;
    lmm_element_t elem = NULL;
    NetworkCm02ActionLmmPtr action = NULL;

    m_latCurrent = value;
    while ((var = lmm_get_var_from_cnst(p_model->p_maxminSystem, p_constraint, &elem))) {
      action = (NetworkCm02ActionLmmPtr) lmm_variable_id(var);
      action->m_latCurrent += delta;
      action->m_weight += delta;
      if (action->m_rate < 0)
        lmm_update_variable_bound(p_model->p_maxminSystem, action->p_variable, sg_tcp_gamma / (2.0 * action->m_latCurrent));
      else {
        lmm_update_variable_bound(p_model->p_maxminSystem, action->p_variable,
                                  min(action->m_rate, sg_tcp_gamma / (2.0 * action->m_latCurrent)));

        if (action->m_rate < sg_tcp_gamma / (2.0 * action->m_latCurrent)) {
          XBT_INFO("Flow is limited BYBANDWIDTH");
        } else {
          XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f",
                   action->m_latCurrent);
        }
      }
      if (!action->m_suspended)
        lmm_update_variable_weight(p_model->p_maxminSystem, action->p_variable, action->m_weight);

    }
    if (tmgr_trace_event_free(event_type))
      p_latEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      p_stateCurrent = SURF_RESOURCE_ON;
    else {
      lmm_constraint_t cnst = p_constraint;
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;

      p_stateCurrent = SURF_RESOURCE_OFF;
      while ((var = lmm_get_var_from_cnst(p_model->p_maxminSystem, cnst, &elem))) {
        ActionPtr action = (ActionPtr) lmm_variable_id(var);

        if (action->getState() == SURF_ACTION_RUNNING ||
            action->getState() == SURF_ACTION_READY) {
          action->m_finish = date;
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
       p_constraint);
  return;
}

/**********
 * Action *
 **********/
void NetworkCm02ActionLmm::updateRemainingLazy(double now)
{
  double delta = 0.0;

  if (m_suspended != 0)
    return;

  delta = now - m_lastUpdate;

  if (m_remains > 0) {
    XBT_DEBUG("Updating action(%p): remains was %lf, last_update was: %lf", this, m_remains, m_lastUpdate);
    double_update(&(m_remains), m_lastValue * delta);

    XBT_DEBUG("Updating action(%p): remains is now %lf", this, m_remains);
  }

  if (m_maxDuration != NO_MAX_DURATION)
    double_update(&m_maxDuration, delta);

  if (m_remains <= 0 &&
      (lmm_get_variable_weight(p_variable) > 0)) {
    m_finish = surf_get_clock();
    setState(SURF_ACTION_DONE);

    heapRemove(p_model->p_actionHeap);
  } else if (((m_maxDuration != NO_MAX_DURATION)
      && (m_maxDuration <= 0))) {
    m_finish = surf_get_clock();
    setState(SURF_ACTION_DONE);
    heapRemove(p_model->p_actionHeap);
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(p_variable);
}
void NetworkCm02ActionLmm::recycle()
{
  return;
}

