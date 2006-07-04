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

/**
 * \brief Returns a workstation given its name
 *
 * If there is no such workstation, the function returns \c NULL.
 *
 * \param name workstation name
 * \return the workstation, or \c NULL if there is no such workstation
 */
SD_workstation_t SD_workstation_get_by_name(const char *name) {
  SD_CHECK_INIT_DONE();

  xbt_assert0(name != NULL, "Invalid parameter");

  return xbt_dict_get_or_null(sd_global->workstations, name);
}

/**
 * \brief Returns the workstation list
 *
 * Use SD_workstation_get_number() to know the array size.
 *
 * \return an array of \ref SD_workstation_t containing all workstations
 * \see SD_workstation_get_number()
 */
const SD_workstation_t*  SD_workstation_get_list(void) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(SD_workstation_get_number() > 0, "There is no workstation!");

  SD_workstation_t *array = sd_global->workstation_list;
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int i;

  if (array == NULL) { /* this is the first time the function is called */
    array = xbt_new0(SD_workstation_t, sd_global->workstation_count);
  
    i = 0;
    xbt_dict_foreach(sd_global->workstations, cursor, key, data) {
      array[i++] = (SD_workstation_t) data;
    }
  }
  return array;
}

/**
 * \brief Returns the number of workstations
 *
 * \return the number of existing workstations
 * \see SD_workstation_get_list()
 */
int SD_workstation_get_number(void) {
  SD_CHECK_INIT_DONE();
  return sd_global->workstation_count;
}

/**
 * \brief Returns the user data of a workstation
 *
 * \param workstation a workstation
 * \return the user data associated with this workstation (can be \c NULL)
 * \see SD_workstation_set_data()
 */
void* SD_workstation_get_data(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return workstation->data;
}

/**
 * \brief Sets the user data of a workstation
 *
 * The new data can be \c NULL. The old data should have been freed first
 * if it was not \c NULL.
 *
 * \param workstation a workstation
 * \param data the new data you want to associate with this workstation
 * \see SD_workstation_get_data()
 */
void SD_workstation_set_data(SD_workstation_t workstation, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  workstation->data = data;
}

/**
 * \brief Returns the name of a workstation
 *
 * \param workstation a workstation
 * \return the name of this workstation (cannot be \c NULL)
 */
const char* SD_workstation_get_name(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->common_public->get_resource_name(workstation->surf_workstation);
}

/**
 * \brief Returns the route between two workstations
 *
 * Use SD_workstation_route_get_size() to know the array size. Don't forget to free the array after use.
 *
 * \param src a workstation
 * \param dst another workstation
 * \return a new array of \ref SD_link_t representating the route between these two workstations
 * \see SD_workstation_route_get_size(), SD_link_t
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

/**
 * \brief Returns the number of links on the route between two workstations
 *
 * \param src a workstation
 * \param dst another workstation
 * \return the number of links on the route between these two workstations
 * \see SD_workstation_route_get_list()
 */
int SD_workstation_route_get_size(SD_workstation_t src, SD_workstation_t dst) {
  SD_CHECK_INIT_DONE();
  return surf_workstation_resource->extension_public->
    get_route_size(src->surf_workstation, dst->surf_workstation);
}

/**
 * \brief Returns the total power of a workstation
 *
 * \param workstation a workstation
 * \return the total power of this workstation
 * \see SD_workstation_get_available_power()
 */
double SD_workstation_get_power(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_speed(workstation->surf_workstation, 1.0);
}

/**
 * \brief Returns the proportion of available power in a workstation
 *
 * \param workstation a workstation
 * \return the proportion of power currently available in this workstation (normally a number between 0 and 1)
 * \see SD_workstation_get_power()
 */
double SD_workstation_get_available_power(SD_workstation_t workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_resource->extension_public->get_available_speed(workstation->surf_workstation);
}

/**
 * \brief Returns an approximative estimated time for the given computation amount on a workstation
 *
 * \param workstation a workstation
 * \param computation_amount the computation amount you want to evaluate (in flops)
 * \return an approximative astimated computation time for the given computation amount on this workstation (in seconds)
 */
double SD_workstation_get_computation_time(SD_workstation_t workstation, double computation_amount) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  xbt_assert0(computation_amount >= 0, "computation_amount must be greater than or equal to zero");
  return computation_amount / SD_workstation_get_power(workstation);
}

/**
 * \brief Returns the latency of the route between two workstations, i.e. the sum of all link latencies
 * between the workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the latency of the route between the two workstations (in seconds)
 */
double SD_workstation_route_get_latency(SD_workstation_t src, SD_workstation_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  SD_link_t *links = SD_workstation_route_get_list(src, dst);
  int nb_links = SD_workstation_route_get_size(src, dst);
  double latency = 0.0;
  int i;
  
  for (i = 0; i < nb_links; i++) {
    latency += SD_link_get_current_latency(links[i]);
  }

  free(links);
  return latency;
}

/**
 * \brief Returns the bandwidth of the route between two workstations, i.e. the minimum link bandwidth of all
 * between the workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the bandwidth of the route between the two workstations (in bytes/second)
 */
double SD_workstation_route_get_bandwidth(SD_workstation_t src, SD_workstation_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  SD_link_t *links = SD_workstation_route_get_list(src, dst);
  int nb_links = SD_workstation_route_get_size(src, dst);
  double bandwidth, min_bandwidth = -1.0;
  int i;
  
  for (i = 0; i < nb_links; i++) {
    bandwidth = SD_link_get_current_bandwidth(links[i]);
    if (bandwidth < min_bandwidth || min_bandwidth == -1.0)
      min_bandwidth = bandwidth;
  }

  free(links);
  return min_bandwidth;
}

/**
 * \brief Returns an approximative estimated time for the given
 * communication amount between two workstations
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \param communication_amount the communication amount you want to evaluate (in bytes)
 * \return an approximative astimated computation time for the given communication amount
 * between the workstations (in seconds)
 */
double SD_workstation_route_get_communication_time(SD_workstation_t src, SD_workstation_t dst,
						   double communication_amount) {
  /* total time = latency + transmission time of the slowest link
     transmission time of a link = communication amount / link bandwidth */
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  xbt_assert0(communication_amount >= 0, "communication_amount must be greater than or equal to zero");

  SD_link_t *links;
  int nb_links;
  double bandwidth, min_bandwidth;
  double latency;
  int i;
  
  if (communication_amount == 0.0)
    return 0.0;

  links = SD_workstation_route_get_list(src, dst);
  nb_links = SD_workstation_route_get_size(src, dst);
  min_bandwidth = -1.0;
  latency = 0;

  for (i = 0; i < nb_links; i++) {
    latency += SD_link_get_current_latency(links[i]);
    bandwidth = SD_link_get_current_bandwidth(links[i]);
    if (bandwidth < min_bandwidth || min_bandwidth == -1.0)
      min_bandwidth = bandwidth;
  }

  return latency + (communication_amount / min_bandwidth);
}

/* Destroys a workstation.
 */
void __SD_workstation_destroy(void *workstation) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  /* workstation->surf_workstation is freed by surf_exit and workstation->data is freed by the user */
  xbt_free(workstation);
}
