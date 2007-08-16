/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "workstation_KCCFLN05_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_workstation, surf,
				"Logging specific to the SURF workstation module (KCCFLN05)");

static int nb_workstation = 0;
static s_route_KCCFLN05_t *routing_table = NULL;
#define ROUTE(i,j) routing_table[(i)+(j)*nb_workstation]
static network_link_KCCFLN05_t loopback = NULL;
static xbt_dict_t parallel_task_network_link_set = NULL;
//added to work with GTNETS
static xbt_dict_t router_set = NULL;

/*xbt_dict_t network_link_set = NULL;*/


/* convenient function */
static void __update_cpu_usage(cpu_KCCFLN05_t cpu)
{
  int cpt;
  surf_action_workstation_KCCFLN05_t action = NULL;
  if ((!xbt_dynar_length(cpu->incomming_communications)) &&
      (!xbt_dynar_length(cpu->outgoing_communications))) {
    /* No communications */
    lmm_update_constraint_bound(maxmin_system, cpu->constraint,
				cpu->power_current * cpu->power_scale);
  } else if ((!xbt_dynar_length(cpu->incomming_communications))
	     && (xbt_dynar_length(cpu->outgoing_communications))) {
    /* Emission */
    lmm_update_constraint_bound(maxmin_system, cpu->constraint,
				cpu->power_current * cpu->power_scale *
				cpu->interference_send);
    xbt_dynar_foreach(cpu->outgoing_communications, cpt, action)
	lmm_elem_set_value(maxmin_system, cpu->constraint,
			   action->variable,
			   cpu->power_current * cpu->power_scale *
			   ROUTE(action->src->id,
				 action->dst->id).impact_on_src);
  } else if ((xbt_dynar_length(cpu->incomming_communications))
	     && (!xbt_dynar_length(cpu->outgoing_communications))) {
    /* Reception */
    lmm_update_constraint_bound(maxmin_system, cpu->constraint,
				cpu->power_current * cpu->power_scale *
				cpu->interference_recv);
    xbt_dynar_foreach(cpu->incomming_communications, cpt, action)
	lmm_elem_set_value(maxmin_system, cpu->constraint,
			   action->variable,
			   cpu->power_current * cpu->power_scale *
			   ROUTE(action->src->id,
				 action->dst->id).impact_on_dst);
  } else {
    /* Emission & Reception */
    lmm_update_constraint_bound(maxmin_system, cpu->constraint,
				cpu->power_current * cpu->power_scale *
				cpu->interference_send_recv);
    xbt_dynar_foreach(cpu->outgoing_communications, cpt, action)
	lmm_elem_set_value(maxmin_system, cpu->constraint,
			   action->variable,
			   cpu->power_current * cpu->power_scale *
			   ROUTE(action->src->id,
				 action->dst->id).
			   impact_on_src_with_other_recv);
    xbt_dynar_foreach(cpu->incomming_communications, cpt, action)
	lmm_elem_set_value(maxmin_system, cpu->constraint,
			   action->variable,
			   cpu->power_current * cpu->power_scale *
			   ROUTE(action->src->id,
				 action->dst->id).
			   impact_on_dst_with_other_send);
  }
}

/**************************************/
/******* Resource Public     **********/
/**************************************/

static void *name_service(const char *name)
{
  xbt_ex_t e;
  void *res = NULL;

  TRY {
    res = xbt_dict_get(workstation_set, name);
  } CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    WARN1("Host '%s' not found, verifing if it is a router", name);
    res = NULL;
    xbt_ex_free(e);
  }

  return res;
}

static const char *get_model_name(void *model_id)
{
  /* We can freely cast as a cpu_KCCFLN05_t because it has the same
     prefix as network_link_KCCFLN05_t. However, only cpu_KCCFLN05_t
     will theoretically be given as an argument here. */
  return ((cpu_KCCFLN05_t) model_id)->name;
}

/* action_get_state is inherited from the surf module */

static void action_use(surf_action_t action)
{
  action->using++;
  return;
}

