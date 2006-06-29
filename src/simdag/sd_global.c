#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "surf/surf.h"

XBT_LOG_NEW_CATEGORY(sd,"Logging specific to SimDag");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_kernel,sd,
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
void SD_init(int *argc, char **argv) {
  xbt_assert0(sd_global == NULL, "SD_init already called");

  sd_global = xbt_new0(s_SD_global_t, 1);
  sd_global->workstations = xbt_dict_new();
  sd_global->workstation_count = 0;
  sd_global->links = xbt_dict_new();
  sd_global->watch_point_reached = 0;

  s_SD_task_t task;
  sd_global->not_scheduled_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->scheduled_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->ready_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->running_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->done_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->failed_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));

  surf_init(argc, argv);
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
 *     \include surfxml.dtd
 *
 * Here is a small example of such a platform: 
 *
 *     \include small_platform.xml
 */
void SD_create_environment(const char *platform_file) {
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *surf_workstation = NULL;
  void *surf_link = NULL;

  SD_CHECK_INIT_DONE();

  surf_timer_resource_init(platform_file);  /* tell Surf to create the environnement */

  
  /*printf("surf_workstation_resource = %p, workstation_set = %p\n", surf_workstation_resource, workstation_set);
    printf("surf_network_resource = %p, network_link_set = %p\n", surf_network_resource, network_link_set);*/

  /*surf_workstation_resource_init_KCCFLN05(platform_file);*/
  surf_workstation_resource_init_CLM03(platform_file);

  /*printf("surf_workstation_resource = %p, workstation_set = %p\n", surf_workstation_resource, workstation_set);
    printf("surf_network_resource = %p, network_link_set = %p\n", surf_network_resource, network_link_set);*/


  /* now let's create the SD wrappers for workstations and links */
  xbt_dict_foreach(workstation_set, cursor, name, surf_workstation) {
    __SD_workstation_create(surf_workstation, NULL);
  }

  xbt_dict_foreach(network_link_set, cursor, name, surf_link) {
    __SD_link_create(surf_link, NULL);
  }
}

/**
 * \brief Launches the simulation.
 *
 * The function will execute the \ref SD_READY ready tasks.
 * The simulation will be stopped when its time reaches \a how_long,
 * when a watch point is reached, or when no more task can be executed.
 * Then you can call SD_simulate() again.
 * 
 * \param how_long maximum duration of the simulation
 * \return a NULL-terminated array of \ref SD_task_t whose state has changed.
 * \see SD_task_schedule(), SD_task_watch()
 */
