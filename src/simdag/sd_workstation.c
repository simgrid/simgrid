/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_workstation, sd,
                                "Logging specific to SimDag (workstation)");

/* Creates a workstation and registers it in SD.
 */
SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data)
{

  SD_workstation_t workstation;
  const char *name;
  SD_CHECK_INIT_DONE();
  xbt_assert0(surf_workstation != NULL, "surf_workstation is NULL !");

  workstation = xbt_new(s_SD_workstation_t, 1);
  workstation->surf_workstation = surf_workstation;
  workstation->data = data;     /* user data */
  workstation->access_mode = SD_WORKSTATION_SHARED_ACCESS;      /* default mode is shared */
  workstation->task_fifo = NULL;
  workstation->current_task = NULL;

  name = SD_workstation_get_name(workstation);
  xbt_dict_set(sd_global->workstations, name, workstation, __SD_workstation_destroy);   /* add the workstation to the dictionary */
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
SD_workstation_t SD_workstation_get_by_name(const char *name)
{
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
const SD_workstation_t *SD_workstation_get_list(void)
{

  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int i;

  SD_CHECK_INIT_DONE();
  xbt_assert0(SD_workstation_get_number() > 0, "There is no workstation!");

  if (sd_global->workstation_list == NULL) {    /* this is the first time the function is called */
    sd_global->workstation_list =
      xbt_new(SD_workstation_t, sd_global->workstation_count);

    i = 0;
    xbt_dict_foreach(sd_global->workstations, cursor, key, data) {
      sd_global->workstation_list[i++] = (SD_workstation_t) data;
    }
  }
  return sd_global->workstation_list;
}

/**
 * \brief Returns the number of workstations
 *
 * \return the number of existing workstations
 * \see SD_workstation_get_list()
 */
int SD_workstation_get_number(void)
{
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
void *SD_workstation_get_data(SD_workstation_t workstation)
{
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
void SD_workstation_set_data(SD_workstation_t workstation, void *data)
{
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
const char *SD_workstation_get_name(SD_workstation_t workstation)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_resource_name(workstation->surf_workstation);
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
  return xbt_dict_get_or_null(SD_workstation_get_properties(ws), name);
}


/**
 * \brief Returns a #xbt_dict_t consisting of the list of properties assigned to this workstation
 *
 * \param workstation a workstation
 * \return the dictionary containing the properties associated with the workstation
 */
xbt_dict_t SD_workstation_get_properties(SD_workstation_t workstation)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0((workstation != NULL), "Invalid parameters");

  return surf_workstation_model->extension.workstation.get_properties(workstation->surf_workstation);

}


/**
 * \brief Returns the route between two workstations
 *
 * Use SD_route_get_size() to know the array size.
 *
 * \param src a workstation
 * \param dst another workstation
 * \return a new array of \ref SD_link_t representating the route between these two workstations
 * \see SD_route_get_size(), SD_link_t
 */
const SD_link_t *SD_route_get_list(SD_workstation_t src, SD_workstation_t dst)
{
  void *surf_src;
  void *surf_dst;
  xbt_dynar_t surf_route;
  const char *link_name;
  void *surf_link;
  unsigned int cpt;

  SD_CHECK_INIT_DONE();

  if (sd_global->recyclable_route == NULL) {
    /* first run */
    sd_global->recyclable_route = xbt_new(SD_link_t, SD_link_get_number());
  }

  surf_src = &src->surf_workstation;
  surf_dst = &dst->surf_workstation;
  surf_route = surf_workstation_model->extension.workstation.get_route(
		  (void*)SD_workstation_get_name(src),(void*)SD_workstation_get_name(dst));

  if(surf_route == NULL)
  {
	  INFO0("PBLM1 surf_route == NULL");
  }

  xbt_dynar_foreach(surf_route, cpt, surf_link) {
    link_name = surf_resource_name(surf_link);
    sd_global->recyclable_route[cpt] =
      xbt_dict_get(sd_global->links, link_name);
  }
  INFO0("FIN");
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
  SD_CHECK_INIT_DONE();
  return xbt_dynar_length(surf_workstation_model->extension.workstation.get_route(
        src->surf_workstation,dst->surf_workstation));
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_model->extension.workstation.
    get_speed(workstation->surf_workstation, 1.0);
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return surf_workstation_model->extension.
    workstation.get_available_speed(workstation->surf_workstation);
}

/**
 * \brief Returns an approximative estimated time for the given computation amount on a workstation
 *
 * \param workstation a workstation
 * \param computation_amount the computation amount you want to evaluate (in flops)
 * \return an approximative astimated computation time for the given computation amount on this workstation (in seconds)
 */
double SD_workstation_get_computation_time(SD_workstation_t workstation,
                                           double computation_amount)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  xbt_assert0(computation_amount >= 0,
              "computation_amount must be greater than or equal to zero");
  return computation_amount / SD_workstation_get_power(workstation);
}

/**
 * \brief Returns the latency of the route between two workstations, i.e. the sum of all link latencies
 * between the workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the latency of the route between the two workstations (in seconds)
 * \see SD_route_get_current_bandwidth()
 */
double SD_route_get_current_latency(SD_workstation_t src,
                                    SD_workstation_t dst)
{

  const SD_link_t *links;
  int nb_links;
  double latency;
  int i;

  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  links = SD_route_get_list(src, dst);
  nb_links = SD_route_get_size(src, dst);
  latency = 0.0;

  for (i = 0; i < nb_links; i++) {
    latency += SD_link_get_current_latency(links[i]);
  }

  return latency;
}

/**
 * \brief Returns the bandwidth of the route between two workstations, i.e. the minimum link bandwidth of all
 * between the workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the bandwidth of the route between the two workstations (in bytes/second)
 * \see SD_route_get_current_latency()
 */
double SD_route_get_current_bandwidth(SD_workstation_t src,
                                      SD_workstation_t dst)
{

  const SD_link_t *links;
  int nb_links;
  double bandwidth;
  double min_bandwidth;
  int i;

  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");

  links = SD_route_get_list(src, dst);
  nb_links = SD_route_get_size(src, dst);
  bandwidth = min_bandwidth = -1.0;


  for (i = 0; i < nb_links; i++) {
    bandwidth = SD_link_get_current_bandwidth(links[i]);
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
 * \param communication_amount the communication amount you want to evaluate (in bytes)
 * \return an approximative astimated computation time for the given communication amount
 * between the workstations (in seconds)
 */
double SD_route_get_communication_time(SD_workstation_t src,
                                       SD_workstation_t dst,
                                       double communication_amount)
{


  /* total time = latency + transmission time of the slowest link
     transmission time of a link = communication amount / link bandwidth */

  const SD_link_t *links;
  int nb_links;
  double bandwidth, min_bandwidth;
  double latency;
  int i;

  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  xbt_assert0(communication_amount >= 0,
              "communication_amount must be greater than or equal to zero");



  if (communication_amount == 0.0)
    return 0.0;

  links = SD_route_get_list(src, dst);
  nb_links = SD_route_get_size(src, dst);
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

/**
 * \brief Returns the access mode of this workstation.
 *
 * \param workstation a workstation
 * \return the access mode for the tasks running on this workstation:
 * SD_WORKSTATION_SHARED_ACCESS or SD_WORKSTATION_SEQUENTIAL_ACCESS
 *
 * \see SD_workstation_set_access_mode(), e_SD_workstation_access_mode_t
 */
e_SD_workstation_access_mode_t SD_workstation_get_access_mode(SD_workstation_t
                                                              workstation)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  return workstation->access_mode;
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");

  if (access_mode == workstation->access_mode) {
    return;                     // nothing is changed
  }

  workstation->access_mode = access_mode;

  if (access_mode == SD_WORKSTATION_SHARED_ACCESS) {
    xbt_fifo_free(workstation->task_fifo);
    workstation->task_fifo = NULL;
  } else {
    workstation->task_fifo = xbt_fifo_new();
  }
}

/* Returns whether a task can start now on a workstation.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              *//*
                                                                                                                                                                                                                                                                                                                                                                                                   int __SD_workstation_can_start(SD_workstation_t workstation, SD_task_t task) {
                                                                                                                                                                                                                                                                                                                                                                                                   SD_CHECK_INIT_DONE();
                                                                                                                                                                                                                                                                                                                                                                                                   xbt_assert0(workstation != NULL && task != NULL, "Invalid parameter");

                                                                                                                                                                                                                                                                                                                                                                                                   return !__SD_workstation_is_busy(workstation) &&
                                                                                                                                                                                                                                                                                                                                                                                                   (xbt_fifo_size(workstation->task_fifo) == 0) || xbt_fifo_get_first_item(workstation->task_fifo) == task);
                                                                                                                                                                                                                                                                                                                                                                                                   }
                                                                                                                                                                                                                                                                                                                                                                                                 */

/* Returns whether a workstation is busy. A workstation is busy is it is
 * in sequential mode and a task is running on it or the fifo is not empty.
 */
int __SD_workstation_is_busy(SD_workstation_t workstation)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");

  DEBUG4
    ("Workstation '%s' access mode: '%s', current task: %s, fifo size: %d",
     SD_workstation_get_name(workstation),
     (workstation->access_mode ==
      SD_WORKSTATION_SHARED_ACCESS) ? "SHARED" : "FIFO",
     (workstation->current_task ? SD_task_get_name(workstation->current_task)
      : "none"),
     (workstation->task_fifo ? xbt_fifo_size(workstation->task_fifo) : 0));

  return workstation->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS &&
    (workstation->current_task != NULL
     || xbt_fifo_size(workstation->task_fifo) > 0);
}

/* Destroys a workstation.
 */
void __SD_workstation_destroy(void *workstation)
{

  SD_workstation_t w;

  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  /* workstation->surf_workstation is freed by surf_exit and workstation->data is freed by the user */

  w = (SD_workstation_t) workstation;

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
  SD_CHECK_INIT_DONE();
  xbt_assert0(workstation != NULL, "Invalid parameter");
  xbt_assert0(workstation->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS, 
	      "Access mode must be set to SD_WORKSTATION_SEQUENTIAL_ACCESS"
	      " to use this function");
  
  return (workstation->current_task);
}
