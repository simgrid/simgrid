/* Copyright (c) 2006-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simdag/simdag_private.h"
#include "simgrid/s4u/host.hpp"
#include "src/surf/HostImpl.hpp"
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
  std::vector<Link*> *route = new std::vector<Link*>();
  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard, route, NULL);

  int cpt=0;
  SD_link_t *list = xbt_new(SD_link_t, route->size());
  for (auto link : *route)
    list[cpt++] = link;

  delete route;
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
  std::vector<Link*> *route = new std::vector<Link*>();
  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard, route, NULL);
  int size = route->size();
  delete route;
  return size;
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
  double latency = 0;
  std::vector<Link*> *route = new std::vector<Link*>();
  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard, route, &latency);
  delete route;

  return latency;
}

/**
 * \brief Returns the bandwidth of the route between two workstations,
 * i.e. the minimum link bandwidth of all between the workstations.
 *
 * \param src the first workstation
 * \param dst the second workstation
 * \return the bandwidth of the route between the two workstations (in bytes/second)
 * \see SD_route_get_latency()
 */
double SD_route_get_bandwidth(sg_host_t src, sg_host_t dst)
{
  double min_bandwidth = -1.0;

  std::vector<Link*> *route = new std::vector<Link*>();
  routing_platf->getRouteAndLatency(src->pimpl_netcard, dst->pimpl_netcard, route, NULL);

  for (auto link : *route) {
    double bandwidth = sg_link_bandwidth(link);
    if (bandwidth < min_bandwidth || min_bandwidth == -1.0)
      min_bandwidth = bandwidth;
  }
  delete route;

  return min_bandwidth;
}