SD_task_t* SD_simulate(double how_long)
{
  double total_time = 0.0; /* we stop the simulation when total_time >= how_long */
  double elapsed_time = 0.0;
  SD_task_t task, dst;
  SD_dependency_t dependency;
  surf_action_t action;
  SD_task_t *changed_tasks = NULL;
  int changed_task_number = 0;
  int changed_task_capacity = 16; /* will be increased if necessary */
  int i;
  static int first_time = 1;

  INFO0("Starting simulation...");

  /* create the array that will be returned */
  changed_tasks = xbt_new0(SD_task_t, changed_task_capacity);
  changed_tasks[0] = NULL;

  if (first_time) {
    surf_solve(); /* Takes traces into account. Returns 0.0 */
    first_time = 0;
  }

  sd_global->watch_point_reached = 0;

  /* explore the ready tasks */
  xbt_swag_foreach(task, sd_global->ready_task_set) {
    INFO1("Executing task '%s'", SD_task_get_name(task));
    action = __SD_task_run(task);
    surf_workstation_resource->common_public->action_set_data(action, task);
    task->state_changed = 1;
    
    changed_tasks[changed_task_number++] = task; /* replace NULL by the task */
    if (changed_task_number == changed_task_capacity) {
      changed_task_capacity *= 2;
      changed_tasks = xbt_realloc(changed_tasks, sizeof(SD_task_t) * changed_task_capacity);
    }
    changed_tasks[changed_task_number] = NULL;
  }

  /* main loop */
  elapsed_time = 0.0;
  while (elapsed_time >= 0.0 && total_time < how_long && !sd_global->watch_point_reached) {

    elapsed_time = surf_solve();
    if (elapsed_time > 0.0)
      total_time += elapsed_time;

    /* let's see which tasks are done */
    while ((action = xbt_swag_extract(surf_workstation_resource->common_public->states.done_action_set))) {
      task = action->data;
      INFO1("Task '%s' done", SD_task_get_name(task));
      __SD_task_set_state(task, SD_DONE);

      /* the state has changed */
      if (!task->state_changed) {
	task->state_changed = 1;
	changed_tasks[changed_task_number++] = task;
	if (changed_task_number == changed_task_capacity) {
	  changed_task_capacity *= 2;
	  changed_tasks = xbt_realloc(changed_tasks, sizeof(SD_task_t) * changed_task_capacity);
	}
	changed_tasks[changed_task_number] = NULL;
      }

      /* remove the dependencies after this task */
      while (xbt_dynar_length(task->tasks_after) > 0) {
	xbt_dynar_get_cpy(task->tasks_after, 0, &dependency);
	dst = dependency->dst;
	SD_task_dependency_remove(task, dst);
	
	/* is dst ready now? */
	if (__SD_task_is_ready(dst) && !sd_global->watch_point_reached) {
	  INFO1("Executing task '%s'", SD_task_get_name(dst));
	  action = __SD_task_run(dst);
	  surf_workstation_resource->common_public->action_set_data(action, dst);
	  dst->state_changed = 1;
	  
	  changed_tasks[changed_task_number++] = dst;
	  if (changed_task_number == changed_task_capacity) {
	    changed_task_capacity *= 2;
	    changed_tasks = xbt_realloc(changed_tasks, sizeof(SD_task_t) * changed_task_capacity);
	  }
	  changed_tasks[changed_task_number] = NULL;
	}
      }
    }

    /* let's see which tasks have just failed */
    while ((action = xbt_swag_extract(surf_workstation_resource->common_public->states.failed_action_set))) {
      task = action->data;
      INFO1("Task '%s' failed", SD_task_get_name(task));
      __SD_task_set_state(task, SD_FAILED);
      if (!task->state_changed) {
	task->state_changed = 1;
	changed_tasks[changed_task_number++] = task;
	if (changed_task_number == changed_task_capacity) {
	  changed_task_capacity *= 2;
	  changed_tasks = xbt_realloc(changed_tasks, sizeof(SD_task_t) * changed_task_capacity);
	}
	changed_tasks[changed_task_number] = NULL;
      }
    }
  }

  /* we must reset every task->state_changed */
  i = 0;
  while (changed_tasks[i] != NULL) {
    changed_tasks[i]->state_changed = 0;
    i++;
  }

  INFO0("Simulation finished");
  printf("elapsed_time = %f, total_time = %f, watch_point_reached = %d\n", elapsed_time, total_time, sd_global->watch_point_reached);

  return changed_tasks;
}

/**
 * \brief Destroys all SD internal data.
 *
 * This function should be called when the simulation is over. Don't forget also to destroy
 * the tasks.
 *
 * \see SD_init(), SD_task_destroy()
 */
void SD_exit(void) {
  if (sd_global != NULL) {
    xbt_dict_free(&sd_global->workstations);
    xbt_dict_free(&sd_global->links);
    xbt_free(sd_global);

    xbt_swag_free(sd_global->not_scheduled_task_set);
    xbt_swag_free(sd_global->scheduled_task_set);
    xbt_swag_free(sd_global->ready_task_set);
    xbt_swag_free(sd_global->running_task_set);
    xbt_swag_free(sd_global->done_task_set);
    xbt_swag_free(sd_global->failed_task_set);

    surf_exit();
  }
}
