/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "surf_private.h"

typedef enum {
  SURF_WORKSTATION_RESOURCE_CPU,
  SURF_WORKSTATION_RESOURCE_LINK,
} e_surf_workstation_resource_type_t;

/**************************************/
/********* cpu object *****************/
/**************************************/
typedef struct cpu_L07 {
  surf_resource_t resource;	/* Do not move this field */
  e_surf_workstation_resource_type_t type;	/* Do not move this field */
  char *name;			/* Do not move this field */
  lmm_constraint_t constraint;	/* Do not move this field */
  double power_scale;
  double power_current;
  tmgr_trace_event_t power_event;
  e_surf_cpu_state_t state_current;
  tmgr_trace_event_t state_event;
  int id;			/* cpu and network card are a single object... */
} s_cpu_L07_t, *cpu_L07_t;

/**************************************/
/*********** network object ***********/
/**************************************/

typedef struct network_link_L07 {
  surf_resource_t resource;	/* Do not move this field */
  e_surf_workstation_resource_type_t type;	/* Do not move this field */
  char *name;			/* Do not move this field */
  lmm_constraint_t constraint;	/* Do not move this field */
  double bw_current;
  tmgr_trace_event_t bw_event;
  e_surf_network_link_state_t state_current;
  tmgr_trace_event_t state_event;
} s_network_link_L07_t, *network_link_L07_t;


typedef struct s_route_L07 {
  network_link_L07_t *links;
  int size;
} s_route_L07_t, *route_L07_t;

/**************************************/
/*************** actions **************/
/**************************************/
typedef struct surf_action_workstation_L07 {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
  double rate;
  int suspended;
} s_surf_action_workstation_L07_t, *surf_action_workstation_L07_t;


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_workstation);

static int nb_workstation = 0;
static s_route_L07_t *routing_table = NULL;
#define ROUTE(i,j) routing_table[(i)+(j)*nb_workstation]
static network_link_L07_t loopback = NULL;
static xbt_dict_t parallel_task_network_link_set = NULL;
lmm_system_t ptask_maxmin_system = NULL;

/**************************************/
/******* Resource Public     **********/
/**************************************/

static void *name_service(const char *name)
{
  return xbt_dict_get_or_null(workstation_set, name);
}

static const char *get_resource_name(void *resource_id)
{
  /* We can freely cast as a cpu_L07_t because it has the same
     prefix as network_link_L07_t. However, only cpu_L07_t
     will theoretically be given as an argument here. */
  return ((cpu_L07_t) resource_id)->name;
}

/* action_get_state is inherited from the surf module */

static void action_use(surf_action_t action)
{
  action->using++;
  return;
}

static int action_free(surf_action_t action)
{
  action->using--;

  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_workstation_L07_t) action)->variable)
      lmm_variable_free(ptask_maxmin_system,
			((surf_action_workstation_L07_t) action)->
			variable);
    free(action);
    return 1;
  }
  return 0;
}

static void action_cancel(surf_action_t action)
{
  surf_action_change_state(action, SURF_ACTION_FAILED);
  return;
}

static void action_recycle(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

/* action_change_state is inherited from the surf module */
/* action_set_data is inherited from the surf module */

static void action_suspend(surf_action_t action)
{
  XBT_IN1("(%p))", action);
  if (((surf_action_workstation_L07_t) action)->suspended != 2) {
    ((surf_action_workstation_L07_t) action)->suspended = 1;
    lmm_update_variable_weight(ptask_maxmin_system,
			       ((surf_action_workstation_L07_t)
				action)->variable, 0.0);
  }
  XBT_OUT;
}

static void action_resume(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_workstation_L07_t) action)->suspended != 2) {
    lmm_update_variable_weight(ptask_maxmin_system,
			       ((surf_action_workstation_L07_t)
				action)->variable, 1.0);
    ((surf_action_workstation_L07_t) action)->suspended = 0;
  }
  XBT_OUT;
}

