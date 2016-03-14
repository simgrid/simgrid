/* Copyright (c) 2006-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_interface.h"
#include "simgrid/sg_config.h"
#include "simgrid/host.h"
#include "src/simdag/simdag_private.h"
#include "src/surf/surf_interface.hpp"
#include "simgrid/s4u/engine.hpp"

#if HAVE_JEDULE
#include "simgrid/jedule/jedule_sd_binding.h"
#endif

XBT_LOG_NEW_CATEGORY(sd, "Logging specific to SimDag");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_kernel, sd, "Logging specific to SimDag (kernel)");

SD_global_t sd_global = NULL;

/**
 * \brief Initializes SD internal data
 *
 * This function must be called before any other SD function. Then you should call SD_create_environment().
 *
 * \param argc argument number
 * \param argv argument list
 * \see SD_create_environment(), SD_exit()
 */
void SD_init(int *argc, char **argv)
{
  TRACE_global_init(argc, argv);

  xbt_assert(sd_global == NULL, "SD_init() already called");

  sd_global = xbt_new(s_SD_global_t, 1);
  sd_global->watch_point_reached = 0;

  sd_global->task_mallocator=xbt_mallocator_new(65536, SD_task_new_f, SD_task_free_f, SD_task_recycle_f);

  sd_global->initial_task_set = xbt_dynar_new(sizeof(SD_task_t), NULL);
  sd_global->executable_task_set = xbt_dynar_new(sizeof(SD_task_t), NULL);
  sd_global->completed_task_set = xbt_dynar_new(sizeof(SD_task_t), NULL);
  sd_global->return_set = xbt_dynar_new(sizeof(SD_task_t), NULL);

  surf_init(argc, argv);

  xbt_cfg_setdefault_string(_sg_cfg_set, "host/model", "ptask_L07");

#if HAVE_JEDULE
  jedule_sd_init();
#endif

  if (_sg_cfg_exit_asap) {
    SD_exit();
    exit(0);
  }
}

/** \brief set a configuration variable
 *
 * Do --help on any simgrid binary to see the list of currently existing configuration variables, and
 * see Section @ref options.
 *
 * Example:
 * SD_config("host/model","default");
 */
void SD_config(const char *key, const char *value){
  xbt_assert(sd_global,"ERROR: Please call SD_init() before using SD_config()");
  xbt_cfg_set_as_string(_sg_cfg_set, key, value);
}

/**
 * \brief Creates the environment
 *
 * The environment (i.e. the \ref sg_host_management "hosts" and the \ref SD_link_management "links") is created with
 * the data stored in the given XML platform file.
 *
 * \param platform_file name of an XML file describing the environment to create
 * \see sg_host_management, SD_link_management
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
  simgrid::s4u::Engine::instance()->loadPlatform(platform_file);

  XBT_DEBUG("Workstation number: %zu, link number: %d", sg_host_count(), sg_link_count());
#if HAVE_JEDULE
  jedule_setup_platform();
#endif
  XBT_VERB("Starting simulation...");
  surf_presolve();            /* Takes traces into account */
}

/**
 * \brief Launches the simulation.
 *
 * The function will execute the \ref SD_RUNNABLE runnable tasks.
 * If \a how_long is positive, then the simulation will be stopped either when time reaches \a how_long or when a watch
 * point is reached.
 * A non-positive value for \a how_long means no time limit, in which case the simulation will be stopped either when a
 * watch point is reached or when no more task can be executed.
 * Then you can call SD_simulate() again.
 *
 * \param how_long maximum duration of the simulation (a negative value means no time limit)
 * \return a dynar of \ref SD_task_t whose state has changed.
 * \see SD_task_schedule(), SD_task_watch()
 */