static int action_free(surf_action_t action)
{
  int cpt;
  surf_action_t act = NULL;
  cpu_KCCFLN05_t src = ((surf_action_workstation_KCCFLN05_t) action)->src;
  cpu_KCCFLN05_t dst = ((surf_action_workstation_KCCFLN05_t) action)->dst;

  action->using--;
  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_workstation_KCCFLN05_t) action)->variable)
      lmm_variable_free(maxmin_system,
			((surf_action_workstation_KCCFLN05_t) action)->
			variable);
    if (src)
      xbt_dynar_foreach(src->outgoing_communications, cpt, act)
	  if (act == action) {
	xbt_dynar_remove_at(src->outgoing_communications, cpt, &act);
	break;
      }

    if (dst)
      xbt_dynar_foreach(dst->incomming_communications, cpt, act)
	  if (act == action) {
	xbt_dynar_remove_at(dst->incomming_communications, cpt, &act);
	break;
      }

    if (src && (!xbt_dynar_length(src->outgoing_communications)))
      __update_cpu_usage(src);
    if (dst && (!xbt_dynar_length(dst->incomming_communications)))
      __update_cpu_usage(dst);

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
  if (((surf_action_workstation_KCCFLN05_t) action)->suspended != 2) {
    ((surf_action_workstation_KCCFLN05_t) action)->suspended = 1;
    lmm_update_variable_weight(maxmin_system,
			       ((surf_action_workstation_KCCFLN05_t)
				action)->variable, 0.0);
  }
  XBT_OUT;
}

static void action_resume(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_workstation_KCCFLN05_t) action)->suspended != 2) {
    if (((surf_action_workstation_KCCFLN05_t) action)->lat_current == 0.0)
      lmm_update_variable_weight(maxmin_system,
				 ((surf_action_workstation_KCCFLN05_t)
				  action)->variable, 1.0);
    else
      lmm_update_variable_weight(maxmin_system,
				 ((surf_action_workstation_KCCFLN05_t)
				  action)->variable,
				 ((surf_action_workstation_KCCFLN05_t)
				  action)->lat_current);

    ((surf_action_workstation_KCCFLN05_t) action)->suspended = 0;
  }
  XBT_OUT;
}