static int action_is_suspended(surf_action_t action)
{
  return (((surf_action_workstation_L07_t) action)->suspended == 1);
}

static void action_set_max_duration(surf_action_t action, double duration)
{				/* FIXME: should inherit */
  XBT_IN2("(%p,%g)", action, duration);
  action->max_duration = duration;
  XBT_OUT;
}


static void action_set_priority(surf_action_t action, double priority)
{				/* FIXME: should inherit */
  XBT_IN2("(%p,%g)", action, priority);
  action->priority = priority;
  XBT_OUT;
}

/**************************************/
/******* Resource Private    **********/
/**************************************/

static int resource_used(void *resource_id)
{
  /* We can freely cast as a network_link_L07_t because it has
     the same prefix as cpu_L07_t */
  return lmm_constraint_used(ptask_maxmin_system,
			     ((network_link_L07_t) resource_id)->
			     constraint);

}

static double share_resources(double now)
{
  s_surf_action_workstation_L07_t s_action;

  xbt_swag_t running_actions =
      surf_workstation_resource->common_public->states.running_action_set;
  double min = generic_maxmin_share_resources2(running_actions,
					       xbt_swag_offset(s_action,
							       variable),
					       ptask_maxmin_system,
					       bottleneck_solve);

  DEBUG1("min value : %f", min);

  return min;
}

static void update_actions_state(double now, double delta)
{
  surf_action_workstation_L07_t action = NULL;
  surf_action_workstation_L07_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_workstation_resource->common_public->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    DEBUG3("Action (%p) : remains (%g) updated by %g.",
	   action, action->generic_action.remains,
	   lmm_variable_getvalue(action->variable) * delta);
    double_update(&(action->generic_action.remains),
		  lmm_variable_getvalue(action->variable) * delta);

    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    DEBUG2("Action (%p) : remains (%g).",
	   action, action->generic_action.remains);
    if ((action->generic_action.remains <= 0) &&
	(lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {
      /* Need to check that none of the resource has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      void *constraint_id = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(ptask_maxmin_system, action->variable,
				    i++))) {
	constraint_id = lmm_constraint_id(cnst);

/* 	if(((network_link_L07_t)constraint_id)->type== */
/* 	   SURF_WORKSTATION_RESOURCE_LINK) { */
/* 	  DEBUG2("Checking for link %s (%p)", */
/* 		 ((network_link_L07_t)constraint_id)->name, */
/* 		 ((network_link_L07_t)constraint_id)); */
/* 	} */
/* 	if(((cpu_L07_t)constraint_id)->type== */
/* 	   SURF_WORKSTATION_RESOURCE_CPU) { */
/* 	  DEBUG3("Checking for cpu %s (%p) : %s", */
/* 		 ((cpu_L07_t)constraint_id)->name, */
/* 		 ((cpu_L07_t)constraint_id), */
/* 		 ((cpu_L07_t)constraint_id)->state_current==SURF_CPU_OFF?"Off":"On"); */
/* 	} */

	if (((((network_link_L07_t) constraint_id)->type ==
	      SURF_WORKSTATION_RESOURCE_LINK) &&
	     (((network_link_L07_t) constraint_id)->state_current ==
	      SURF_NETWORK_LINK_OFF)) ||
	    ((((cpu_L07_t) constraint_id)->type ==
	      SURF_WORKSTATION_RESOURCE_CPU) &&
	     (((cpu_L07_t) constraint_id)->state_current ==
	      SURF_CPU_OFF))) {
	  DEBUG1("Action (%p) Failed!!", action);
	  action->generic_action.finish = surf_get_clock();
	  surf_action_change_state((surf_action_t) action,
				   SURF_ACTION_FAILED);
	  break;
	}
      }
    }
  }
  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  cpu_L07_t cpu = id;
  network_link_L07_t nw_link = id;

  if (nw_link->type == SURF_WORKSTATION_RESOURCE_LINK) {
    DEBUG2("Updating link %s (%p)", nw_link->name, nw_link);
    if (event_type == nw_link->bw_event) {
      nw_link->bw_current = value;
      lmm_update_constraint_bound(ptask_maxmin_system, nw_link->constraint,
				  nw_link->bw_current);
    } else if (event_type == nw_link->state_event) {
      if (value > 0)
	nw_link->state_current = SURF_NETWORK_LINK_ON;
      else
	nw_link->state_current = SURF_NETWORK_LINK_OFF;
    } else {
      CRITICAL0("Unknown event ! \n");
      xbt_abort();
    }
    return;
  } else if (cpu->type == SURF_WORKSTATION_RESOURCE_CPU) {
    DEBUG3("Updating cpu %s (%p) with value %g", cpu->name, cpu, value);
    if (event_type == cpu->power_event) {
      cpu->power_current = value;
      lmm_update_constraint_bound(ptask_maxmin_system, cpu->constraint,
				  cpu->power_current);
    } else if (event_type == cpu->state_event) {
      if (value > 0)
	cpu->state_current = SURF_CPU_ON;
      else
	cpu->state_current = SURF_CPU_OFF;
    } else {
      CRITICAL0("Unknown event ! \n");
      xbt_abort();
    }
    return;
  } else {
    DIE_IMPOSSIBLE;
  }
  return;
}

