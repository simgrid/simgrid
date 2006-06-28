#include "private.h"
#include "simdag/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

/* Creates a workstation and registers it in SD.
 */
SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(surf_workstation != NULL, "surf_workstation is NULL !");

  SD_workstation_t workstation = xbt_new0(s_SD_workstation_t, 1);
  workstation->surf_workstation = surf_workstation;
  workstation->data = data; /* user data */
  
  const char *name = SD_workstation_get_name(workstation);
  xbt_dict_set(sd_global->workstations, name, workstation, __SD_workstation_destroy); /* add the workstation to the dictionary */
  sd_global->workstation_count++;

  return workstation;
}

/** @ingroup SD_workstation_management
 * @brief Returns a workstation given its name
 *
 * If there is no such workstation, the function returns NULL.
 *
 * @param name workstation name
 * @return the workstation, or NULL if there is no such workstation
 */
SD_workstation_t SD_workstation_get_by_name(const char *name) {
  SD_CHECK_INIT_DONE();

  xbt_assert0(name != NULL, "Invalid parameter");

  return xbt_dict_get_or_null(sd_global->workstations, name);
}

/** @ingroup SD_workstation_management
 * @brief Returns the workstations list
 *
 * Use SD_workstation_get_number to known the array size.
 *
 * @return an array of SD_workstation_t containing all workstations
 * @see SD_workstation_get_number
 */
SD_workstation_t*  SD_workstation_get_list(void) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(SD_workstation_get_number() > 0, "There is no workstation!");

  SD_workstation_t* array = xbt_new0(SD_workstation_t, sd_global->workstation_count);
  
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int i=0;

  xbt_dict_foreach(sd_global->workstations, cursor, key, data) {
    array[i++] = (SD_workstation_t) data;
  }

  return array;
}

/** @ingroup SD_workstation_management
 * @brief Returns the number of workstations
 *
 * @return the number of existing workstations
 * @see SD_workstation_get_list
 */
int SD_workstation_get_number(void) {
  SD_CHECK_INIT_DONE();
  return sd_global->workstation_count;
}

/** @ingroup SD_workstation_management
 * @brief Sets the user data of a workstation
 *
 * The new data can be NULL. The old data should have been freed first
 * if it was not NULL.
 *
 * @param workstation a workstation
 * @param data the new data you want to associate with this workstation
 * @see SD_workstation_get_data
 */
void SD_workstation_set_data(SD_workstation_t workstation, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  workstation->data = data;
}

/** @ingroup SD_workstation_management
 * @brief Returns the user data of a workstation
 *
 * @param workstation a workstation
 * @return the user data associated with this workstation (can be NULL)
 * @see SD_workstation_set_data
 */
void* SD_workstation_get_data(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return workstation->data;
}

/** @ingroup SD_workstation_management
 * @brief Returns the name of a workstation
 *
 * @param workstation a workstation
 * @return the name of this workstation (cannot be NULL)
 */
const char* SD_workstation_get_name(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->common_public->get_resource_name(workstation->surf_workstation);
}

/** @ingroup SD_workstation_management
 * @brief Returns the route between two workstations
 *
 * Use SD_workstation_route_get_size to known the array size.
 *
 * @param src a workstation
 * @param dst another workstation
 * @return a new array of SD_link_t representating the route between these two workstations
 * @see SD_workstation_route_get_size
 */
SD_link_t* SD_workstation_route_get_list(SD_workstation_t src, SD_workstation_t dst) {
  SD_CHECK_INIT_DONE();

  void *surf_src = src->surf_workstation;
  void *surf_dst = dst->surf_workstation;

  const void **surf_route = surf_workstation_resource->extension_public->get_route(surf_src, surf_dst);
  int route_size = surf_workstation_resource->extension_public->get_route_size(surf_src, surf_dst);

  SD_link_t* route = xbt_new0(SD_link_t, route_size);
  const char *link_name;
  int i;
  for (i = 0; i < route_size; i++) {
    link_name = surf_workstation_resource->extension_public->get_link_name(surf_route[i]);
    route[i] = xbt_dict_get(sd_global->links, link_name);
  }

  return route;
}

/** @ingroup SD_workstation_management
 * @brief Returns the number of links on the route between two workstations
 *
 * @param src a workstation
 * @param dst another workstation
 * @return the number of links on the route between these two workstations
 * @see SD_workstation_route_get_list
 */
int SD_workstation_route_get_size(SD_workstation_t src, SD_workstation_t dst) {
  SD_CHECK_INIT_DONE();
  return surf_workstation_resource->extension_public->
    get_route_size(src->surf_workstation, dst->surf_workstation);
}

/** @ingroup SD_workstation_management
 * @brief Returns the total power of a workstation
 *
 * @param workstation a workstation
 * @return the total power of this workstation
 * @see SD_workstation_get_available_power
 */
double SD_workstation_get_power(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_speed(workstation->surf_workstation, 1.0);
}

/** @ingroup SD_workstation_management
 * @brief Returns the proportion of available power in a workstation
 *
 * @param workstation a workstation
 * @return the proportion of power currently available in this workstation (normally a number between 0 and 1)
 * @see SD_workstation_get_power
 */
double SD_workstation_get_available_power(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_available_speed(workstation->surf_workstation);
}

/* Destroys a workstation.
 */
void __SD_workstation_destroy(void *workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  /* workstation->surf_workstation is freed by surf_exit and workstation->data is freed by the user */
  xbt_free(workstation);
}
