#include "simdag/simdag.h"
#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "surf/surf.h"

SD_global_t sd_global = NULL;

/* Initialises SD internal data. This function should be called before any other SD function.
 */
void SD_init(int *argc, char **argv) {
  xbt_assert0(sd_global == NULL, "SD_init already called");

  sd_global = xbt_new0(s_SD_global_t, 1);
  sd_global->workstations = xbt_dict_new();
  sd_global->workstation_count = 0;
  sd_global->links = xbt_dict_new();
  sd_global->tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);

  s_SD_task_t task;
  sd_global->not_scheduled_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->scheduled_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->running_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->done_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->failed_task_set = xbt_swag_new(xbt_swag_offset(task, state_hookup));

  surf_init(argc, argv);
}

/* Creates the environnement described in a xml file of a platform description.
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

/* Launches the simulation. Returns a NULL-terminated array of SD_task_t whose state has changed.
 */
SD_task_t* SD_simulate(double how_long)
{
  /* TODO */

  double total_time = 0.0; /* we stop the simulation when total_time >= how_long */
  double elapsed_time = 0.0;
  int watch_point_reached = 0;
  int i;
  SD_task_t task;
  surf_action_t surf_action;

  surf_solve(); /* Takes traces into account. Returns 0.0 */

  /* main loop */
  while (elapsed_time >= 0.0 && total_time < how_long && !watch_point_reached) {
    for (i = 0 ; i < xbt_dynar_length(sd_global->tasks); i++) {
      xbt_dynar_get_cpy(sd_global->tasks, i, &task);
      printf("Examining task '%s'...\n", SD_task_get_name(task));

      /* if the task is scheduled and the dependencies are satisfied,
	 we can execute the task */
      if (SD_task_get_state(task) == SD_SCHEDULED) {
	printf("Task '%s' is scheduled.\n", SD_task_get_name(task));

	if (xbt_dynar_length(task->tasks_before) == 0) {
	  printf("The dependencies are satisfied. Executing task '%s'\n", SD_task_get_name(task));
	  surf_action = __SD_task_run(task);
	}
	else {
	  printf("Cannot execute task '%s' because some depencies are not satisfied.\n", SD_task_get_name(task));
	}
      }
      else {
	printf("Task '%s' is not scheduled. Nothing to do.\n", SD_task_get_name(task));
      }
    }
    elapsed_time = surf_solve();
    if (elapsed_time > 0.0)
      total_time += elapsed_time;
    printf("Total time: %f\n", total_time);
  }

  return NULL;
}

void SD_test() {
  /* temporary test to explore the workstations and the links */
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  SD_workstation_t workstation = NULL;
  double power, available_power, bandwidth, latency;
  SD_link_t link = NULL;

  surf_solve();

  xbt_dict_foreach(sd_global->workstations, cursor, name, workstation) {
    power = SD_workstation_get_power(workstation);
    available_power = SD_workstation_get_available_power(workstation);
    printf("Workstation name: %s, power: %f Mflop/s, available power: %f%%\n", name, power, (available_power*100));
  }

  xbt_dict_foreach(sd_global->links, cursor, name, link) {
    bandwidth = SD_link_get_current_bandwidth(link);
    latency = SD_link_get_current_latency(link);
    printf("Link name: %s, bandwidth: %f, latency: %f\n", name, bandwidth, latency);
  }

  /* test the route between two workstations */
  SD_workstation_t src, dst;
  xbt_dict_cursor_first(sd_global->workstations, &cursor);
  xbt_dict_cursor_get_or_free(&cursor, &name, (void**) &src);
  xbt_dict_cursor_step(cursor);
  xbt_dict_cursor_get_or_free(&cursor, &name, (void**) &dst);
  xbt_dict_cursor_free(&cursor);

  SD_link_t *route = SD_workstation_route_get_list(src, dst);
  int route_size = SD_workstation_route_get_size(src, dst);

  printf("Route between %s and %s (%d links) : ", SD_workstation_get_name(src), SD_workstation_get_name(dst), route_size);
  int i;
  for (i = 0; i < route_size; i++) {
    printf("%s ", SD_link_get_name(route[i]));
  }
  printf("\n");
}

/* Destroys all SD internal data. This function should be called when the simulation is over.
 * The tasks should have been destroyed first.
 */
void SD_exit() {
  if (sd_global != NULL) {
    xbt_dict_free(&sd_global->workstations);
    xbt_dict_free(&sd_global->links);
    xbt_dynar_free(&sd_global->tasks);
    xbt_free(sd_global);

    xbt_swag_free(sd_global->not_scheduled_task_set);
    xbt_swag_free(sd_global->scheduled_task_set);
    xbt_swag_free(sd_global->running_task_set);
    xbt_swag_free(sd_global->done_task_set);
    xbt_swag_free(sd_global->failed_task_set);

    surf_exit();
  }
}
