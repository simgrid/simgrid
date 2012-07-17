/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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
#include "instr/instr_private.h"
#include "surf/surfxml_parse.h"
#ifdef HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

#ifdef HAVE_JEDULE
#include "instr/jedule/jedule_sd_binding.h"
#endif

XBT_LOG_NEW_CATEGORY(sd, "Logging specific to SimDag");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_kernel, sd,
                                "Logging specific to SimDag (kernel)");

SD_global_t sd_global = NULL;

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
#ifdef HAVE_TRACING
  TRACE_global_init(argc, argv);
#endif

  s_SD_task_t task;

  xbt_assert(sd_global == NULL, "SD_init() already called");

  sd_global = xbt_new(s_SD_global_t, 1);
  sd_global->workstation_list = NULL;
  sd_global->link_list = NULL;
  sd_global->recyclable_route = NULL;
  sd_global->watch_point_reached = 0;

  sd_global->task_mallocator=xbt_mallocator_new(65536, SD_task_new_f,SD_task_free_f,SD_task_recycle_f);

  sd_global->not_scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->schedulable_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->runnable_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->in_fifo_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->running_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->done_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->failed_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->return_set =
      xbt_swag_new(xbt_swag_offset(task, return_hookup));
  sd_global->task_number = 0;

  surf_init(argc, argv);

  xbt_cfg_setdefault_string(_surf_cfg_set, "workstation/model",
                            "ptask_L07");

#ifdef HAVE_TRACING
  TRACE_start ();
#endif

#ifdef HAVE_JEDULE
  jedule_sd_init();
#endif

  XBT_DEBUG("ADD SD LEVELS");
  SD_HOST_LEVEL = xbt_lib_add_level(host_lib,__SD_workstation_destroy);
  SD_LINK_LEVEL = xbt_lib_add_level(link_lib,__SD_link_destroy);
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

  XBT_DEBUG("Recreating the swags...");
  xbt_swag_free(sd_global->not_scheduled_task_set);
  xbt_swag_free(sd_global->schedulable_task_set);
  xbt_swag_free(sd_global->scheduled_task_set);
  xbt_swag_free(sd_global->runnable_task_set);
  xbt_swag_free(sd_global->in_fifo_task_set);
  xbt_swag_free(sd_global->running_task_set);
  xbt_swag_free(sd_global->done_task_set);
  xbt_swag_free(sd_global->failed_task_set);

  sd_global->not_scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->schedulable_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->runnable_task_set =
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

#ifdef HAVE_JEDULE
  jedule_sd_cleanup();
  jedule_sd_init();
#endif
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
  xbt_lib_cursor_t cursor = NULL;
  char *name = NULL;
  void **surf_workstation = NULL;
  void **surf_link = NULL;

  parse_platform_file(platform_file);

  /* now let's create the SD wrappers for workstations and links */
  xbt_lib_foreach(host_lib, cursor, name, surf_workstation){
    if(surf_workstation[SURF_WKS_LEVEL])
      __SD_workstation_create(surf_workstation[SURF_WKS_LEVEL], NULL);
  }

  xbt_lib_foreach(link_lib, cursor, name, surf_link) {
  if(surf_link[SURF_LINK_LEVEL])
    __SD_link_create(surf_link[SURF_LINK_LEVEL], NULL);
  }

  XBT_DEBUG("Workstation number: %d, link number: %d",
         SD_workstation_get_number(), SD_link_get_number());
#ifdef HAVE_JEDULE
  jedule_setup_platform();
#endif
}

/**
 * \brief Launches the simulation.
 *
 * The function will execute the \ref SD_RUNNABLE runnable tasks.
 * If \a how_long is positive, then the simulation will be stopped either
 * when time reaches \a how_long or when a watch point is reached.
 * A nonpositive value for \a how_long means no time limit, in which case
 * the simulation will be stopped either when a watch point is reached or
 * when no more task can be executed.
 * Then you can call SD_simulate() again.
 *
 * \param how_long maximum duration of the simulation (a negative value means no time limit)
 * \return a dynar of \ref SD_task_t whose state has changed.
 * \see SD_task_schedule(), SD_task_watch()
 */

