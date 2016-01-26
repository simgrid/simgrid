/* Copyright (c) 2006-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/host_interface.hpp"
#include "src/simdag/simdag_private.h"
#include "simgrid/simdag.h"
#include "simgrid/host.h"
#include <simgrid/s4u/host.hpp>
#include "xbt/dict.h"
#include "xbt/lib.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_workstation, sd,
                                "Logging specific to SimDag (workstation)");

/* Creates a storage and registers it in SD.
 */
SD_storage_t __SD_storage_create(void *surf_storage, void *data)
{

  SD_storage_priv_t storage;
  const char *name;

  storage = xbt_new(s_SD_storage_priv_t, 1);
  storage->data = data;     /* user data */
  name = surf_resource_name((surf_cpp_resource_t)surf_storage);
  storage->host = (const char*)surf_storage_get_host( (surf_resource_t )surf_storage_resource_by_name(name));
  xbt_lib_set(storage_lib,name, SD_STORAGE_LEVEL, storage);
  return xbt_lib_get_elm_or_null(storage_lib, name);
}

/* Destroys a storage.
 */
void __SD_storage_destroy(void *storage)
{
  SD_storage_priv_t s;

  s = (SD_storage_priv_t) storage;
  xbt_free(s);
}

/**
 * \brief Returns the workstation list
 *
 * Use sg_host_count() to know the array size.
 * 
 * \return an array of \ref sg_host_t containing all workstations
 * \remark The workstation order in the returned array is generally different from the workstation creation/declaration order in the XML platform (we use a hash table internally).
 * \see sg_host_count()
 */
const sg_host_t *sg_host_list(void) {
  xbt_assert(sg_host_count() > 0, "There is no workstation!");

  if (sd_global->host_list == NULL)     /* this is the first time the function is called */
    sd_global->host_list = (sg_host_t*)xbt_dynar_to_array(sg_hosts_as_dynar());

  return sd_global->host_list;
}

/** @brief Displays debugging informations about a workstation */
void sg_host_dump(sg_host_t ws)
{
  xbt_dict_t props;
  xbt_dict_cursor_t cursor=NULL;
  char *key,*data;

  XBT_INFO("Displaying workstation %s", sg_host_get_name(ws));
  XBT_INFO("  - speed: %.0f", ws->speed());
  XBT_INFO("  - available speed: %.2f", sg_host_get_available_speed(ws));
  props = sg_host_get_properties(ws);
  
  if (!xbt_dict_is_empty(props)){
    XBT_INFO("  - properties:");

    xbt_dict_foreach(props,cursor,key,data) {
      XBT_INFO("    %s->%s",key,data);
    }
  }
}

/**
 * \brief Returns the route between two workstations
 *
 * Use SD_route_get_size() to know the array size.
 *
 * \param src a workstation
 * \param dst another workstation
 * \return a new array of \ref SD_link_t representing the route between these two workstations
 * \see SD_route_get_size(), SD_link_t
 */
double sg_host_speed(sg_host_t workstation)
{
  return workstation->speed();
}
int sg_host_core_count(sg_host_t workstation) {
  return workstation->core_count();
}


double sg_host_computation_time(sg_host_t workstation,
                                           double flops_amount)
{
  xbt_assert(flops_amount >= 0,
              "flops_amount must be greater than or equal to zero");
  return flops_amount / workstation->speed();
}

xbt_dict_t sg_host_get_mounted_storage_list(sg_host_t workstation){
  return workstation->extension<simgrid::surf::Host>()->getMountedStorageList();
}

xbt_dynar_t sg_host_get_attached_storage_list(sg_host_t workstation){
  return surf_host_get_attached_storage_list(workstation);
}

const SD_link_t *SD_route_get_list(sg_host_t src,
                                   sg_host_t dst)
{
  xbt_dynar_t surf_route;
  void *surf_link;
  unsigned int cpt;

  if (sd_global->recyclable_route == NULL) {
    /* first run */
    sd_global->recyclable_route = xbt_new(SD_link_t, sg_link_count());
  }

  surf_route = surf_host_model_get_route((surf_host_model_t)surf_host_model, src, dst);

  xbt_dynar_foreach(surf_route, cpt, surf_link) {
    sd_global->recyclable_route[cpt] = (SD_link_t)surf_link;
  }
  return sd_global->recyclable_route;
}

