#include "private.h"
#include "simdag/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

/* Creates a workstation and registers it in SD.
 */
SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data) {
  CHECK_INIT_DONE();

  SD_workstation_data_t sd_data = xbt_new0(s_SD_workstation_data_t, 1); /* workstation private data */
  sd_data->surf_workstation = surf_workstation;
  
  SD_workstation_t workstation = xbt_new0(s_SD_workstation_t, 1);
  workstation->data = data; /* user data */
  workstation->sd_data = sd_data; /* private data */
  
  const char *name = SD_workstation_get_name(workstation);
  xbt_dict_set(sd_global->workstations, name, workstation, __SD_workstation_destroy); /* add the workstation to the dictionary */

  /* TODO: route */
  return workstation;
}

/* Returns a workstation given its name, or NULL if there is no such workstation.
 */
SD_workstation_t SD_workstation_get_by_name(const char *name) {
  CHECK_INIT_DONE();

  xbt_assert0(name != NULL, "Invalid parameter");

  return xbt_dict_get_or_null(sd_global->workstations, name);
}

/* Returns a NULL-terminated array of existing workstations.
 */
SD_workstation_t*  SD_workstation_get_list(void) {
  CHECK_INIT_DONE();

  SD_workstation_t* array = xbt_new0(SD_workstation_t, sd_global->workstation_count + 1);
  
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int i=0;

  xbt_dict_foreach(sd_global->workstations,cursor,key,data) {
    array[i++] = (SD_workstation_t) data;
  }
  array[i] = NULL;

  return array;
}

/* Returns the number or workstations.
 */
int SD_workstation_get_number(void) {
  CHECK_INIT_DONE();
  return sd_global->workstation_count;
}

/* Sets the data of a workstation. The new data can be NULL. The old data should have been freed first if it was not NULL.
 */
void SD_workstation_set_data(SD_workstation_t workstation, void *data) {
  CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  workstation->data = data;
}

/* Returns the data of a workstation. The user data can be NULL.
 */
void* SD_workstation_get_data(SD_workstation_t workstation) {
  CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return workstation->data;
}

/* Returns the name of a workstation.
 */
const char* SD_workstation_get_name(SD_workstation_t workstation) {
  CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->common_public->get_resource_name(workstation->sd_data->surf_workstation);
}

SD_link_t* SD_workstation_route_get_list(SD_workstation_t src, SD_workstation_t dst) {
  CHECK_INIT_DONE();
  /* TODO */
  return NULL;
}

int SD_workstation_route_get_size(SD_workstation_t src, SD_workstation_t dst) {
  CHECK_INIT_DONE();
  /* TODO */
  return 0;
}

/* Returns the total power of a workstation.
 */
double SD_workstation_get_power(SD_workstation_t workstation) {
  CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_speed(workstation->sd_data->surf_workstation, 1.0);
}

/* Returns the proportion of available power in a workstation (normally a number between 0 and 1).
 */
double SD_workstation_get_available_power(SD_workstation_t workstation) {
  CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  /* TODO */
  return 0;
  /*return workstation->available_power;*/
}

/* Destroys a workstation. The user data (if any) should have been destroyed first.
 */
void __SD_workstation_destroy(void *workstation) {
  CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");

  if (((SD_workstation_t) workstation)->sd_data != NULL) {
    xbt_free(((SD_workstation_t) workstation)->sd_data);
  }
  
  /* TODO: route */

  xbt_free(workstation);
}
