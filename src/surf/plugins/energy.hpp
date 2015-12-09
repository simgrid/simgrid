/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "../host_interface.hpp"
#include <map>

#ifndef ENERGY_CALLBACK_HPP_
#define ENERGY_CALLBACK_HPP_

namespace simgrid {
namespace energy {

class XBT_PRIVATE HostEnergy;

extern XBT_PRIVATE std::map<simgrid::surf::Host*, HostEnergy*> *surf_energy;

class HostEnergy {
public:
  HostEnergy(simgrid::surf::Host *ptr);
  ~HostEnergy();

  double getCurrentWattsValue(double cpu_load);
  double getConsumedEnergy();
  double getWattMinAt(int pstate);
  double getWattMaxAt(int pstate);

  xbt_dynar_t getWattsRangeList();
  xbt_dynar_t power_range_watts_list;   /*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
  double watts_off;                      /*< Consumption when the machine is turned off (shutdown) */
  double total_energy;					/*< Total energy consumed by the host */
  double last_updated;					/*< Timestamp of the last energy update event*/
  simgrid::surf::Host *host;

  void unref() {if (--refcount == 0) delete this;}
  void ref() {refcount++;}
  int refcount = 1;
};

}
}

#endif /* ENERGY_CALLBACK_HPP_ */