static void finalize(void)
{
  int i, j;

  xbt_dict_free(&network_link_set);
  xbt_dict_free(&workstation_set);
  if (parallel_task_network_link_set != NULL) {
    xbt_dict_free(&parallel_task_network_link_set);
  }
  xbt_swag_free(surf_workstation_resource->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.
		done_action_set);

  free(surf_workstation_resource->common_public);
  free(surf_workstation_resource->common_private);
  free(surf_workstation_resource->extension_public);

  free(surf_workstation_resource);
  surf_workstation_resource = NULL;

  for (i = 0; i < nb_workstation; i++)
    for (j = 0; j < nb_workstation; j++)
      free(ROUTE(i, j).links);
  free(routing_table);
  routing_table = NULL;
  nb_workstation = 0;

  if (ptask_maxmin_system) {
    lmm_system_free(ptask_maxmin_system);
    ptask_maxmin_system = NULL;
  }
}

/**************************************/
/******* Resource Private    **********/
/**************************************/

static e_surf_cpu_state_t resource_get_state(void *cpu)
{
  return ((cpu_L07_t) cpu)->state_current;
}

static double get_speed(void *cpu, double load)
{
  return load * (((cpu_L07_t) cpu)->power_scale);
}

static double get_available_speed(void *cpu)
{
  return ((cpu_L07_t) cpu)->power_current;
}

