/* energy.hpp: internal interface to the energy plugin                      */

/* Copyright (c) 2014-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/surf/host_interface.hpp"
#include <map>

#ifndef ENERGY_CALLBACK_HPP_
#define ENERGY_CALLBACK_HPP_

namespace simgrid {
namespace energy {

class XBT_PRIVATE HostEnergy;

extern XBT_PRIVATE std::map<simgrid::surf::Host*, HostEnergy*> *surf_energy;

class HostEnergy {
public:
  typedef std::pair<double,double> power_range;

  HostEnergy(simgrid::surf::Host *ptr);
  ~HostEnergy();

  double getCurrentWattsValue(double cpu_load);
  double getConsumedEnergy();
  double getWattMinAt(int pstate);
  double getWattMaxAt(int pstate);

  void unref() {if (--refcount == 0) delete this;}
  void ref() {refcount++;}

private:
  void initWattsRangeList();
  int refcount = 1;
  simgrid::surf::Host *host;
  std::vector<power_range> power_range_watts_list;   /*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
public:
  double watts_off = 0.0; /*< Consumption when the machine is turned off (shutdown) */
  double total_energy = 0.0; /*< Total energy consumed by the host */
  double last_updated;       /*< Timestamp of the last energy update event*/
};

XBT_PUBLIC(double) surf_host_get_wattmin_at(sg_host_t resource, int pstate);
XBT_PUBLIC(double) surf_host_get_wattmax_at(sg_host_t resource, int pstate);
XBT_PUBLIC(double) surf_host_get_consumed_energy(sg_host_t host);
}
}



#endif /* ENERGY_CALLBACK_HPP_ */
