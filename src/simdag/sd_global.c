#include "simdag/simdag.h"
#include "private.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

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

/* Checks that SG_init has been called.
 */
void check_init_done() {
  xbt_assert0(init_done, "SG_init not called yet");
}

/* Creates the environnement described in a xml file of a platform descriptions.
 */
void SG_create_environnement(const char *platform_file) {
  check_init_done();
  surf_timer_resource_init(platform_file);
  surf_workstation_resource_init_KCCFLN05(platform_file); /* tell Surf to create the environnement */
}


/* Launches the simulation. Returns a NULL-terminated array of SG_task_t whose state has changed.
 */
SG_task_t* SG_simulate(double how_long)
{
  /* TODO */
  return NULL;
}