/**
 * \brief Returns the number of links on the route between two workstations
 *
 * \param src a workstation
 * \param dst another workstation
 * \return the number of links on the route between these two workstations
 * \see SD_route_get_list()
 */
int SD_route_get_size(sg_host_t src, sg_host_t dst)
{
  return xbt_dynar_length(surf_host_model_get_route(
		    (surf_host_model_t)surf_host_model, src, dst));
}

/**
 * \brief Returns the total speed of a workstation
 *
 * \param workstation a workstation
 * \return the total speed of this workstation
 * \see sg_host_get_available_speed()
 */
/**
 * \brief Returns the amount of cores of a workstation
 *
 * \param workstation a workstation
 * \return the amount of cores of this workstation
 */
/**
 * \brief Returns an approximative estimated time for the given computation amount on a workstation
 *
 * \param workstation a workstation
 * \param flops_amount the computation amount you want to evaluate (in flops)
 * \return an approximative estimated computation time for the given computation amount on this workstation (in seconds)
 */
/**
 * \brief Returns the latency of the route between two workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the latency of the route between the two workstations (in seconds)
 * \see SD_route_get_bandwidth()
 */
double SD_route_get_latency(sg_host_t src, sg_host_t dst)
{
  xbt_dynar_t route = NULL;
  double latency = 0;

  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard,
                                    &route, &latency);

  return latency;
}

/**
 * \brief Returns the bandwidth of the route between two workstations,
 * i.e. the minimum link bandwidth of all between the workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the bandwidth of the route between the two workstations
 * (in bytes/second)
 * \see SD_route_get_latency()
 */
double SD_route_get_bandwidth(sg_host_t src, sg_host_t dst)
{

  const SD_link_t *links;
  int nb_links;
  double bandwidth;
  double min_bandwidth;
  int i;

  links = SD_route_get_list(src, dst);
  nb_links = SD_route_get_size(src, dst);
  min_bandwidth = -1.0;

  for (i = 0; i < nb_links; i++) {
    bandwidth = sg_link_bandwidth(links[i]);
    if (bandwidth < min_bandwidth || min_bandwidth == -1.0)
      min_bandwidth = bandwidth;
  }

  return min_bandwidth;
}

/**
 * \brief Returns an approximative estimated time for the given
 * communication amount between two workstations
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \param bytes_amount the communication amount you want to evaluate (in bytes)
 * \return an approximative estimated communication time for the given bytes amount
 * between the workstations (in seconds)
 */
double SD_route_get_communication_time(sg_host_t src,
                                       sg_host_t dst,
                                       double bytes_amount)
{


  /* total time = latency + transmission time of the slowest link
     transmission time of a link = communication amount / link bandwidth */

  const SD_link_t *links;
  xbt_dynar_t route = NULL;
  int nb_links;
  double bandwidth, min_bandwidth;
  double latency = 0;
  int i;

  xbt_assert(bytes_amount >= 0, "bytes_amount must be greater than or equal to zero");


  if (bytes_amount == 0.0)
    return 0.0;

  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard,
                                    &route, &latency);

  links = SD_route_get_list(src, dst);
  nb_links = SD_route_get_size(src, dst);
  min_bandwidth = -1.0;

  for (i = 0; i < nb_links; i++) {
    bandwidth = sg_link_bandwidth(links[i]);
    if (bandwidth < min_bandwidth || min_bandwidth == -1.0)
      min_bandwidth = bandwidth;
  }

  return latency + (bytes_amount / min_bandwidth);
}

/**
 * \brief Return the list of mounted storages on a workstation.
 *
 * \param workstation a workstation
 * \return a dynar containing all mounted storages on the workstation
 */
/**
 * \brief Return the list of mounted storages on a workstation.
 *
 * \param workstation a workstation
 * \return a dynar containing all mounted storages on the workstation
 */
/**
 * \brief Returns the host name the storage is attached to
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char *SD_storage_get_host(msg_storage_t storage) {
  xbt_assert((storage != NULL), "Invalid parameters");
  SD_storage_priv_t priv = SD_storage_priv(storage);
  return priv->host;
}