static surf_action_t execute_parallel_task(int workstation_nb,
					   void **workstation_list,
					   double *computation_amount,
					   double *communication_amount,
					   double amount, double rate)
{
  surf_action_workstation_L07_t action = NULL;
  int i, j, k;
  int nb_link = 0;
  int nb_host = 0;

  if (parallel_task_network_link_set == NULL) {
    parallel_task_network_link_set =
	xbt_dict_new_ext(workstation_nb * workstation_nb * 10);
  }

  /* Compute the number of affected resources... */
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_L07_t card_src = workstation_list[i];
      cpu_L07_t card_dst = workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      network_link_L07_t *route = ROUTE(card_src->id, card_dst->id).links;

      if (communication_amount[i * workstation_nb + j] > 0)
	for (k = 0; k < route_size; k++) {
	  xbt_dict_set(parallel_task_network_link_set, route[k]->name,
		       route[k], NULL);
	}
    }
  }
  nb_link = xbt_dict_length(parallel_task_network_link_set);
  xbt_dict_reset(parallel_task_network_link_set);

  for (i = 0; i < workstation_nb; i++)
    if (computation_amount[i] > 0)
      nb_host++;

  action = xbt_new0(s_surf_action_workstation_L07_t, 1);
  DEBUG3("Creating a parallel task (%p) with %d cpus and %d links.",
	 action, workstation_nb, nb_link);
  action->generic_action.using = 1;
  action->generic_action.cost = amount;
  action->generic_action.remains = amount;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
  action->generic_action.resource_type =
      (surf_resource_t) surf_workstation_resource;
  action->suspended = 0;	/* Should be useless because of the
				   calloc but it seems to help valgrind... */
  action->generic_action.state_set =
      surf_workstation_resource->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  if (action->rate > 0)
    action->variable =
	lmm_variable_new(ptask_maxmin_system, action, 1.0, -1.0,
			 workstation_nb + nb_link);
  else
    action->variable =
	lmm_variable_new(ptask_maxmin_system, action, 1.0, action->rate,
			 workstation_nb + nb_link);

  for (i = 0; i < workstation_nb; i++)
    lmm_expand(ptask_maxmin_system,
	       ((cpu_L07_t) workstation_list[i])->constraint,
	       action->variable, computation_amount[i]);
  
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_L07_t card_src = workstation_list[i];
      cpu_L07_t card_dst = workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      network_link_L07_t *route = ROUTE(card_src->id, card_dst->id).links;
      
      if (communication_amount[i * workstation_nb + j] == 0.0) 
	continue;
      for (k = 0; k < route_size; k++) {
	  lmm_expand_add(ptask_maxmin_system, route[k]->constraint,
			 action->variable,
			 communication_amount[i * workstation_nb + j]);
      }
    }
  }

  if (nb_link + nb_host == 0) {
    action->generic_action.cost = 1.0;
    action->generic_action.remains = 0.0;
  }

  return (surf_action_t) action;
}

static surf_action_t execute(void *cpu, double size)
{
  double val = 0.0;

  return execute_parallel_task(1, &cpu, &size, &val, 1, -1);
}

static surf_action_t communicate(void *src, void *dst, double size,
				 double rate)
{
  void **workstation_list = xbt_new0(void *, 2);
  double *computation_amount = xbt_new0(double, 2);
  double *communication_amount = xbt_new0(double, 4);
  surf_action_t res = NULL;

  workstation_list[0] = src;
  workstation_list[1] = dst;
  communication_amount[1] = size;

  res = execute_parallel_task(2, workstation_list,
			      computation_amount, communication_amount,
			      1, rate);

  free(computation_amount);
  free(communication_amount);
  free(workstation_list);

  return res;
}

static surf_action_t action_sleep(void *cpu, double duration)
{
  surf_action_workstation_L07_t action = NULL;

  XBT_IN2("(%s,%g)", ((cpu_L07_t) cpu)->name, duration);

  action = (surf_action_workstation_L07_t) execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  action->suspended = 2;
  lmm_update_variable_weight(ptask_maxmin_system, action->variable, 0.0);

  XBT_OUT;
  return (surf_action_t) action;
}

/* returns an array of network_link_L07_t */
static const void **get_route(void *src, void *dst)
{
  cpu_L07_t card_src = src;
  cpu_L07_t card_dst = dst;
  route_L07_t route = &(ROUTE(card_src->id, card_dst->id));

  return (const void **) route->links;
}

static int get_route_size(void *src, void *dst)
{
  cpu_L07_t card_src = src;
  cpu_L07_t card_dst = dst;
  route_L07_t route = &(ROUTE(card_src->id, card_dst->id));
  return route->size;
}

static const char *get_link_name(const void *link)
{
  return ((network_link_L07_t) link)->name;
}

static double get_link_bandwidth(const void *link)
{
  return ((network_link_L07_t) link)->bw_current;
}

static double get_link_latency(const void *link)
{
  static int warned = 0;

  if(!warned) {
    WARN0("This model does not take latency into account.");
    warned = 1;
  }
  return 0.0;
}

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/

static void cpu_free(void *cpu)
{
  free(((cpu_L07_t) cpu)->name);
  free(cpu);
}