xbt_dynar_t SD_simulate(double how_long) {
  xbt_dynar_t changed_tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);
  SD_task_t task;

  SD_simulate_swag(how_long);
  while( (task = xbt_swag_extract(sd_global->return_set)) != NULL) {
    xbt_dynar_push(changed_tasks, &task);
  }

  return changed_tasks;
}

xbt_swag_t SD_simulate_swag(double how_long) {
  double total_time = 0.0;      /* we stop the simulation when total_time >= how_long */
  double elapsed_time = 0.0;
  SD_task_t task, task_safe, dst;
  SD_dependency_t dependency;
  surf_action_t action;
  unsigned int iter, depcnt;
  static int first_time = 1;

   if (first_time) {
     XBT_VERB("Starting simulation...");

     surf_presolve();            /* Takes traces into account */
     first_time = 0;
   }

  sd_global->watch_point_reached = 0;

  xbt_swag_reset(sd_global->return_set);

  /* explore the runnable tasks */
  xbt_swag_foreach_safe(task, task_safe, sd_global->runnable_task_set) {
    XBT_VERB("Executing task '%s'", SD_task_get_name(task));
    if (__SD_task_try_to_run(task))
      xbt_swag_insert(task,sd_global->return_set);
  }

  /* main loop */
  elapsed_time = 0.0;
  while (elapsed_time >= 0.0 &&
         (how_long < 0.0 || total_time < how_long) &&
         !sd_global->watch_point_reached) {
    surf_model_t model = NULL;
    /* dumb variables */


    XBT_DEBUG("Total time: %f", total_time);

    elapsed_time = surf_solve(how_long > 0 ? surf_get_clock() + how_long - total_time: -1.0);
    XBT_DEBUG("surf_solve() returns %f", elapsed_time);
    if (elapsed_time > 0.0)
      total_time += elapsed_time;

    /* let's see which tasks are done */
    xbt_dynar_foreach(model_list, iter, model) {
      while ((action = xbt_swag_extract(model->states.done_action_set))) {
        task = action->data;
        task->start_time =
            surf_workstation_model->
            action_get_start_time(task->surf_action);
        task->finish_time = surf_get_clock();
        XBT_VERB("Task '%s' done", SD_task_get_name(task));
        XBT_DEBUG("Calling __SD_task_just_done");
        __SD_task_just_done(task);
        XBT_DEBUG("__SD_task_just_done called on task '%s'",
               SD_task_get_name(task));

        /* the state has changed */
        xbt_swag_insert(task,sd_global->return_set);

        /* remove the dependencies after this task */
        xbt_dynar_foreach(task->tasks_after, depcnt, dependency) {
          dst = dependency->dst;
          if (dst->unsatisfied_dependencies > 0)
            dst->unsatisfied_dependencies--;
          if (dst->is_not_ready > 0)
            dst->is_not_ready--;

          if (!(dst->unsatisfied_dependencies)) {
            if (__SD_task_is_scheduled(dst))
              __SD_task_set_state(dst, SD_RUNNABLE);
            else
              __SD_task_set_state(dst, SD_SCHEDULABLE);
          }

          if (SD_task_get_kind(dst) == SD_TASK_COMM_E2E) {
            SD_dependency_t comm_dep;
            SD_task_t comm_dst;
            xbt_dynar_get_cpy(dst->tasks_after, 0, &comm_dep);
            comm_dst = comm_dep->dst;
            if (__SD_task_is_not_scheduled(comm_dst) &&
                comm_dst->is_not_ready > 0) {
              comm_dst->is_not_ready--;

              if (!(comm_dst->is_not_ready)) {
                __SD_task_set_state(comm_dst, SD_SCHEDULABLE);
              }
            }
          }

          /* is dst runnable now? */
          if (__SD_task_is_runnable(dst)
              && !sd_global->watch_point_reached) {
            XBT_VERB("Executing task '%s'", SD_task_get_name(dst));
            if (__SD_task_try_to_run(dst))
              xbt_swag_insert(dst,sd_global->return_set);
          }
        }
      }

      /* let's see which tasks have just failed */
      while ((action = xbt_swag_extract(model->states.failed_action_set))) {
        task = action->data;
        task->start_time =
            surf_workstation_model->
            action_get_start_time(task->surf_action);
        task->finish_time = surf_get_clock();
        XBT_VERB("Task '%s' failed", SD_task_get_name(task));
        __SD_task_set_state(task, SD_FAILED);
        surf_workstation_model->action_unref(action);
        task->surf_action = NULL;

        xbt_swag_insert(task,sd_global->return_set);
      }
    }
  }

  if (!sd_global->watch_point_reached && how_long<0){
    if (xbt_swag_size(sd_global->done_task_set) < sd_global->task_number){
        XBT_WARN("Simulation is finished but %d tasks are still not done",
            (sd_global->task_number - xbt_swag_size(sd_global->done_task_set)));
        xbt_swag_foreach_safe (task, task_safe,sd_global->not_scheduled_task_set){
                XBT_WARN("%s is in SD_NOT_SCHEDULED state", SD_task_get_name(task));
    }
      xbt_swag_foreach_safe (task, task_safe,sd_global->schedulable_task_set){
                XBT_WARN("%s is in SD_SCHEDULABLE state", SD_task_get_name(task));
  }
        xbt_swag_foreach_safe (task, task_safe,sd_global->scheduled_task_set){
                XBT_WARN("%s is in SD_SCHEDULED state", SD_task_get_name(task));
         }
    }
  }

  XBT_DEBUG("elapsed_time = %f, total_time = %f, watch_point_reached = %d",
         elapsed_time, total_time, sd_global->watch_point_reached);
  XBT_DEBUG("current time = %f", surf_get_clock());

  return sd_global->return_set;
}

