/* Copyright (c) 2007-2009 Da SimGrid Team.  All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "surf/surf.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/config.h"

XBT_LOG_NEW_CATEGORY(sd, "Logging specific to SimDag");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_kernel, sd,
                                "Logging specific to SimDag (kernel)");

SD_global_t sd_global = NULL;

XBT_LOG_EXTERNAL_CATEGORY(sd_kernel);
XBT_LOG_EXTERNAL_CATEGORY(sd_task);
XBT_LOG_EXTERNAL_CATEGORY(sd_workstation);

/**
 * \brief Initialises SD internal data
 *
 * This function must be called before any other SD function. Then you
 * should call SD_create_environment().
 *
 * \param argc argument number
 * \param argv argument list
 * \see SD_create_environment(), SD_exit()
 */
void SD_init(int *argc, char **argv)
{

  s_SD_task_t task;

  xbt_assert0(!SD_INITIALISED(), "SD_init() already called");

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(sd_kernel, sd);
  XBT_LOG_CONNECT(sd_task, sd);
  XBT_LOG_CONNECT(sd_workstation, sd);


  sd_global = xbt_new(s_SD_global_t, 1);
  sd_global->workstations = xbt_dict_new();
  sd_global->workstation_count = 0;
  sd_global->workstation_list = NULL;
  sd_global->links = xbt_dict_new();
  sd_global->link_count = 0;
  sd_global->link_list = NULL;
  sd_global->recyclable_route = NULL;
  sd_global->watch_point_reached = 0;

  sd_global->not_scheduled_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->scheduled_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->ready_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->in_fifo_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->running_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->done_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->failed_task_set =
    xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->task_number = 0;

  surf_init(argc, argv);
  xbt_cfg_set_string(_surf_cfg_set, "workstation_model", "ptask_L07");
}

/**
 * \brief Reinits the application part of the simulation (experimental feature)
 *
 * This function allows you to run several simulations on the same platform
 * by resetting the part describing the application.
 *
 * @warning: this function is still experimental and not perfect. For example,
 * the simulation clock (and traces usage) is not reset. So, do not use it if
 * you use traces in your simulation, and do not use absolute timing after using it.
 * That being said, this function is still precious if you want to compare a bunch of
 * heuristics on the same platforms.
 */
void SD_application_reinit(void)
{

  s_SD_task_t task;

  if (SD_INITIALISED()) {
    DEBUG0("Recreating the swags...");
    xbt_swag_free(sd_global->not_scheduled_task_set);
    xbt_swag_free(sd_global->scheduled_task_set);
    xbt_swag_free(sd_global->ready_task_set);
    xbt_swag_free(sd_global->in_fifo_task_set);
    xbt_swag_free(sd_global->running_task_set);
    xbt_swag_free(sd_global->done_task_set);
    xbt_swag_free(sd_global->failed_task_set);

    sd_global->not_scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->ready_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->in_fifo_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->running_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->done_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->failed_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->task_number = 0;
  } else {
    WARN0("SD_application_reinit called before initialization of SimDag");
    /* we cannot use exceptions here because xbt is not running! */
  }

}

/**
 * \brief Creates the environment
 *
 * The environment (i.e. the \ref SD_workstation_management "workstations" and the
 * \ref SD_link_management "links") is created with the data stored in the given XML
 * platform file.
 *
 * \param platform_file name of an XML file describing the environment to create
 * \see SD_workstation_management, SD_link_management
 *
 * The XML file follows this DTD:
 *
 *     \include simgrid.dtd
 *
 * Here is a small example of such a platform:
 *
 *     \include small_platform.xml
 */
void SD_create_environment(const char *platform_file)
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *surf_workstation = NULL;
  void *surf_link = NULL;

  SD_CHECK_INIT_DONE();

  DEBUG0("SD_create_environment");

  surf_config_models_setup(platform_file);

  parse_platform_file(platform_file);

  /* now let's create the SD wrappers for workstations and links */
  xbt_dict_foreach(surf_model_resource_set(surf_workstation_model), cursor,
                   name, surf_workstation) {
    __SD_workstation_create(surf_workstation, NULL);
  }

  xbt_dict_foreach(surf_model_resource_set(surf_network_model), cursor, name, surf_link) {
    __SD_link_create(surf_link, NULL);
  }

  DEBUG2("Workstation number: %d, link number: %d",
         SD_workstation_get_number(), SD_link_get_number());
}

/**
 * \brief Launches the simulation.
 *
 * The function will execute the \ref SD_READY ready tasks.
 * The simulation will be stopped when its time reaches \a how_long,
 * when a watch point is reached, or when no more task can be executed.
 * Then you can call SD_simulate() again.
 *
 * \param how_long maximum duration of the simulation (a negative value means no time limit)
 * \return a NULL-terminated array of \ref SD_task_t whose state has changed.
 * \see SD_task_schedule(), SD_task_watch()
 */
