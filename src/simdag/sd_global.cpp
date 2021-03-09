/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simdag_private.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/surf_interface.hpp"

#include <array>

XBT_LOG_NEW_CATEGORY(sd, "Logging specific to SimDag");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_kernel, sd, "Logging specific to SimDag (kernel)");

simgrid::sd::Global* sd_global = nullptr;

namespace simgrid {
namespace sd {

std::set<SD_task_t>* simulate(double how_long)
{
  XBT_VERB("Run simulation for %f seconds", how_long);

  sd_global->watch_point_reached = false;
  sd_global->return_set.clear();

  /* explore the runnable tasks */
  while (not sd_global->runnable_tasks.empty())
    SD_task_run(*(sd_global->runnable_tasks.begin()));

  double elapsed_time = 0.0;
  double total_time   = 0.0;
  /* main loop */
  while (elapsed_time >= 0 && (how_long < 0 || 0.00001 < (how_long - total_time)) &&
         not sd_global->watch_point_reached) {
    XBT_DEBUG("Total time: %f", total_time);

    elapsed_time = surf_solve(how_long > 0 ? surf_get_clock() + how_long - total_time : -1.0);
    XBT_DEBUG("surf_solve() returns %f", elapsed_time);
    if (elapsed_time > 0.0)
      total_time += elapsed_time;

    /* let's see which tasks are done */
    for (auto const& model : simgrid::kernel::EngineImpl::get_instance()->get_all_models()) {
      const simgrid::kernel::resource::Action* action = model->extract_done_action();
      while (action != nullptr && action->get_data() != nullptr) {
        auto* task = static_cast<SD_task_t>(action->get_data());
        XBT_VERB("Task '%s' done", SD_task_get_name(task));
        SD_task_set_state(task, SD_DONE);

        /* the state has changed. Add it only if it's the first change */
        sd_global->return_set.emplace(task);

        /* remove the dependencies after this task */
        for (auto const& succ : *task->successors) {
          succ->predecessors->erase(task);
          succ->inputs->erase(task);
          XBT_DEBUG("Release dependency on %s: %zu remain(s). Becomes schedulable if %zu=0", SD_task_get_name(succ),
                    succ->predecessors->size() + succ->inputs->size(), succ->predecessors->size());

          if (SD_task_get_state(succ) == SD_NOT_SCHEDULED && succ->predecessors->empty())
            SD_task_set_state(succ, SD_SCHEDULABLE);

          if (SD_task_get_state(succ) == SD_SCHEDULED && succ->predecessors->empty() && succ->inputs->empty())
            SD_task_set_state(succ, SD_RUNNABLE);

          if (SD_task_get_state(succ) == SD_RUNNABLE && not sd_global->watch_point_reached)
            SD_task_run(succ);
        }
        task->successors->clear();

        for (auto const& output : *task->outputs) {
          output->start_time = task->finish_time;
          output->predecessors->erase(task);
          if (SD_task_get_state(output) == SD_SCHEDULED)
            SD_task_set_state(output, SD_RUNNABLE);
          else
            SD_task_set_state(output, SD_SCHEDULABLE);

          SD_task_t comm_dst = *(output->successors->begin());
          if (SD_task_get_state(comm_dst) == SD_NOT_SCHEDULED && comm_dst->predecessors->empty()) {
            XBT_DEBUG("%s is a transfer, %s may be ready now if %zu=0", SD_task_get_name(output),
                      SD_task_get_name(comm_dst), comm_dst->predecessors->size());
            SD_task_set_state(comm_dst, SD_SCHEDULABLE);
          }
          if (SD_task_get_state(output) == SD_RUNNABLE && not sd_global->watch_point_reached)
            SD_task_run(output);
        }
        task->outputs->clear();
        action = model->extract_done_action();
      }

      /* let's see which tasks have just failed */
      action = model->extract_failed_action();
      while (action != nullptr) {
        auto* task = static_cast<SD_task_t>(action->get_data());
        XBT_VERB("Task '%s' failed", SD_task_get_name(task));
        SD_task_set_state(task, SD_FAILED);
        sd_global->return_set.insert(task);
        action = model->extract_failed_action();
      }
    }
  }

  if (not sd_global->watch_point_reached && how_long < 0 && not sd_global->initial_tasks.empty()) {
    XBT_WARN("Simulation is finished but %zu tasks are still not done", sd_global->initial_tasks.size());
    for (auto const& t : sd_global->initial_tasks)
      XBT_WARN("%s is in %s state", SD_task_get_name(t), __get_state_name(SD_task_get_state(t)));
  }

  XBT_DEBUG("elapsed_time = %f, total_time = %f, watch_point_reached = %d", elapsed_time, total_time,
            sd_global->watch_point_reached);
  XBT_DEBUG("current time = %f", surf_get_clock());

  return &sd_global->return_set;
}
} // namespace sd
} // namespace simgrid