/**
 * \brief Returns the current clock
 *
 * \return the current clock, in second
 */
double SD_get_clock(void) {
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
#ifdef HAVE_TRACING
  TRACE_surf_release();
#endif

  xbt_mallocator_free(sd_global->task_mallocator);

  XBT_DEBUG("Destroying workstation and link arrays...");
  xbt_free(sd_global->workstation_list);
  xbt_free(sd_global->link_list);
  xbt_free(sd_global->recyclable_route);

  XBT_DEBUG("Destroying the swags...");
  xbt_swag_free(sd_global->not_scheduled_task_set);
  xbt_swag_free(sd_global->schedulable_task_set);
  xbt_swag_free(sd_global->scheduled_task_set);
  xbt_swag_free(sd_global->runnable_task_set);
  xbt_swag_free(sd_global->in_fifo_task_set);
  xbt_swag_free(sd_global->running_task_set);
  xbt_swag_free(sd_global->done_task_set);
  xbt_swag_free(sd_global->failed_task_set);
  xbt_swag_free(sd_global->return_set);

#ifdef HAVE_TRACING
  TRACE_end();
#endif

  XBT_DEBUG("Exiting Surf...");
  surf_exit();

  xbt_free(sd_global);
  sd_global = NULL;

#ifdef HAVE_JEDULE
  jedule_sd_dump();
  jedule_sd_cleanup();
#endif
}

/**
 * \brief load script file
 */

void SD_load_environment_script(const char *script_file)
{
#ifdef HAVE_LUA
  lua_State *L = lua_open();
  luaL_openlibs(L);

  if (luaL_loadfile(L, script_file) || lua_pcall(L, 0, 0, 0)) {
    printf("error: %s\n", lua_tostring(L, -1));
    return;
  }
#else
  xbt_die
      ("Lua is not available!! to call SD_load_environment_script, lua should be available...");
#endif
  return;
}
