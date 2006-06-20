#include "simdag/simdag.h"
#include "private.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

SD_global_t sd_global = NULL;

/* Initialises SD internal data. This function should be called before any other SD function.
 */
void SD_init(int *argc, char **argv) {
  xbt_assert0(sd_global == NULL, "SD_init already called");

  sd_global = xbt_new0(s_SD_global_t, 1);
  sd_global->workstations = xbt_dict_new();
  sd_global->workstation_count = 0;
  /*sd_global->links = xbt_dynar_new(sizeof(s_SD_link_t), __SD_link_destroy);*/
  sd_global->links = xbt_dict_new();

  surf_init(argc, argv);
}

/* Creates the environnement described in a xml file of a platform description.
 */
void SD_create_environment(const char *platform_file) {
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *surf_workstation = NULL;
  void *surf_link = NULL;

  CHECK_INIT_DONE();

  surf_timer_resource_init(platform_file);  /* tell Surf to create the environnement */

  
  /*printf("surf_workstation_resource = %p, workstation_set = %p\n", surf_workstation_resource, workstation_set);
    printf("surf_network_resource = %p, network_link_set = %p\n", surf_network_resource, network_link_set);*/

  surf_workstation_resource_init_KCCFLN05(platform_file);
  /*surf_workstation_resource_init_CLM03(platform_file);*/

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

  return NULL;
}

/* Destroys all SD data. This function should be called when the simulation is over.
 */
void SD_clean() {
  if (sd_global != NULL) {
    xbt_dict_free(&sd_global->workstations);
    xbt_dict_free(&sd_global->links);
    xbt_free(sd_global);
    surf_exit();
    /* TODO: destroy the tasks */
  }
}