SD_task_t *SD_simulate(double how_long)
{
  double total_time = 0.0;      /* we stop the simulation when total_time >= how_long */
  double elapsed_time = 0.0;
  SD_task_t task, task_safe, dst;
  SD_dependency_t dependency;
  surf_action_t action;
  SD_task_t *res = NULL;
  xbt_dynar_t changed_tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);
  unsigned int iter;
  static int first_time = 1;

  SD_CHECK_INIT_DONE();

  INFO0("Starting simulation...");

  if (first_time) {
    surf_presolve();            /* Takes traces into account */
    first_time = 0;
  }

  if (how_long > 0) {
    surf_timer_model->extension.timer.set(surf_get_clock() + how_long,
                                          NULL, NULL);
  }
  sd_global->watch_point_reached = 0;

  /* explore the ready tasks */
  xbt_swag_foreach_safe(task, task_safe, sd_global->ready_task_set) {
    INFO1("Executing task '%s'", SD_task_get_name(task));
    if (__SD_task_try_to_run(task) && !xbt_dynar_member(changed_tasks, &task))
      xbt_dynar_push(changed_tasks, &task);
  }

  /* main loop */
  elapsed_time = 0.0;
  while (elapsed_time >= 0.0 &&
         (how_long < 0.0 || total_time < how_long) &&
         !sd_global->watch_point_reached) {
    surf_model_t model = NULL;
    /* dumb variables */
    void *fun = NULL;
    void *arg = NULL;


    DEBUG1("Total time: %f", total_time);

    elapsed_time = surf_solve();
    DEBUG1("surf_solve() returns %f", elapsed_time);
    if (elapsed_time > 0.0)
      total_time += elapsed_time;

    /* let's see which tasks are done */
    xbt_dynar_foreach(model_list, iter, model) {
      while ((action = xbt_swag_extract(model->states.done_action_set))) {
        task = action->data;
        INFO1("Task '%s' done", SD_task_get_name(task));
        DEBUG0("Calling __SD_task_just_done");
        __SD_task_just_done(task);
        DEBUG1("__SD_task_just_done called on task '%s'",
               SD_task_get_name(task));

        /* the state has changed */
        if (!xbt_dynar_member(changed_tasks, &task))
          xbt_dynar_push(changed_tasks, &task);

        /* remove the dependencies after this task */
        while (xbt_dynar_length(task->tasks_after) > 0) {
          xbt_dynar_get_cpy(task->tasks_after, 0, &dependency);
          dst = dependency->dst;
          SD_task_dependency_remove(task, dst);

          /* is dst ready now? */
          if (__SD_task_is_ready(dst) && !sd_global->watch_point_reached) {
            INFO1("Executing task '%s'", SD_task_get_name(dst));
            if (__SD_task_try_to_run(dst) &&
                !xbt_dynar_member(changed_tasks, &task))
              xbt_dynar_push(changed_tasks, &task);
          }
        }
      }

      /* let's see which tasks have just failed */
      while ((action = xbt_swag_extract(model->states.failed_action_set))) {
        task = action->data;
        INFO1("Task '%s' failed", SD_task_get_name(task));
        __SD_task_set_state(task, SD_FAILED);
        surf_workstation_model->action_unref(action);
        task->surf_action = NULL;

        if (!xbt_dynar_member(changed_tasks, &task))
          xbt_dynar_push(changed_tasks, &task);
      }
    }

    while (surf_timer_model->extension.timer.get(&fun, (void *) &arg)) {
    }
  }

  res = xbt_new0(SD_task_t, (xbt_dynar_length(changed_tasks) + 1));

  xbt_dynar_foreach(changed_tasks, iter, task) {
    res[iter] = task;
  }
  xbt_dynar_free(&changed_tasks);

  INFO0("Simulation finished");
  DEBUG3("elapsed_time = %f, total_time = %f, watch_point_reached = %d",
         elapsed_time, total_time, sd_global->watch_point_reached);
  DEBUG1("current time = %f", surf_get_clock());

  return res;
}

/**
 * \brief Returns the current clock
 *
 * \return the current clock, in second
 */
double SD_get_clock(void)
{
  SD_CHECK_INIT_DONE();

  return surf_get_clock();
}

/**
 * \brief Destroys all SD internal data
 *
 * This function should be called when the simulation is over. Don't forget also to destroy
 * the tasks.
 *
 * \see SD_init(), SD_task_destroy()
 */
void SD_exit(void)
{
  if (SD_INITIALISED()) {
    DEBUG0("Destroying workstation and link dictionaries...");
    xbt_dict_free(&sd_global->workstations);
    xbt_dict_free(&sd_global->links);

    DEBUG0("Destroying workstation and link arrays if necessary...");
    if (sd_global->workstation_list != NULL)
      xbt_free(sd_global->workstation_list);

    if (sd_global->link_list != NULL)
      xbt_free(sd_global->link_list);

    if (sd_global->recyclable_route != NULL)
      xbt_free(sd_global->recyclable_route);

    DEBUG0("Destroying the swags...");
    xbt_swag_free(sd_global->not_scheduled_task_set);
    xbt_swag_free(sd_global->scheduled_task_set);
    xbt_swag_free(sd_global->ready_task_set);
    xbt_swag_free(sd_global->in_fifo_task_set);
    xbt_swag_free(sd_global->running_task_set);
    xbt_swag_free(sd_global->done_task_set);
    xbt_swag_free(sd_global->failed_task_set);

    xbt_free(sd_global);
    sd_global = NULL;

    DEBUG0("Exiting Surf...");
    surf_exit();
  } else {
    WARN0("SD_exit() called, but SimDag is not running");
    /* we cannot use exceptions here because xbt is not running! */
  }
}
