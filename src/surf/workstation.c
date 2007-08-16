/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "gras_config.h"
#include "workstation_private.h"
#include "cpu_private.h"
#include "network_private.h"

surf_workstation_model_t surf_workstation_model = NULL;
xbt_dict_t workstation_set = NULL;
static xbt_dict_t parallel_task_network_link_set = NULL;

static workstation_CLM03_t workstation_new(const char *name,
					   void *cpu, void *card)
{
  workstation_CLM03_t workstation = xbt_new0(s_workstation_CLM03_t, 1);

  workstation->model = (surf_model_t) surf_workstation_model;
  workstation->name = xbt_strdup(name);
  workstation->cpu = cpu;
  workstation->network_card = card;

  return workstation;
}

static void workstation_free(void *workstation)
{
  free(((workstation_CLM03_t) workstation)->name);
  free(workstation);
}

static void create_workstations(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *cpu = NULL;
  void *nw_card = NULL;

  xbt_dict_foreach(cpu_set, cursor, name, cpu) {
    nw_card = xbt_dict_get_or_null(network_card_set, name);
    xbt_assert1(nw_card, "No corresponding card found for %s", name);

    xbt_dict_set(workstation_set, name,
		 workstation_new(name, cpu, nw_card), workstation_free);
  }
}

static void *name_service(const char *name)
{
  return xbt_dict_get_or_null(workstation_set, name);
}

static const char *get_model_name(void *model_id)
{
  return ((workstation_CLM03_t) model_id)->name;
}

static int model_used(void *model_id)
{
  xbt_assert0(0,
	      "Workstation is a virtual model. I should not be there!");
  return 0;
}

static int parallel_action_free(surf_action_t action)
{
  action->using--;
  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_parallel_task_CSL05_t) action)->variable)
      lmm_variable_free(maxmin_system,
			((surf_action_parallel_task_CSL05_t) action)->
			variable);
    free(action);
    return 1;
  }
  return 0;
}

static void parallel_action_use(surf_action_t action)
{
  action->using++;
}