/**
 * @brief helper for pretty printing of task state
 * @param state the state of a task
 * @return the equivalent as a readable string
 */
const char* __get_state_name(e_SD_task_state_t state)
{
  static constexpr std::array<const char*, 7> state_names{
      {"not scheduled", "schedulable", "scheduled", "runnable", "running", "done", "failed"}};
  return state_names.at(static_cast<int>(log2(static_cast<double>(state))));
}

/**
 * @brief Initializes SD internal data
 *
 * This function must be called before any other SD function. Then you should call SD_create_environment().
 *
 * @param argc argument number
 * @param argv argument list
 * @see SD_create_environment(), SD_exit()
 */
void SD_init_nocheck(int* argc, char** argv)
{
  xbt_assert(sd_global == nullptr, "SD_init() already called");

  surf_init(argc, argv);

  sd_global = new simgrid::sd::Global();

  simgrid::config::set_default<std::string>("host/model", "ptask_L07");
  if (simgrid::config::get_value<bool>("debug/clean-atexit"))
    atexit(SD_exit);
}

/** @brief set a configuration variable
 *
 * Do --help on any simgrid binary to see the list of currently existing configuration variables, and
 * see Section @ref options.
 *
 * Example: SD_config("host/model","default")
 */
void SD_config(const char* key, const char* value)
{
  xbt_assert(sd_global, "ERROR: Please call SD_init() before using SD_config()");
  simgrid::config::set_as_string(key, value);
}

/**
 * @brief Creates the environment
 *
 * The environment (i.e. the @ref SD_host_api "hosts" and the @ref SD_link_api "links") is created with
 * the data stored in the given XML platform file.
 *
 * @param platform_file name of an XML file describing the environment to create
 * @see SD_host_api, SD_link_api
 *
 * The XML file follows this DTD:
 *
 *     @include simgrid.dtd
 *
 * Here is a small example of such a platform:
 *
 *     @include small_platform.xml
 */
void SD_create_environment(const char* platform_file)
{
  simgrid::s4u::Engine::get_instance()->load_platform(platform_file);

  XBT_DEBUG("Host number: %zu, link number: %d", sg_host_count(), sg_link_count());
#if SIMGRID_HAVE_JEDULE
  jedule_sd_init();
#endif
  XBT_VERB("Starting simulation...");
  surf_presolve(); /* Takes traces into account */
}

/**
 * @brief Launches the simulation.
 *
 * The function will execute the @ref SD_RUNNABLE runnable tasks.
 * If @a how_long is positive, then the simulation will be stopped either when time reaches @a how_long or when a watch
 * point is reached.
 * A non-positive value for @a how_long means no time limit, in which case the simulation will be stopped either when a
 * watch point is reached or when no more task can be executed.
 * Then you can call SD_simulate() again.
 *
 * @param how_long maximum duration of the simulation (a negative value means no time limit)
 * @return a dynar of @ref SD_task_t whose state has changed.
 * @see SD_task_schedule(), SD_task_watch()
 */
void SD_simulate(double how_long)
{
  simgrid::sd::simulate(how_long);
}

void SD_simulate_with_update(double how_long, xbt_dynar_t changed_tasks_dynar)
{
  const std::set<SD_task_t>* changed_tasks = simgrid::sd::simulate(how_long);
  for (auto const& task : *changed_tasks)
    xbt_dynar_push(changed_tasks_dynar, &task);
}

/** @brief Returns the current clock, in seconds */
double SD_get_clock()
{
  return surf_get_clock();
}

/**
 * @brief Destroys all SD internal data
 * This function should be called when the simulation is over. Don't forget to destroy too.
 * @see SD_init(), SD_task_destroy()
 */
void SD_exit()
{
#if SIMGRID_HAVE_JEDULE
  jedule_sd_exit();
#endif
  delete sd_global;
}
