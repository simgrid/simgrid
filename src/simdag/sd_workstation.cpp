/* Copyright (c) 2006-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/host_interface.hpp"
#include "src/simdag/simdag_private.h"
#include "simgrid/simdag.h"
#include <simgrid/s4u/host.hpp>
#include "xbt/dict.h"
#include "xbt/lib.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"
#include "simgrid/msg.h" //FIXME: why?

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_workstation, sd,
                                "Logging specific to SimDag (workstation)");

/* Creates a workstation and registers it in SD.
 */
SD_workstation_t __SD_workstation_create(const char *name)
{

  SD_workstation_priv_t workstation;

  workstation = xbt_new(s_SD_workstation_priv_t, 1);
  workstation->access_mode = SD_WORKSTATION_SHARED_ACCESS;      /* default mode is shared */
  workstation->task_fifo = NULL;
  workstation->current_task = NULL;

  sg_host_t sg_host = sg_host_by_name(name);
  sg_host_sd_set(sg_host,workstation);
  return sg_host;
}

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
 * \brief Returns a workstation given its name
 *
 * If there is no such workstation, the function returns \c NULL.
 *
 * \param name workstation name
 * \return the workstation, or \c NULL if there is no such workstation
 */
SD_workstation_t SD_workstation_get_by_name(const char *name)
{
  return sg_host_by_name(name);
}

/**
 * \brief Returns the workstation list
 *
 * Use SD_workstation_get_number() to know the array size.
 * 
 * \return an array of \ref SD_workstation_t containing all workstations
 * \remark The workstation order in the returned array is generally different from the workstation creation/declaration order in the XML platform (we use a hash table internally).
 * \see SD_workstation_get_number()
 */
const SD_workstation_t *SD_workstation_get_list(void) {
  xbt_assert(SD_workstation_get_count() > 0, "There is no workstation!");

  if (sd_global->workstation_list == NULL)     /* this is the first time the function is called */
    sd_global->workstation_list = (SD_workstation_t*)xbt_dynar_to_array(sg_hosts_as_dynar());

  return sd_global->workstation_list;
}

/**
 * \brief Returns the number of workstations
 *
 * \return the number of existing workstations
 * \see SD_workstation_get_list()
 */
int SD_workstation_get_count(void)
{
  return sg_host_count();
}

/**
 * \brief Returns the user data of a workstation
 *
 * \param workstation a workstation
 * \return the user data associated with this workstation (can be \c NULL)
 * \see SD_workstation_set_data()
 */