static cpu_L07_t cpu_new(const char *name, double power_scale,
			 double power_initial,
			 tmgr_trace_t power_trace,
			 e_surf_cpu_state_t state_initial,
			 tmgr_trace_t state_trace)
{
  cpu_L07_t cpu = xbt_new0(s_cpu_L07_t, 1);

  cpu->resource = (surf_resource_t) surf_workstation_resource;
  cpu->type = SURF_WORKSTATION_RESOURCE_CPU;
  cpu->name = xbt_strdup(name);
  cpu->id = nb_workstation++;

  cpu->power_scale = power_scale;
  xbt_assert0(cpu->power_scale > 0, "Power has to be >0");

  cpu->power_current = power_initial;
  if (power_trace)
    cpu->power_event =
	tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
      lmm_constraint_new(ptask_maxmin_system, cpu,
			 cpu->power_current * cpu->power_scale);

  xbt_dict_set(workstation_set, name, cpu, cpu_free);

  return cpu;
}

static void parse_cpu(void)
{
  double power_scale = 0.0;
  double power_initial = 0.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_cpu_state_t state_initial = SURF_CPU_OFF;
  tmgr_trace_t state_trace = NULL;

  surf_parse_get_double(&power_scale, A_surfxml_cpu_power);
  surf_parse_get_double(&power_initial, A_surfxml_cpu_availability);
  surf_parse_get_trace(&power_trace, A_surfxml_cpu_availability_file);

  xbt_assert0((A_surfxml_cpu_state == A_surfxml_cpu_state_ON) ||
	      (A_surfxml_cpu_state == A_surfxml_cpu_state_OFF),
	      "Invalid state");
  if (A_surfxml_cpu_state == A_surfxml_cpu_state_ON)
    state_initial = SURF_CPU_ON;
  if (A_surfxml_cpu_state == A_surfxml_cpu_state_OFF)
    state_initial = SURF_CPU_OFF;
  surf_parse_get_trace(&state_trace, A_surfxml_cpu_state_file);

  cpu_new(A_surfxml_cpu_name, power_scale, power_initial, power_trace,
	  state_initial, state_trace);
}

static void create_routing_table(void)
{
  routing_table = xbt_new0(s_route_L07_t, nb_workstation * nb_workstation);
}

static void network_link_free(void *nw_link)
{
  free(((network_link_L07_t) nw_link)->name);
  free(nw_link);
}

static network_link_L07_t network_link_new(char *name,
					   double bw_initial,
					   tmgr_trace_t bw_trace,
					   e_surf_network_link_state_t
					   state_initial,
					   tmgr_trace_t state_trace,
					   e_surf_network_link_sharing_policy_t
					   policy)
{
  network_link_L07_t nw_link = xbt_new0(s_network_link_L07_t, 1);


  nw_link->resource = (surf_resource_t) surf_workstation_resource;
  nw_link->type = SURF_WORKSTATION_RESOURCE_LINK;
  nw_link->name = name;
  nw_link->bw_current = bw_initial;
  if (bw_trace)
    nw_link->bw_event =
	tmgr_history_add_trace(history, bw_trace, 0.0, 0, nw_link);
  nw_link->state_current = state_initial;
  if (state_trace)
    nw_link->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, nw_link);

  nw_link->constraint =
      lmm_constraint_new(ptask_maxmin_system, nw_link,
			 nw_link->bw_current);

  if (policy == SURF_NETWORK_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->constraint);

  xbt_dict_set(network_link_set, name, nw_link, network_link_free);

  return nw_link;
}

