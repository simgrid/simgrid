/* energy.hpp: internal interface to the energy plugin                      */

/* Copyright (c) 2014-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include <map>

#include "src/surf/HostImpl.hpp"

#ifndef ENERGY_CALLBACK_HPP_
#define ENERGY_CALLBACK_HPP_

namespace simgrid {
namespace energy {

class XBT_PRIVATE HostEnergy;

class HostEnergy {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> EXTENSION_ID;
  typedef std::pair<double,double> power_range;

  HostEnergy(simgrid::s4u::Host *ptr);
  ~HostEnergy();

  double getCurrentWattsValue(double cpu_load);
  double getConsumedEnergy();
  double getWattMinAt(int pstate);
  double getWattMaxAt(int pstate);
  void update();

private:
  void initWattsRangeList();
  simgrid::s4u::Host *host = nullptr;
  std::vector<power_range> power_range_watts_list;   /*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
public:
  double watts_off = 0.0; /*< Consumption when the machine is turned off (shutdown) */
  double total_energy = 0.0; /*< Total energy consumed by the host */
  double last_updated;       /*< Timestamp of the last energy update event*/
};

}
}

#endif /* ENERGY_CALLBACK_HPP_ */