static int action_is_suspended(surf_action_t action)
{
  return (((surf_action_workstation_KCCFLN05_t) action)->suspended == 1);
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

static int model_used(void *model_id)
{
  /* We can freely cast as a network_link_KCCFLN05_t because it has
     the same prefix as cpu_KCCFLN05_t */
  if (((cpu_KCCFLN05_t) model_id)->type ==
      SURF_WORKSTATION_RESOURCE_CPU)
    return (lmm_constraint_used
	    (maxmin_system, ((cpu_KCCFLN05_t) model_id)->constraint)
	    || ((((cpu_KCCFLN05_t) model_id)->bus) ?
		lmm_constraint_used(maxmin_system,
				    ((cpu_KCCFLN05_t) model_id)->
				    bus) : 0));
  else
    return lmm_constraint_used(maxmin_system,
			       ((network_link_KCCFLN05_t) model_id)->
			       constraint);

}

static double share_models(double now)
{
  s_surf_action_workstation_KCCFLN05_t s_action;
  surf_action_workstation_KCCFLN05_t action = NULL;

  xbt_swag_t running_actions =
      surf_workstation_model->common_public->states.running_action_set;
  double min = generic_maxmin_share_models(running_actions,
					      xbt_swag_offset(s_action,
							      variable));

  xbt_swag_foreach(action, running_actions) {
    if (action->latency > 0) {
      if (min < 0) {
	min = action->latency;
	DEBUG3("Updating min (value) with %p (start %f): %f", action,
	       action->generic_action.start, min);
      } else if (action->latency < min) {
	min = action->latency;
	DEBUG3("Updating min (latency) with %p (start %f): %f", action,
	       action->generic_action.start, min);
      }
    }
  }

  DEBUG1("min value : %f", min);

  return min;
}

static void update_actions_state(double now, double delta)
{
  double deltap = 0.0;
  surf_action_workstation_KCCFLN05_t action = NULL;
  surf_action_workstation_KCCFLN05_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_workstation_model->common_public->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    deltap = delta;
    if (action->latency > 0) {
      if (action->latency > deltap) {
	double_update(&(action->latency), deltap);
	deltap = 0.0;
      } else {
	double_update(&(deltap), action->latency);
	action->latency = 0.0;
      }
      if ((action->latency == 0.0) && (action->suspended == 0)) {
	if ((action)->lat_current == 0.0)
	  lmm_update_variable_weight(maxmin_system, action->variable, 1.0);
	else
	  lmm_update_variable_weight(maxmin_system, action->variable,
				     action->lat_current);
      }
    }
    DEBUG3("Action (%p) : remains (%g) updated by %g.",
	   action, action->generic_action.remains,
	   lmm_variable_getvalue(action->variable) * deltap);
    double_update(&(action->generic_action.remains),
		  lmm_variable_getvalue(action->variable) * deltap);

    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    if ((action->generic_action.remains <= 0) &&
	(lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {
      /* Need to check that none of the model has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      void *constraint_id = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	constraint_id = lmm_constraint_id(cnst);

/* 	if(((network_link_KCCFLN05_t)constraint_id)->type== */
/* 	   SURF_WORKSTATION_RESOURCE_LINK) { */
/* 	  DEBUG2("Checking for link %s (%p)", */
/* 		 ((network_link_KCCFLN05_t)constraint_id)->name, */
/* 		 ((network_link_KCCFLN05_t)constraint_id)); */
/* 	} */
/* 	if(((cpu_KCCFLN05_t)constraint_id)->type== */
/* 	   SURF_WORKSTATION_RESOURCE_CPU) { */
/* 	  DEBUG3("Checking for cpu %s (%p) : %s", */
/* 		 ((cpu_KCCFLN05_t)constraint_id)->name, */
/* 		 ((cpu_KCCFLN05_t)constraint_id), */
/* 		 ((cpu_KCCFLN05_t)constraint_id)->state_current==SURF_CPU_OFF?"Off":"On"); */
/* 	} */

	if (((((network_link_KCCFLN05_t) constraint_id)->type ==
	      SURF_WORKSTATION_RESOURCE_LINK) &&
	     (((network_link_KCCFLN05_t) constraint_id)->state_current ==
	      SURF_NETWORK_LINK_OFF)) ||
	    ((((cpu_KCCFLN05_t) constraint_id)->type ==
	      SURF_WORKSTATION_RESOURCE_CPU) &&
	     (((cpu_KCCFLN05_t) constraint_id)->state_current ==
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

static void update_model_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  cpu_KCCFLN05_t cpu = id;
  network_link_KCCFLN05_t nw_link = id;

  if (nw_link->type == SURF_WORKSTATION_RESOURCE_LINK) {
    DEBUG2("Updating link %s (%p)", nw_link->name, nw_link);
    if (event_type == nw_link->bw_event) {
      nw_link->bw_current = value;
      lmm_update_constraint_bound(maxmin_system, nw_link->constraint,
				  nw_link->bw_current);
    } else if (event_type == nw_link->lat_event) {
      double delta = value - nw_link->lat_current;
      lmm_variable_t var = NULL;
      surf_action_workstation_KCCFLN05_t action = NULL;

      nw_link->lat_current = value;
      while (lmm_get_var_from_cnst
	     (maxmin_system, nw_link->constraint, &var)) {
	action = lmm_variable_id(var);
	action->lat_current += delta;
	if (action->rate < 0)
	  lmm_update_variable_bound(maxmin_system, action->variable,
				    SG_TCP_CTE_GAMMA / (2.0 *
							action->
							lat_current));
	else
	  lmm_update_variable_bound(maxmin_system, action->variable,
				    min(action->rate,
					SG_TCP_CTE_GAMMA / (2.0 *
							    action->
							    lat_current)));
	if (action->suspended == 0)
	  lmm_update_variable_weight(maxmin_system, action->variable,
				     action->lat_current);
	lmm_update_variable_latency(maxmin_system, action->variable,
				    delta);


      }
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
      __update_cpu_usage(cpu);
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
  xbt_dict_free(&router_set);
  if (parallel_task_network_link_set != NULL) {
    xbt_dict_free(&parallel_task_network_link_set);
  }
  xbt_swag_free(surf_workstation_model->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_workstation_model->common_public->states.
		running_action_set);
  xbt_swag_free(surf_workstation_model->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_workstation_model->common_public->states.
		done_action_set);

  free(surf_workstation_model->common_public);
  free(surf_workstation_model->common_private);
  free(surf_workstation_model->extension_public);

  free(surf_workstation_model);
  surf_workstation_model = NULL;

  for (i = 0; i < nb_workstation; i++)
    for (j = 0; j < nb_workstation; j++)
      free(ROUTE(i, j).links);
  free(routing_table);
  routing_table = NULL;
  nb_workstation = 0;

  if (maxmin_system) {
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
}

/**************************************/
/******* Resource Private    **********/
/**************************************/

static surf_action_t execute(void *cpu, double size)
{
  surf_action_workstation_KCCFLN05_t action = NULL;
  cpu_KCCFLN05_t CPU = cpu;

  XBT_IN2("(%s,%g)", CPU->name, size);
  action = xbt_new0(s_surf_action_workstation_KCCFLN05_t, 1);

  action->generic_action.using = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.priority = 1.0;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type =
      (surf_model_t) surf_workstation_model;
  action->suspended = 0;

  if (CPU->state_current == SURF_CPU_ON)
    action->generic_action.state_set =
	surf_workstation_model->common_public->states.
	running_action_set;
  else
    action->generic_action.state_set =
	surf_workstation_model->common_public->states.failed_action_set;
  xbt_swag_insert(action, action->generic_action.state_set);

  action->variable = lmm_variable_new(maxmin_system, action,
				      action->generic_action.priority,
				      -1.0, 1);
  lmm_expand(maxmin_system, CPU->constraint, action->variable, 1.0);
  XBT_OUT;
  return (surf_action_t) action;
}

static surf_action_t action_sleep(void *cpu, double duration)
{
  surf_action_workstation_KCCFLN05_t action = NULL;

  XBT_IN2("(%s,%g)", ((cpu_KCCFLN05_t) cpu)->name, duration);

  action = (surf_action_workstation_KCCFLN05_t) execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  action->suspended = 2;
  lmm_update_variable_weight(maxmin_system, action->variable, 0.0);

  XBT_OUT;
  return (surf_action_t) action;
}

static e_surf_cpu_state_t model_get_state(void *cpu)
{
  return ((cpu_KCCFLN05_t) cpu)->state_current;
}

static double get_speed(void *cpu, double load)
{
  return load * (((cpu_KCCFLN05_t) cpu)->power_scale);
}

static double get_available_speed(void *cpu)
{
  return ((cpu_KCCFLN05_t) cpu)->power_current;
}


static surf_action_t communicate(void *src, void *dst, double size,
				 double rate)
{
  surf_action_workstation_KCCFLN05_t action = NULL;
  cpu_KCCFLN05_t card_src = src;
  cpu_KCCFLN05_t card_dst = dst;
  route_KCCFLN05_t route = &(ROUTE(card_src->id, card_dst->id));
  int route_size = route->size;
  int i;

  XBT_IN4("(%s,%s,%g,%g)", card_src->name, card_dst->name, size, rate);
  xbt_assert2(route_size,
	      "You're trying to send data from %s to %s but there is no connexion between these two cards.",
	      card_src->name, card_dst->name);

  action = xbt_new0(s_surf_action_workstation_KCCFLN05_t, 1);

  action->generic_action.using = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->src = src;
  action->dst = dst;
  action->generic_action.model_type =
      (surf_model_t) surf_workstation_model;
  action->suspended = 0;	/* Should be useless because of the 
				   calloc but it seems to help valgrind... */
  action->generic_action.state_set =
      surf_workstation_model->common_public->states.running_action_set;

  xbt_dynar_push(card_src->outgoing_communications, &action);
  xbt_dynar_push(card_dst->incomming_communications, &action);

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  action->latency = 0.0;
  for (i = 0; i < route_size; i++)
    action->latency += route->links[i]->lat_current;
  action->lat_current = action->latency;

  if (action->latency > 0)
    action->variable = lmm_variable_new(maxmin_system, action, 0.0, -1.0, route_size + 4);	/* +1 for the src bus
												   +1 for the dst bus
												   +1 for the src cpu
												   +1 for the dst cpu */
  else
    action->variable = lmm_variable_new(maxmin_system, action, 1.0, -1.0,
					route_size + 4);

  if (action->rate < 0) {
    if (action->lat_current > 0)
      lmm_update_variable_bound(maxmin_system, action->variable,
				SG_TCP_CTE_GAMMA / (2.0 *
						    action->lat_current));
    else
      lmm_update_variable_bound(maxmin_system, action->variable, -1.0);
  } else {
    if (action->lat_current > 0)
      lmm_update_variable_bound(maxmin_system, action->variable,
				min(action->rate,
				    SG_TCP_CTE_GAMMA / (2.0 *
							action->
							lat_current)));
    else
      lmm_update_variable_bound(maxmin_system, action->variable,
				action->rate);
  }

  lmm_update_variable_latency(maxmin_system, action->variable,
			      action->latency);

  for (i = 0; i < route_size; i++)
    lmm_expand(maxmin_system, route->links[i]->constraint,
	       action->variable, 1.0);
  if (card_src->bus)
    lmm_expand(maxmin_system, card_src->bus, action->variable, 1.0);
  if (card_dst->bus)
    lmm_expand(maxmin_system, card_dst->bus, action->variable, 1.0);
  lmm_expand(maxmin_system, card_src->constraint, action->variable, 0.0);
  lmm_expand(maxmin_system, card_dst->constraint, action->variable, 0.0);

  XBT_OUT;
  return (surf_action_t) action;
}

static surf_action_t execute_parallel_task(int workstation_nb,
					   void **workstation_list,
					   double *computation_amount,
					   double *communication_amount,
					   double amount, double rate)
{
  surf_action_workstation_KCCFLN05_t action = NULL;
  int i, j, k;
  int nb_link = 0;
  int nb_host = 0;

  if (parallel_task_network_link_set == NULL) {
    parallel_task_network_link_set =
	xbt_dict_new_ext(workstation_nb * workstation_nb * 10);
  }

  /* Compute the number of affected models... */
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_KCCFLN05_t card_src = workstation_list[i];
      cpu_KCCFLN05_t card_dst = workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      network_link_KCCFLN05_t *route =
	  ROUTE(card_src->id, card_dst->id).links;

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

  action = xbt_new0(s_surf_action_workstation_KCCFLN05_t, 1);
  DEBUG3("Creating a parallel task (%p) with %d cpus and %d links.",
	 action, nb_host, nb_link);
  action->generic_action.using = 1;
  action->generic_action.cost = amount;
  action->generic_action.remains = amount;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
  action->generic_action.model_type =
      (surf_model_t) surf_workstation_model;
  action->suspended = 0;	/* Should be useless because of the
				   calloc but it seems to help valgrind... */
  action->generic_action.state_set =
      surf_workstation_model->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  if (action->rate > 0)
    action->variable = lmm_variable_new(maxmin_system, action, 1.0, -1.0,
					nb_host + nb_link);
  else
    action->variable =
	lmm_variable_new(maxmin_system, action, 1.0, action->rate,
			 nb_host + nb_link);

  for (i = 0; i < workstation_nb; i++)
    if (computation_amount[i] > 0)
      lmm_expand(maxmin_system,
		 ((cpu_KCCFLN05_t) workstation_list[i])->constraint,
		 action->variable, computation_amount[i]);

  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_KCCFLN05_t card_src = workstation_list[i];
      cpu_KCCFLN05_t card_dst = workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      network_link_KCCFLN05_t *route =
	  ROUTE(card_src->id, card_dst->id).links;

      for (k = 0; k < route_size; k++) {
	if (communication_amount[i * workstation_nb + j] > 0) {
	  lmm_expand_add(maxmin_system, route[k]->constraint,
			 action->variable,
			 communication_amount[i * workstation_nb + j]);
	}
      }
    }
  }

  if (nb_link + nb_host == 0) {
    action->generic_action.cost = 1.0;
    action->generic_action.remains = 0.0;
  }

  return (surf_action_t) action;
}

/* returns an array of network_link_KCCFLN05_t */
static const void **get_route(void *src, void *dst)
{
  cpu_KCCFLN05_t card_src = src;
  cpu_KCCFLN05_t card_dst = dst;
  route_KCCFLN05_t route = &(ROUTE(card_src->id, card_dst->id));

  return (const void **) route->links;
}

static int get_route_size(void *src, void *dst)
{
  cpu_KCCFLN05_t card_src = src;
  cpu_KCCFLN05_t card_dst = dst;
  route_KCCFLN05_t route = &(ROUTE(card_src->id, card_dst->id));
  return route->size;
}

static const char *get_link_name(const void *link)
{
  return ((network_link_KCCFLN05_t) link)->name;
}

static double get_link_bandwidth(const void *link)
{
  return ((network_link_KCCFLN05_t) link)->bw_current;
}

static double get_link_latency(const void *link)
{
  return ((network_link_KCCFLN05_t) link)->lat_current;
}

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/


static void router_free(void *router)
{
  free(((router_KCCFLN05_t) router)->name);
}

static void router_new(const char *name)
{
  static unsigned int nb_routers = 0;

  INFO1("Creating a router %s", name);

  router_KCCFLN05_t router;
  router = xbt_new0(s_router_KCCFLN05_t, 1);

  router->name = xbt_strdup(name);
  router->id = nb_routers++;
  xbt_dict_set(router_set, name, router, router_free);
}

static void parse_routers(void)
{
  //add a dumb router just to be GTNETS compatible
  router_new(A_surfxml_router_name);
}

static void cpu_free(void *cpu)
{
  free(((cpu_KCCFLN05_t) cpu)->name);
  xbt_dynar_free(&(((cpu_KCCFLN05_t) cpu)->incomming_communications));
  xbt_dynar_free(&(((cpu_KCCFLN05_t) cpu)->outgoing_communications));
  free(cpu);
}

static cpu_KCCFLN05_t cpu_new(const char *name, double power_scale,
			      double power_initial,
			      tmgr_trace_t power_trace,
			      e_surf_cpu_state_t state_initial,
			      tmgr_trace_t state_trace,
			      double interference_send,
			      double interference_recv,
			      double interference_send_recv,
			      double max_outgoing_rate)
{
  cpu_KCCFLN05_t cpu = xbt_new0(s_cpu_KCCFLN05_t, 1);

  cpu->model = (surf_model_t) surf_workstation_model;
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

  cpu->interference_send = interference_send;
  cpu->interference_recv = interference_recv;
  cpu->interference_send_recv = interference_send_recv;

  cpu->constraint =
      lmm_constraint_new(maxmin_system, cpu,
			 cpu->power_current * cpu->power_scale);
  if (max_outgoing_rate > 0)
    cpu->bus = lmm_constraint_new(maxmin_system, cpu, max_outgoing_rate);

  cpu->incomming_communications =
      xbt_dynar_new(sizeof(surf_action_workstation_KCCFLN05_t), NULL);
  cpu->outgoing_communications =
      xbt_dynar_new(sizeof(surf_action_workstation_KCCFLN05_t), NULL);

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
  double interference_send = 0.0;
  double interference_recv = 0.0;
  double interference_send_recv = 0.0;
  double max_outgoing_rate = -1.0;

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

  surf_parse_get_double(&interference_send,
			A_surfxml_cpu_interference_send);
  surf_parse_get_double(&interference_recv,
			A_surfxml_cpu_interference_recv);
  surf_parse_get_double(&interference_send_recv,
			A_surfxml_cpu_interference_send_recv);
  surf_parse_get_double(&max_outgoing_rate,
			A_surfxml_cpu_max_outgoing_rate);

  cpu_new(A_surfxml_cpu_name, power_scale, power_initial, power_trace,
	  state_initial, state_trace, interference_send, interference_recv,
	  interference_send_recv, max_outgoing_rate);
}

static void create_routing_table(void)
{
  routing_table =
      xbt_new0(s_route_KCCFLN05_t, nb_workstation * nb_workstation);
}

static void network_link_free(void *nw_link)
{
  free(((network_link_KCCFLN05_t) nw_link)->name);
  free(nw_link);
}

static network_link_KCCFLN05_t network_link_new(char *name,
						double bw_initial,
						tmgr_trace_t bw_trace,
						double lat_initial,
						tmgr_trace_t lat_trace,
						e_surf_network_link_state_t
						state_initial,
						tmgr_trace_t state_trace,
						e_surf_network_link_sharing_policy_t
						policy)
{
  network_link_KCCFLN05_t nw_link = xbt_new0(s_network_link_KCCFLN05_t, 1);


  nw_link->model = (surf_model_t) surf_workstation_model;
  nw_link->type = SURF_WORKSTATION_RESOURCE_LINK;
  nw_link->name = name;
  nw_link->bw_current = bw_initial;
  if (bw_trace)
    nw_link->bw_event =
	tmgr_history_add_trace(history, bw_trace, 0.0, 0, nw_link);
  nw_link->state_current = state_initial;
  nw_link->lat_current = lat_initial;
  if (lat_trace)
    nw_link->lat_event =
	tmgr_history_add_trace(history, lat_trace, 0.0, 0, nw_link);
  if (state_trace)
    nw_link->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, nw_link);

  nw_link->constraint =
      lmm_constraint_new(maxmin_system, nw_link, nw_link->bw_current);

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
  double lat_initial;
  tmgr_trace_t lat_trace;
  e_surf_network_link_state_t state_initial = SURF_NETWORK_LINK_ON;
  e_surf_network_link_sharing_policy_t policy_initial =
      SURF_NETWORK_LINK_SHARED;
  tmgr_trace_t state_trace;

  name = xbt_strdup(A_surfxml_network_link_name);
  surf_parse_get_double(&bw_initial, A_surfxml_network_link_bandwidth);
  surf_parse_get_trace(&bw_trace, A_surfxml_network_link_bandwidth_file);
  surf_parse_get_double(&lat_initial, A_surfxml_network_link_latency);
  surf_parse_get_trace(&lat_trace, A_surfxml_network_link_latency_file);

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

  network_link_new(name, bw_initial, bw_trace,
		   lat_initial, lat_trace, state_initial, state_trace,
		   policy_initial);
}

static void route_new(int src_id, int dst_id,
		      network_link_KCCFLN05_t * link_list, int nb_link,
		      double impact_on_src, double impact_on_dst,
		      double impact_on_src_with_other_recv,
		      double impact_on_dst_with_other_send)
{
  route_KCCFLN05_t route = &(ROUTE(src_id, dst_id));

  route->size = nb_link;
  route->links = link_list =
      xbt_realloc(link_list, sizeof(network_link_KCCFLN05_t) * nb_link);
  route->impact_on_src = impact_on_src;
  route->impact_on_dst = impact_on_src;
  route->impact_on_src_with_other_recv = impact_on_src_with_other_recv;
  route->impact_on_dst_with_other_send = impact_on_dst_with_other_send;
}

static int nb_link;
static int link_list_capacity;
static network_link_KCCFLN05_t *link_list = NULL;
static int src_id = -1;
static int dst_id = -1;
static double impact_on_src;
static double impact_on_dst;
static double impact_on_src_with_other_recv;
static double impact_on_dst_with_other_send;

static void parse_route_set_endpoints(void)
{
  cpu_KCCFLN05_t cpu_tmp = NULL;

  cpu_tmp = (cpu_KCCFLN05_t) name_service(A_surfxml_route_src);
  if (cpu_tmp != NULL) {
    src_id = cpu_tmp->id;
  } else {
    xbt_assert1(xbt_dict_get_or_null(router_set, A_surfxml_route_src),
		"Invalid name '%s': neither a cpu nor a router!",
		A_surfxml_route_src);
    src_id = -1;
    return;
  }

  cpu_tmp = (cpu_KCCFLN05_t) name_service(A_surfxml_route_dst);
  if (cpu_tmp != NULL) {
    dst_id = cpu_tmp->id;
  } else {
    xbt_assert1(xbt_dict_get_or_null(router_set, A_surfxml_route_dst),
		"Invalid name '%s': neither a cpu nor a router!",
		A_surfxml_route_dst);
    dst_id = -1;
    return;
  }

  surf_parse_get_double(&impact_on_src, A_surfxml_route_impact_on_src);
  surf_parse_get_double(&impact_on_dst, A_surfxml_route_impact_on_dst);
  surf_parse_get_double(&impact_on_src_with_other_recv,
			A_surfxml_route_impact_on_src_with_other_recv);
  surf_parse_get_double(&impact_on_dst_with_other_send,
			A_surfxml_route_impact_on_dst_with_other_send);

  nb_link = 0;
  link_list_capacity = 1;
  link_list = xbt_new(network_link_KCCFLN05_t, link_list_capacity);

}

static void parse_route_elem(void)
{
  xbt_ex_t e;
  if (nb_link == link_list_capacity) {
    link_list_capacity *= 2;
    link_list =
	xbt_realloc(link_list,
		    (link_list_capacity) *
		    sizeof(network_link_KCCFLN05_t));
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
    route_new(src_id, dst_id, link_list, nb_link, impact_on_src,
	      impact_on_dst, impact_on_src_with_other_recv,
	      impact_on_dst_with_other_send);
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

  /* Figuring out the router (added after GTNETS) */
  surf_parse_reset_parser();
  STag_surfxml_router_fun = parse_routers;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();

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
				    498000000, NULL, 0.000015, NULL,
				    SURF_NETWORK_LINK_ON, NULL,
				    SURF_NETWORK_LINK_FATPIPE);
      ROUTE(i, i).size = 1;
      ROUTE(i, i).links = xbt_new0(network_link_KCCFLN05_t, 1);
      ROUTE(i, i).links[0] = loopback;
    }

}

/**************************************/
/********* Module  creation ***********/
/**************************************/

static void model_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_model = xbt_new0(s_surf_workstation_model_t, 1);

  surf_workstation_model->common_private =
      xbt_new0(s_surf_model_private_t, 1);
  surf_workstation_model->common_public =
      xbt_new0(s_surf_model_public_t, 1);
  surf_workstation_model->extension_public =
      xbt_new0(s_surf_workstation_model_extension_public_t, 1);

  surf_workstation_model->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_model->common_public->name_service = name_service;
  surf_workstation_model->common_public->get_model_name =
      get_model_name;
  surf_workstation_model->common_public->action_get_state =
      surf_action_get_state;
  surf_workstation_model->common_public->action_get_start_time =
      surf_action_get_start_time;
  surf_workstation_model->common_public->action_get_finish_time =
      surf_action_get_finish_time;
  surf_workstation_model->common_public->action_use = action_use;
  surf_workstation_model->common_public->action_free = action_free;
  surf_workstation_model->common_public->action_cancel = action_cancel;
  surf_workstation_model->common_public->action_recycle =
      action_recycle;
  surf_workstation_model->common_public->action_change_state =
      surf_action_change_state;
  surf_workstation_model->common_public->action_set_data =
      surf_action_set_data;
  surf_workstation_model->common_public->suspend = action_suspend;
  surf_workstation_model->common_public->resume = action_resume;
  surf_workstation_model->common_public->is_suspended =
      action_is_suspended;
  surf_workstation_model->common_public->set_max_duration =
      action_set_max_duration;
  surf_workstation_model->common_public->set_priority =
      action_set_priority;
  surf_workstation_model->common_public->name = "Workstation KCCFLN05";

  surf_workstation_model->common_private->model_used = model_used;
  surf_workstation_model->common_private->share_models =
      share_models;
  surf_workstation_model->common_private->update_actions_state =
      update_actions_state;
  surf_workstation_model->common_private->update_model_state =
      update_model_state;
  surf_workstation_model->common_private->finalize = finalize;

  surf_workstation_model->extension_public->execute = execute;
  surf_workstation_model->extension_public->sleep = action_sleep;
  surf_workstation_model->extension_public->get_state =
      model_get_state;
  surf_workstation_model->extension_public->get_speed = get_speed;
  surf_workstation_model->extension_public->get_available_speed =
      get_available_speed;
  surf_workstation_model->extension_public->communicate = communicate;
  surf_workstation_model->extension_public->execute_parallel_task =
      execute_parallel_task;
  surf_workstation_model->extension_public->get_route = get_route;
  surf_workstation_model->extension_public->get_route_size =
      get_route_size;
  surf_workstation_model->extension_public->get_link_name =
      get_link_name;
  surf_workstation_model->extension_public->get_link_bandwidth =
      get_link_bandwidth;
  surf_workstation_model->extension_public->get_link_latency =
      get_link_latency;

  workstation_set = xbt_dict_new();
  router_set = xbt_dict_new();
  network_link_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

/**************************************/
/*************** Generic **************/
/**************************************/
void surf_workstation_model_init_KCCFLN05(const char *filename)
{
  xbt_assert0(!surf_cpu_model, "CPU model type already defined");
  xbt_assert0(!surf_network_model,
	      "network model type already defined");
  model_init_internal();
  parse_file(filename);

  xbt_dynar_push(model_list, &surf_workstation_model);
}
