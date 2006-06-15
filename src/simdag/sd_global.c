#include "simdag/simdag.h"
#include "private.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

#define CHECK_INIT_DONE() xbt_assert0(init_done, "SG_init not called yet")

SG_global_t sg_global = NULL;

static int init_done = 0;

/* Initialises SG internal data. This function should be called before any other SG function.
 */
void SG_init(int *argc, char **argv) {
  xbt_assert0(!init_done, "SG_init already called");

  sg_global = xbt_new0(s_SG_global_t, 1);
  sg_global->workstations = xbt_dict_new();
  sg_global->workstation_count = 0;

  surf_init(argc, argv);

  init_done = 1;
}

/* Creates the environnement described in a xml file of a platform descriptions.
 */
void SG_create_environment(const char *platform_file) {
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *workstation = NULL;

  CHECK_INIT_DONE();
  surf_timer_resource_init(platform_file);
  surf_workstation_resource_init_KCCFLN05(platform_file); /* tell Surf to create the environnement */

  /* now let's create the SG wrappers */
  xbt_dict_foreach(workstation_set, cursor, name, workstation) {
    __SG_workstation_create(name, workstation, NULL);
  }
}

/* Launches the simulation. Returns a NULL-terminated array of SG_task_t whose state has changed.
 */
SG_task_t* SG_simulate(double how_long)
{
  /* TODO */

  /* temporary test to access to the surf workstation structure */
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *workstation = NULL;
  const char *surf_name;
  int speed;

  xbt_dict_foreach(workstation_set, cursor, name, workstation) {
    surf_name = surf_workstation_resource->common_public->get_resource_name(workstation);
    speed = surf_workstation_resource->extension_public->get_speed(workstation, 1.0);
    printf("Workstation name: %s, Surf name: %s, speed: %d\n", name, surf_name, speed);
  }

  return NULL;
}

/* Destroys all SG data. This function should be called when the simulation is over.
 */
void SG_clean() {
  if (init_done) {
    xbt_dict_free(&sg_global->workstations);
    xbt_free(sg_global);
    surf_exit();
    /* TODO: destroy the workstations, the links and the tasks */
  }
}