static void parse_network_link(void)
{
  char *name;
  double bw_initial;
  tmgr_trace_t bw_trace;
  e_surf_network_link_state_t state_initial = SURF_NETWORK_LINK_ON;
  e_surf_network_link_sharing_policy_t policy_initial =
      SURF_NETWORK_LINK_SHARED;
  tmgr_trace_t state_trace;

  name = xbt_strdup(A_surfxml_network_link_name);
  surf_parse_get_double(&bw_initial, A_surfxml_network_link_bandwidth);
  surf_parse_get_trace(&bw_trace, A_surfxml_network_link_bandwidth_file);

  xbt_assert0((A_surfxml_network_link_state ==
	       A_surfxml_network_link_state_ON)
	      || (A_surfxml_network_link_state ==
		  A_surfxml_network_link_state_OFF), "Invalid state");
  if (A_surfxml_network_link_state == A_surfxml_network_link_state_ON)
    state_initial = SURF_NETWORK_LINK_ON;
  else if (A_surfxml_network_link_state ==
	   A_surfxml_network_link_state_OFF)
    state_initial = SURF_NETWORK_LINK_OFF;

  if (A_surfxml_network_link_sharing_policy ==
      A_surfxml_network_link_sharing_policy_SHARED)
    policy_initial = SURF_NETWORK_LINK_SHARED;
  else if (A_surfxml_network_link_sharing_policy ==
	   A_surfxml_network_link_sharing_policy_FATPIPE)
    policy_initial = SURF_NETWORK_LINK_FATPIPE;

  surf_parse_get_trace(&state_trace, A_surfxml_network_link_state_file);

  network_link_new(name, bw_initial, bw_trace, state_initial, state_trace,
		   policy_initial);
}

static void route_new(int src_id, int dst_id,
		      network_link_L07_t * link_list, int nb_link)
{
  route_L07_t route = &(ROUTE(src_id, dst_id));

  route->size = nb_link;
  route->links = link_list =
      xbt_realloc(link_list, sizeof(network_link_L07_t) * nb_link);
}

static int nb_link;
static int link_list_capacity;
static network_link_L07_t *link_list = NULL;
static int src_id = -1;
static int dst_id = -1;

static void parse_route_set_endpoints(void)
{
  cpu_L07_t cpu_tmp = NULL;

  cpu_tmp = (cpu_L07_t) name_service(A_surfxml_route_src);
  xbt_assert1(cpu_tmp, "Invalid cpu %s", A_surfxml_route_src);
  if (cpu_tmp != NULL)
    src_id = cpu_tmp->id;

  cpu_tmp = (cpu_L07_t) name_service(A_surfxml_route_dst);
  xbt_assert1(cpu_tmp, "Invalid cpu %s", A_surfxml_route_dst);
  if (cpu_tmp != NULL)
    dst_id = cpu_tmp->id;

  nb_link = 0;
  link_list_capacity = 1;
  link_list = xbt_new(network_link_L07_t, link_list_capacity);
}

static void parse_route_elem(void)
{
  xbt_ex_t e;
  if (nb_link == link_list_capacity) {
    link_list_capacity *= 2;
    link_list =
	xbt_realloc(link_list,
		    (link_list_capacity) * sizeof(network_link_L07_t));
  }
  TRY {
    link_list[nb_link++] =
	xbt_dict_get(network_link_set, A_surfxml_route_element_name);
  }
  CATCH(e) {
    RETHROW1("Link %s not found (dict raised this exception: %s)",
	     A_surfxml_route_element_name);
  }
}

static void parse_route_set_route(void)
{
  if (src_id != -1 && dst_id != -1)
    route_new(src_id, dst_id, link_list, nb_link);
}

static void parse_file(const char *file)
{
  int i;

  /* Figuring out the cpus */
  surf_parse_reset_parser();
  ETag_surfxml_cpu_fun = parse_cpu;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

  create_routing_table();

  /* Figuring out the network links */
  surf_parse_reset_parser();
  ETag_surfxml_network_link_fun = parse_network_link;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

  /* Building the routes */
  surf_parse_reset_parser();
  STag_surfxml_route_fun = parse_route_set_endpoints;
  ETag_surfxml_route_element_fun = parse_route_elem;
  ETag_surfxml_route_fun = parse_route_set_route;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

  /* Adding loopback if needed */
  for (i = 0; i < nb_workstation; i++)
    if (!ROUTE(i, i).size) {
      if (!loopback)
	loopback = network_link_new(xbt_strdup("__MSG_loopback__"),
				    498000000, NULL,
				    SURF_NETWORK_LINK_ON, NULL,
				    SURF_NETWORK_LINK_FATPIPE);

      ROUTE(i, i).size = 1;
      ROUTE(i, i).links = xbt_new0(network_link_L07_t, 1);
      ROUTE(i, i).links[0] = loopback;
    }
}

