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

/** @brief Returns the route between two workstations
 *
 * Use SD_route_get_size() to know the array size.
 *
 * \param src a host
 * \param dst another host
 * \return an array of the \ref SD_link_t composing the route
 * \see SD_route_get_size(), SD_link_t
 */
SD_link_t *SD_route_get_list(sg_host_t src, sg_host_t dst)
{
  xbt_dynar_t surf_route;
  SD_link_t* list;
  void *surf_link;
  unsigned int cpt;
  surf_route = surf_host_model_get_route((surf_host_model_t)surf_host_model,
                                         src, dst);

  list = xbt_new(SD_link_t, xbt_dynar_length(surf_route));
  xbt_dynar_foreach(surf_route, cpt, surf_link) {
    list[cpt] = (SD_link_t)surf_link;
  }
  return list;
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
  xbt_dynar_t route = NULL;
  unsigned int cpt;
  double latency = 0;
  double bandwidth;
  double min_bandwidth = -1.0;
  SD_link_t link;

  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard,
                                    &route, &latency);

  xbt_dynar_foreach(route, cpt, link){
    bandwidth = sg_link_bandwidth(link);
    if (bandwidth < min_bandwidth || min_bandwidth == -1.0)
      min_bandwidth = bandwidth;
  }

  return min_bandwidth;
}