void *SD_workstation_get_data(SD_workstation_t workstation)
{
  return sg_host_user(workstation);
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
void SD_workstation_set_data(SD_workstation_t workstation, void *data)
{
	sg_host_user_set(workstation, data);
}

/**
 * \brief Returns the name of a workstation
 *
 * \param workstation a workstation
 * \return the name of this workstation (cannot be \c NULL)
 */
const char *SD_workstation_get_name(SD_workstation_t workstation)
{
  return sg_host_get_name(workstation);
}

/**
 * \brief Returns the value of a given workstation property
 *
 * \param ws a workstation
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */
const char *SD_workstation_get_property_value(SD_workstation_t ws,
                                              const char *name)
{
  return (const char*) xbt_dict_get_or_null(SD_workstation_get_properties(ws), name);
}


/**
 * \brief Returns a #xbt_dict_t consisting of the list of properties assigned to this workstation
 *
 * \param workstation a workstation
 * \return the dictionary containing the properties associated with the workstation
 */
xbt_dict_t SD_workstation_get_properties(SD_workstation_t workstation)
{
  return sg_host_get_properties(workstation);
}


/** @brief Displays debugging informations about a workstation */
void SD_workstation_dump(SD_workstation_t ws)
{
  xbt_dict_t props;
  xbt_dict_cursor_t cursor=NULL;
  char *key,*data;
  SD_task_t task = NULL;
  
  XBT_INFO("Displaying workstation %s", SD_workstation_get_name(ws));
  XBT_INFO("  - power: %.0f", SD_workstation_get_power(ws));
  XBT_INFO("  - available power: %.2f", SD_workstation_get_available_power(ws));
  switch (sg_host_sd(ws)->access_mode){
  case SD_WORKSTATION_SHARED_ACCESS:
      XBT_INFO("  - access mode: Space shared");
      break;
  case SD_WORKSTATION_SEQUENTIAL_ACCESS:
      XBT_INFO("  - access mode: Exclusive");
    task = SD_workstation_get_current_task(ws);
    if(task)
      XBT_INFO("    current running task: %s",
               SD_task_get_name(task));
    else
      XBT_INFO("    no task running");
      break;
  default: break;
  }
  props = SD_workstation_get_properties(ws);
  
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
const SD_link_t *SD_route_get_list(SD_workstation_t src,
                                   SD_workstation_t dst)
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
int SD_route_get_size(SD_workstation_t src, SD_workstation_t dst)
{
  return xbt_dynar_length(surf_host_model_get_route(
		    (surf_host_model_t)surf_host_model, src, dst));
}

/**
 * \brief Returns the total power of a workstation
 *
 * \param workstation a workstation
 * \return the total power of this workstation
 * \see SD_workstation_get_available_power()
 */
double SD_workstation_get_power(SD_workstation_t workstation)
{
  return workstation->speed();
}
/**
 * \brief Returns the amount of cores of a workstation
 *
 * \param workstation a workstation
 * \return the amount of cores of this workstation
 */
int SD_workstation_get_cores(SD_workstation_t workstation) {
  return workstation->core_count();
}

/**
 * \brief Returns the proportion of available power in a workstation
 *
 * \param workstation a workstation
 * \return the proportion of power currently available in this workstation (normally a number between 0 and 1)
 * \see SD_workstation_get_power()
 */
double SD_workstation_get_available_power(SD_workstation_t workstation)
{
  return surf_host_get_available_speed(workstation);
}

/**
 * \brief Returns an approximative estimated time for the given computation amount on a workstation
 *
 * \param workstation a workstation
 * \param flops_amount the computation amount you want to evaluate (in flops)
 * \return an approximative estimated computation time for the given computation amount on this workstation (in seconds)
 */
double SD_workstation_get_computation_time(SD_workstation_t workstation,
                                           double flops_amount)
{
  xbt_assert(flops_amount >= 0,
              "flops_amount must be greater than or equal to zero");
  return flops_amount / SD_workstation_get_power(workstation);
}

/**
 * \brief Returns the latency of the route between two workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the latency of the route between the two workstations (in seconds)
 * \see SD_route_get_bandwidth()
 */
double SD_route_get_latency(SD_workstation_t src, SD_workstation_t dst)
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
double SD_route_get_bandwidth(SD_workstation_t src, SD_workstation_t dst)
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
double SD_route_get_communication_time(SD_workstation_t src,
                                       SD_workstation_t dst,
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
 * \brief Returns the access mode of this workstation.
 *
 * \param workstation a workstation
 * \return the access mode for the tasks running on this workstation:
 * SD_WORKSTATION_SHARED_ACCESS or SD_WORKSTATION_SEQUENTIAL_ACCESS
 *
 * \see SD_workstation_set_access_mode(), e_SD_workstation_access_mode_t
 */
e_SD_workstation_access_mode_t
SD_workstation_get_access_mode(SD_workstation_t workstation)
{
  return sg_host_sd(workstation)->access_mode;
}

/**
 * \brief Sets the access mode for the tasks that will be executed on a workstation
 *
 * By default, a workstation model is shared, i.e. several tasks
 * can be executed at the same time on a workstation. The CPU power of
 * the workstation is shared between the running tasks on the workstation.
 * In sequential mode, only one task can use the workstation, and the other
 * tasks wait in a FIFO.
 *
 * \param workstation a workstation
 * \param access_mode the access mode you want to set to this workstation:
 * SD_WORKSTATION_SHARED_ACCESS or SD_WORKSTATION_SEQUENTIAL_ACCESS
 *
 * \see SD_workstation_get_access_mode(), e_SD_workstation_access_mode_t
 */
void SD_workstation_set_access_mode(SD_workstation_t workstation,
                                    e_SD_workstation_access_mode_t
                                    access_mode)
{
  xbt_assert(access_mode != SD_WORKSTATION_SEQUENTIAL_ACCESS ||
             access_mode != SD_WORKSTATION_SHARED_ACCESS,
             "Trying to set an invalid access mode");

  if (access_mode == sg_host_sd(workstation)->access_mode) {
    return;                     // nothing is changed
  }

  sg_host_sd(workstation)->access_mode = access_mode;

  if (access_mode == SD_WORKSTATION_SHARED_ACCESS) {
    xbt_fifo_free(sg_host_sd(workstation)->task_fifo);
    sg_host_sd(workstation)->task_fifo = NULL;
  } else {
	  sg_host_sd(workstation)->task_fifo = xbt_fifo_new();
  }
}

/**
 * \brief Return the list of mounted storages on a workstation.
 *
 * \param workstation a workstation
 * \return a dynar containing all mounted storages on the workstation
 */
xbt_dict_t SD_workstation_get_mounted_storage_list(SD_workstation_t workstation){
  return workstation->extension<simgrid::surf::Host>()->getMountedStorageList();
}

/**
 * \brief Return the list of mounted storages on a workstation.
 *
 * \param workstation a workstation
 * \return a dynar containing all mounted storages on the workstation
 */
xbt_dynar_t SD_workstation_get_attached_storage_list(SD_workstation_t workstation){
  return surf_host_get_attached_storage_list(workstation);
}

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

/* Returns whether a task can start now on a workstation*/
/*
  int __SD_workstation_can_start(SD_workstation_t workstation, SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert(workstation != NULL && task != NULL, "Invalid parameter");

  return !__SD_workstation_is_busy(workstation) &&
  (xbt_fifo_size(workstation->task_fifo) == 0) || xbt_fifo_get_first_item(workstation->task_fifo) == task);
  }
*/

/* Returns whether a workstation is busy. A workstation is busy is it is
 * in sequential mode and a task is running on it or the fifo is not empty.
 */
int __SD_workstation_is_busy(SD_workstation_t workstation)
{
  XBT_DEBUG
      ("Workstation '%s' access mode: '%s', current task: %s, fifo size: %d",
       SD_workstation_get_name(workstation),
       (sg_host_sd(workstation)->access_mode ==
        SD_WORKSTATION_SHARED_ACCESS) ? "SHARED" : "FIFO",
       (sg_host_sd(workstation)->current_task ?
        SD_task_get_name(sg_host_sd(workstation)->current_task)
        : "none"),
       (sg_host_sd(workstation)->task_fifo ? xbt_fifo_size(sg_host_sd(workstation)->task_fifo) :
        0));

  return sg_host_sd(workstation)->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS &&
      (sg_host_sd(workstation)->current_task != NULL
       || xbt_fifo_size(sg_host_sd(workstation)->task_fifo) > 0);
}

/* Destroys a workstation.
 */
void __SD_workstation_destroy(void *workstation)
{

  if (workstation==NULL)
	  return;
  SD_workstation_priv_t w;

  /* workstation->surf_workstation is freed by surf_exit and workstation->data is freed by the user */

  w = (SD_workstation_priv_t) workstation;

  if (w->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS) {
    xbt_fifo_free(w->task_fifo);
  }
  xbt_free(w);
}

/** 
 * \brief Returns the kind of the task currently running on a workstation
 * Only call this with sequential access mode set
 * \param workstation a workstation */
SD_task_t SD_workstation_get_current_task(SD_workstation_t workstation)
{
  xbt_assert(sg_host_sd(workstation)->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS,
              "Access mode must be set to SD_WORKSTATION_SEQUENTIAL_ACCESS"
              " to use this function");

  return (sg_host_sd(workstation)->current_task);
}