static int action_free(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    return surf_network_model->common_public->action_free(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    return surf_cpu_model->common_public->action_free(action);
  else if (action->model_type ==
	   (surf_model_t) surf_workstation_model)
    return parallel_action_free(action);
  else
    DIE_IMPOSSIBLE;
  return 0;
}

static void action_use(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->action_use(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->action_use(action);
  else if (action->model_type ==
	   (surf_model_t) surf_workstation_model)
    parallel_action_use(action);
  else
    DIE_IMPOSSIBLE;
  return;
}

static void action_cancel(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->action_cancel(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->action_cancel(action);
  else if (action->model_type ==
	   (surf_model_t) surf_workstation_model)
    parallel_action_use(action);
  else
    DIE_IMPOSSIBLE;
  return;
}

static void action_recycle(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->action_change_state(action,
							      state);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->action_change_state(action, state);
  else if (action->model_type ==
	   (surf_model_t) surf_workstation_model)
    surf_action_change_state(action, state);
  else
    DIE_IMPOSSIBLE;
  return;
}

static double share_models(double now)
{
  s_surf_action_parallel_task_CSL05_t action;
  return generic_maxmin_share_models(surf_workstation_model->
					common_public->states.
					running_action_set,
					xbt_swag_offset(action, variable));
}

static void update_actions_state(double now, double delta)
{
  surf_action_parallel_task_CSL05_t action = NULL;
  surf_action_parallel_task_CSL05_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_workstation_model->common_public->states.running_action_set;
  /* FIXME: unused
     xbt_swag_t failed_actions =
     surf_workstation_model->common_public->states.failed_action_set;
   */

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    double_update(&(action->generic_action.remains),
		  lmm_variable_getvalue(action->variable) * delta);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);
    if ((action->generic_action.remains <= 0) &&
	(lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the model has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      surf_model_t model = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	model = (surf_model_t) lmm_constraint_id(cnst);
	if (model == (surf_model_t) surf_cpu_model) {
	  cpu_Cas01_t cpu = lmm_constraint_id(cnst);
	  if (cpu->state_current == SURF_CPU_OFF) {
	    action->generic_action.finish = surf_get_clock();
	    action_change_state((surf_action_t) action,
				SURF_ACTION_FAILED);
	    break;
	  }
	} else if (model == (surf_model_t) surf_network_model) {
	  network_link_CM02_t nw_link = lmm_constraint_id(cnst);

	  if (nw_link->state_current == SURF_NETWORK_LINK_OFF) {
	    action->generic_action.finish = surf_get_clock();
	    action_change_state((surf_action_t) action,
				SURF_ACTION_FAILED);
	    break;
	  }
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
  return;
}

static surf_action_t execute(void *workstation, double size)
{
  return surf_cpu_model->extension_public->
      execute(((workstation_CLM03_t) workstation)->cpu, size);
}

static surf_action_t action_sleep(void *workstation, double duration)
{
  return surf_cpu_model->extension_public->
      sleep(((workstation_CLM03_t) workstation)->cpu, duration);
}

static void action_suspend(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->suspend(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->suspend(action);
  else
    DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->resume(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->resume(action);
  else
    DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    return surf_network_model->common_public->is_suspended(action);
  if (action->model_type == (surf_model_t) surf_cpu_model)
    return surf_cpu_model->common_public->is_suspended(action);
  DIE_IMPOSSIBLE;
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->set_max_duration(action,
							   duration);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->set_max_duration(action, duration);
  else
    DIE_IMPOSSIBLE;
}

static void action_set_priority(surf_action_t action, double priority)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->set_priority(action, priority);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->set_priority(action, priority);
  else
    DIE_IMPOSSIBLE;
}

static surf_action_t communicate(void *workstation_src,
				 void *workstation_dst, double size,
				 double rate)
{
  return surf_network_model->extension_public->
      communicate(((workstation_CLM03_t) workstation_src)->network_card,
		  ((workstation_CLM03_t) workstation_dst)->network_card,
		  size, rate);
}

static e_surf_cpu_state_t get_state(void *workstation)
{
  return surf_cpu_model->extension_public->
      get_state(((workstation_CLM03_t) workstation)->cpu);
}

static double get_speed(void *workstation, double load)
{
  return surf_cpu_model->extension_public->
      get_speed(((workstation_CLM03_t) workstation)->cpu, load);
}

static double get_available_speed(void *workstation)
{
  return surf_cpu_model->extension_public->
      get_available_speed(((workstation_CLM03_t) workstation)->cpu);
}

static surf_action_t execute_parallel_task_bogus(int workstation_nb,
						 void **workstation_list,
						 double
						 *computation_amount,
						 double
						 *communication_amount,
						 double amount,
						 double rate)
{
  xbt_assert0(0, "This model does not implement parallel tasks");
}

static surf_action_t execute_parallel_task(int workstation_nb,
					   void **workstation_list,
					   double *computation_amount,
					   double *communication_amount,
					   double amount, double rate)
{
  surf_action_parallel_task_CSL05_t action = NULL;
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
      network_card_CM02_t card_src =
	  ((workstation_CLM03_t *) workstation_list)[i]->network_card;
      network_card_CM02_t card_dst =
	  ((workstation_CLM03_t *) workstation_list)[j]->network_card;
      int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
      network_link_CM02_t *route = ROUTE(card_src->id, card_dst->id);

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

  if (nb_link + workstation_nb == 0)
    return NULL;

  action = xbt_new0(s_surf_action_parallel_task_CSL05_t, 1);
  action->generic_action.using = 1;
  action->generic_action.cost = amount;
  action->generic_action.remains = amount;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
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
		 ((cpu_Cas01_t)
		  ((workstation_CLM03_t) workstation_list[i])->cpu)->
		 constraint, action->variable, computation_amount[i]);

  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      network_card_CM02_t card_src =
	  ((workstation_CLM03_t *) workstation_list)[i]->network_card;
      network_card_CM02_t card_dst =
	  ((workstation_CLM03_t *) workstation_list)[j]->network_card;
      int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
      network_link_CM02_t *route = ROUTE(card_src->id, card_dst->id);

      for (k = 0; k < route_size; k++) {
	if (communication_amount[i * workstation_nb + j] > 0) {
	  lmm_expand_add(maxmin_system, route[k]->constraint,
			 action->variable,
			 communication_amount[i * workstation_nb + j]);
	}
      }
    }
  }

  return (surf_action_t) action;
}

/* returns an array of network_link_CM02_t */
static const void **get_route(void *src, void *dst)
{
  workstation_CLM03_t workstation_src = (workstation_CLM03_t) src;
  workstation_CLM03_t workstation_dst = (workstation_CLM03_t) dst;
  return surf_network_model->extension_public->
      get_route(workstation_src->network_card,
		workstation_dst->network_card);
}

static int get_route_size(void *src, void *dst)
{
  workstation_CLM03_t workstation_src = (workstation_CLM03_t) src;
  workstation_CLM03_t workstation_dst = (workstation_CLM03_t) dst;
  return surf_network_model->extension_public->
      get_route_size(workstation_src->network_card,
		     workstation_dst->network_card);
}

static const char *get_link_name(const void *link)
{
  return surf_network_model->extension_public->get_link_name(link);
}

static double get_link_bandwidth(const void *link)
{
  return surf_network_model->extension_public->get_link_bandwidth(link);
}

static double get_link_latency(const void *link)
{
  return surf_network_model->extension_public->get_link_latency(link);
}

static void finalize(void)
{
  xbt_dict_free(&workstation_set);
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
}

static void surf_workstation_model_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_model = xbt_new0(s_surf_workstation_model_t, 1);

  surf_workstation_model->common_private =
      xbt_new0(s_surf_model_private_t, 1);
  surf_workstation_model->common_public =
      xbt_new0(s_surf_model_public_t, 1);
/*   surf_workstation_model->extension_private = xbt_new0(s_surf_workstation_model_extension_private_t,1); */
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
  surf_workstation_model->common_public->action_free = action_free;
  surf_workstation_model->common_public->action_use = action_use;
  surf_workstation_model->common_public->action_cancel = action_cancel;
  surf_workstation_model->common_public->action_recycle =
      action_recycle;
  surf_workstation_model->common_public->action_change_state =
      action_change_state;
  surf_workstation_model->common_public->action_set_data =
      surf_action_set_data;
  surf_workstation_model->common_public->name = "Workstation";

  surf_workstation_model->common_private->model_used = model_used;
  surf_workstation_model->common_private->share_models =
      share_models;
  surf_workstation_model->common_private->update_actions_state =
      update_actions_state;
  surf_workstation_model->common_private->update_model_state =
      update_model_state;
  surf_workstation_model->common_private->finalize = finalize;

  surf_workstation_model->common_public->suspend = action_suspend;
  surf_workstation_model->common_public->resume = action_resume;
  surf_workstation_model->common_public->is_suspended =
      action_is_suspended;
  surf_workstation_model->common_public->set_max_duration =
      action_set_max_duration;
  surf_workstation_model->common_public->set_priority =
      action_set_priority;

  surf_workstation_model->extension_public->execute = execute;
  surf_workstation_model->extension_public->sleep = action_sleep;
  surf_workstation_model->extension_public->get_state = get_state;
  surf_workstation_model->extension_public->get_speed = get_speed;
  surf_workstation_model->extension_public->get_available_speed =
      get_available_speed;
  surf_workstation_model->extension_public->communicate = communicate;
  surf_workstation_model->extension_public->execute_parallel_task =
      execute_parallel_task_bogus;
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

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

/********************************************************************/
/* The model used in MSG and presented at CCGrid03                  */
/********************************************************************/
/* @InProceedings{Casanova.CLM_03, */
/*   author = {Henri Casanova and Arnaud Legrand and Loris Marchal}, */
/*   title = {Scheduling Distributed Applications: the SimGrid Simulation Framework}, */
/*   booktitle = {Proceedings of the third IEEE International Symposium on Cluster Computing and the Grid (CCGrid'03)}, */
/*   publisher = {"IEEE Computer Society Press"}, */
/*   month = {may}, */
/*   year = {2003} */
/* } */
void surf_workstation_model_init_CLM03(const char *filename)
{
  surf_workstation_model_init_internal();
  surf_cpu_model_init_Cas01(filename);
  surf_network_model_init_CM02(filename);
  create_workstations();
  update_model_description(surf_workstation_model_description,
			      surf_workstation_model_description_size,
			      "CLM03",
			      (surf_model_t) surf_workstation_model);
  xbt_dynar_push(model_list, &surf_workstation_model);
}

void surf_workstation_model_init_compound(const char *filename)
{

  xbt_assert0(surf_cpu_model, "No CPU model defined yet!");
  xbt_assert0(surf_network_model, "No network model defined yet!");
  surf_workstation_model_init_internal();
  create_workstations();

  update_model_description(surf_workstation_model_description,
			      surf_workstation_model_description_size,
			      "compound",
			      (surf_model_t) surf_workstation_model);

  xbt_dynar_push(model_list, &surf_workstation_model);
}