xbt_dynar_t SD_simulate(double how_long) {
  /* we stop the simulation when total_time >= how_long */
  double total_time = 0.0;
  SD_task_t task, dst;
  SD_dependency_t dependency;
  surf_action_t action;
  unsigned int iter, depcnt;

  XBT_VERB("Run simulation for %f seconds", how_long);
  sd_global->watch_point_reached = 0;

  xbt_dynar_reset(sd_global->return_set);

  /* explore the runnable tasks */
  xbt_dynar_foreach(sd_global->executable_task_set , iter, task) {
    XBT_VERB("Executing task '%s'", SD_task_get_name(task));
    SD_task_run(task);
    xbt_dynar_push(sd_global->return_set, &task);
    iter--;
  }

  /* main loop */
  double elapsed_time = 0.0;
  while (elapsed_time >= 0.0 && (how_long < 0.0 || 0.00001 < (how_long -total_time)) &&
         !sd_global->watch_point_reached) {
    surf_model_t model = NULL;

    XBT_DEBUG("Total time: %f", total_time);

    elapsed_time = surf_solve(how_long > 0 ? surf_get_clock() + how_long - total_time: -1.0);
    XBT_DEBUG("surf_solve() returns %f", elapsed_time);
    if (elapsed_time > 0.0)
      total_time += elapsed_time;

    /* let's see which tasks are done */
    xbt_dynar_foreach(all_existing_models, iter, model) {
      while ((action = surf_model_extract_done_action_set(model))) {
        task = (SD_task_t) action->getData();
        task->start_time = task->surf_action->getStartTime();

        task->finish_time = surf_get_clock();
        XBT_VERB("Task '%s' done", SD_task_get_name(task));
        SD_task_set_state(task, SD_DONE);
        task->surf_action->unref();
        task->surf_action = NULL;

        /* the state has changed. Add it only if it's the first change */
        if (!xbt_dynar_member(sd_global->return_set, &task)) {
          xbt_dynar_push(sd_global->return_set, &task);
        }

        /* remove the dependencies after this task */
        xbt_dynar_foreach(task->tasks_after, depcnt, dependency) {
          dst = dependency->dst;
          dst->unsatisfied_dependencies--;
          if (dst->is_not_ready > 0)
            dst->is_not_ready--;

          XBT_DEBUG("Released a dependency on %s: %d remain(s). Became schedulable if %d=0",
             SD_task_get_name(dst), dst->unsatisfied_dependencies, dst->is_not_ready);

          if (!(dst->unsatisfied_dependencies)) {
            if (SD_task_get_state(dst) == SD_SCHEDULED)
              SD_task_set_state(dst, SD_RUNNABLE);
            else
              SD_task_set_state(dst, SD_SCHEDULABLE);
          }

          if (SD_task_get_state(dst) == SD_NOT_SCHEDULED && !(dst->is_not_ready)) {
            SD_task_set_state(dst, SD_SCHEDULABLE);
          }

          if (SD_task_get_kind(dst) == SD_TASK_COMM_E2E) {
            SD_dependency_t comm_dep;
            SD_task_t comm_dst;
            xbt_dynar_get_cpy(dst->tasks_after, 0, &comm_dep);
            comm_dst = comm_dep->dst;
            if (SD_task_get_state(comm_dst) == SD_NOT_SCHEDULED && comm_dst->is_not_ready > 0) {
              comm_dst->is_not_ready--;

            XBT_DEBUG("%s is a transfer, %s may be ready now if %d=0",
               SD_task_get_name(dst), SD_task_get_name(comm_dst), comm_dst->is_not_ready);

              if (!(comm_dst->is_not_ready)) {
                SD_task_set_state(comm_dst, SD_SCHEDULABLE);
              }
            }
          }

          /* is dst runnable now? */
          if (SD_task_get_state(dst) == SD_RUNNABLE && !sd_global->watch_point_reached) {
            XBT_VERB("Executing task '%s'", SD_task_get_name(dst));
            SD_task_run(dst);
            xbt_dynar_push(sd_global->return_set, &dst);
          }
        }
      }

      /* let's see which tasks have just failed */
      while ((action = surf_model_extract_failed_action_set(model))) {
        task = (SD_task_t) action->getData();
        task->start_time = task->surf_action->getStartTime();
        task->finish_time = surf_get_clock();
        XBT_VERB("Task '%s' failed", SD_task_get_name(task));
        SD_task_set_state(task, SD_FAILED);
        action->unref();
        task->surf_action = NULL;

        xbt_dynar_push(sd_global->return_set, &task);
      }
    }
  }

  if (!sd_global->watch_point_reached && how_long<0){
    if (!xbt_dynar_is_empty(sd_global->initial_task_set)) {
        XBT_WARN("Simulation is finished but %lu tasks are still not done",
            xbt_dynar_length(sd_global->initial_task_set));
        static const char* state_names[] =
          { "SD_NOT_SCHEDULED", "SD_SCHEDULABLE", "SD_SCHEDULED", "SD_RUNNABLE", "SD_RUNNING", "SD_DONE","SD_FAILED" };
        xbt_dynar_foreach(sd_global->initial_task_set, iter, task){
          XBT_WARN("%s is in %s state", SD_task_get_name(task), state_names[SD_task_get_state(task)]);
        }
    }
  }

  XBT_DEBUG("elapsed_time = %f, total_time = %f, watch_point_reached = %d",
         elapsed_time, total_time, sd_global->watch_point_reached);
  XBT_DEBUG("current time = %f", surf_get_clock());

  return sd_global->return_set;
}

/** @brief Returns the current clock, in seconds */
double SD_get_clock(void) {
  return surf_get_clock();
}

/**
 * \brief Destroys all SD internal data
 *
 * This function should be called when the simulation is over. Don't forget to destroy too.
 *
 * \see SD_init(), SD_task_destroy()
 */
void SD_exit(void)
{
  TRACE_surf_resource_utilization_release();

#if HAVE_JEDULE
  jedule_sd_cleanup();
  jedule_sd_exit();
#endif

  xbt_mallocator_free(sd_global->task_mallocator);
  xbt_dynar_free_container(&(sd_global->initial_task_set));
  xbt_dynar_free_container(&(sd_global->executable_task_set));
  xbt_dynar_free_container(&(sd_global->completed_task_set));
  xbt_dynar_free_container(&(sd_global->return_set));
  xbt_free(sd_global);
  sd_global = NULL;
}