/**************************************/
/********* Module  creation ***********/
/**************************************/

static void resource_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_resource = xbt_new0(s_surf_workstation_resource_t, 1);

  surf_workstation_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_workstation_resource->common_public =
      xbt_new0(s_surf_resource_public_t, 1);
  surf_workstation_resource->extension_public =
      xbt_new0(s_surf_workstation_resource_extension_public_t, 1);

  surf_workstation_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_resource->common_public->name_service = name_service;
  surf_workstation_resource->common_public->get_resource_name =
      get_resource_name;
  surf_workstation_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_workstation_resource->common_public->action_get_start_time =
      surf_action_get_start_time;
  surf_workstation_resource->common_public->action_get_finish_time =
      surf_action_get_finish_time;
  surf_workstation_resource->common_public->action_use = action_use;
  surf_workstation_resource->common_public->action_free = action_free;
  surf_workstation_resource->common_public->action_cancel = action_cancel;
  surf_workstation_resource->common_public->action_recycle =
      action_recycle;
  surf_workstation_resource->common_public->action_change_state =
      surf_action_change_state;
  surf_workstation_resource->common_public->action_set_data =
      surf_action_set_data;
  surf_workstation_resource->common_public->suspend = action_suspend;
  surf_workstation_resource->common_public->resume = action_resume;
  surf_workstation_resource->common_public->is_suspended =
      action_is_suspended;
  surf_workstation_resource->common_public->set_max_duration =
      action_set_max_duration;
  surf_workstation_resource->common_public->set_priority =
      action_set_priority;
  surf_workstation_resource->common_public->name = "Workstation ptask_L07";

  surf_workstation_resource->common_private->resource_used = resource_used;
  surf_workstation_resource->common_private->share_resources =
      share_resources;
  surf_workstation_resource->common_private->update_actions_state =
      update_actions_state;
  surf_workstation_resource->common_private->update_resource_state =
      update_resource_state;
  surf_workstation_resource->common_private->finalize = finalize;

  surf_workstation_resource->extension_public->execute = execute;
  surf_workstation_resource->extension_public->sleep = action_sleep;
  surf_workstation_resource->extension_public->get_state =
      resource_get_state;
  surf_workstation_resource->extension_public->get_speed = get_speed;
  surf_workstation_resource->extension_public->get_available_speed =
      get_available_speed;
  surf_workstation_resource->extension_public->communicate = communicate;
  surf_workstation_resource->extension_public->execute_parallel_task =
      execute_parallel_task;
  surf_workstation_resource->extension_public->get_route = get_route;
  surf_workstation_resource->extension_public->get_route_size =
      get_route_size;
  surf_workstation_resource->extension_public->get_link_name =
      get_link_name;
  surf_workstation_resource->extension_public->get_link_bandwidth =
      get_link_bandwidth;
  surf_workstation_resource->extension_public->get_link_latency =
      get_link_latency;

  workstation_set = xbt_dict_new();
  network_link_set = xbt_dict_new();

  if (!ptask_maxmin_system)
    ptask_maxmin_system = lmm_system_new();
}

/**************************************/
/*************** Generic **************/
/**************************************/
void surf_workstation_resource_init_ptask_L07(const char *filename)
{
  xbt_assert0(!surf_cpu_resource, "CPU resource type already defined");
  xbt_assert0(!surf_network_resource,
	      "network resource type already defined");
  resource_init_internal();
  parse_file(filename);
  WARN0("This model does not take latency into account.");

  update_resource_description(surf_workstation_resource_description,
			      surf_workstation_resource_description_size,
			      "ptask_L07",
			      (surf_resource_t) surf_workstation_resource);
  xbt_dynar_push(resource_list, &surf_workstation_resource);
}
