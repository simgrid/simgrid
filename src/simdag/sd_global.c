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

  surf_init(argc, argv);
}

/* Creates the environnement described in a xml file of a platform descriptions.
 */
void SD_create_environment(const char *platform_file) {
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *surf_workstation = NULL;

  CHECK_INIT_DONE();
  surf_timer_resource_init(platform_file);
  surf_workstation_resource_init_KCCFLN05(platform_file); /* tell Surf to create the environnement */

  /* now let's create the SD wrappers */
  xbt_dict_foreach(workstation_set, cursor, name, surf_workstation) {
    __SD_workstation_create(surf_workstation, NULL);
  }
}

/* Launches the simulation. Returns a NULL-terminated array of SD_task_t whose state has changed.
 */
SD_task_t* SD_simulate(double how_long)
{
  /* TODO */

  /* temporary test to explore the workstations */
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  SD_workstation_t workstation = NULL;
  double power, available_power;

  surf_solve();
  
  xbt_dict_foreach(sd_global->workstations, cursor, name, workstation) {
    power = SD_workstation_get_power(workstation);
    available_power = SD_workstation_get_available_power(workstation);
    printf("Workstation name: %s, power: %f Mflop/s, available power: %f%%\n", name, power, (available_power*100));
  }
  /* TODO: remove name from SD workstation structure */

  return NULL;
}

/* Destroys all SD data. This function should be called when the simulation is over.
 */
void SD_clean() {
  if (sd_global != NULL) {
    xbt_dict_free(&sd_global->workstations);
    xbt_free(sd_global);
    surf_exit();
    /* TODO: destroy the workstations, the links and the tasks */
  }
}
