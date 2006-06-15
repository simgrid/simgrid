#include "private.h"
#include "simdag/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"

/* Creates a workstation and registers it in SG.
 */
SG_workstation_t __SG_workstation_create(const char *name, void *surf_workstation, void *data) {
  SG_workstation_data_t sgdata = xbt_new0(s_SG_workstation_data_t, 1); /* workstation private data */
  sgdata->surf_workstation = surf_workstation;
  
  SG_workstation_t workstation = xbt_new0(s_SG_workstation_t, 1);
  workstation->name = xbt_strdup(name);
  workstation->data = data; /* user data */
  workstation->sgdata = sgdata; /* private data */
  
  xbt_dict_set(sg_global->workstations, name, workstation, free); /* add the workstation to the dictionary */

  /* TODO: route */

  return workstation;
}

/* Returns a workstation given its name, or NULL if there is no such workstation.
 */
SG_workstation_t SG_workstation_get_by_name(const char *name) {
  xbt_assert0(sg_global != NULL, "SG_init not called yet");
  xbt_assert0(name != NULL, "Invalid parameter");

  return xbt_dict_get_or_null(sg_global->workstations, name);
}

/* Returns a NULL-terminated array of existing workstations.
 */
SG_workstation_t*  SG_workstation_get_list(void) {
  xbt_assert0(sg_global != NULL, "SG_init not called yet");
  SG_workstation_t* array = xbt_new0(SG_workstation_t, sg_global->workstation_count + 1);
  
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int i=0;

  xbt_dict_foreach(sg_global->workstations,cursor,key,data) {
    array[i++] = (SG_workstation_t) data;
  }
  array[i] = NULL;

  return array;
}

/* Returns the number or workstations.
 */
int SG_workstation_get_number(void) {
  xbt_assert0(sg_global != NULL, "SG_init not called yet");
  return sg_global->workstation_count;
}

/* Sets the data of a workstation.
 */
void SG_workstation_set_data(SG_workstation_t workstation, void *data) {
  xbt_assert0(workstation != NULL, "Invalid parameter");
  workstation->data = data;
}

/* Returns the data of a workstation.
 */
void* SG_workstation_get_data(SG_workstation_t workstation) {
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return workstation->data;
}

/* Returns the name of a workstation.
 */
const char* SG_workstation_get_name(SG_workstation_t workstation) {
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return NULL;
  /*  return workstation->name;*/
}

SG_link_t* SG_workstation_route_get_list(SG_workstation_t src, SG_workstation_t dst) {
  /* TODO */
  return NULL;
}

int SG_workstation_route_get_size(SG_workstation_t src, SG_workstation_t dst) {
  /* TODO */
  return 0;
}

/* Returns the total power of a workstation.
 */
double SG_workstation_get_power(SG_workstation_t workstation) {
  xbt_assert0(workstation != NULL, "Invalid parameter");
  /* TODO */
  return 0;
  /*  return workstation->power;*/
}

/* Return the available power of a workstation.
 */
double SG_workstation_get_available_power(SG_workstation_t workstation) {
  xbt_assert0(workstation != NULL, "Invalid parameter");
  /* TODO */
  return 0;
  /*return workstation->available_power;*/
}

/* Destroys a workstation. The user data (if any) should have been destroyed first.
 */
void __SG_workstation_destroy(SG_workstation_t workstation) {
  xbt_assert0(workstation != NULL, "Invalid parameter");

  SG_workstation_data_t sgdata = workstation->sgdata;
  if (sgdata != NULL) {
    xbt_free(sgdata);
  }
  
  /* TODO: route */

  xbt_free(workstation);
}
