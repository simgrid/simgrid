/*
 * callback.hpp
 *
 *  Created on: Jan 10, 2014
 *      Author: bedaride
 */
#include "../cpu_interface.hpp"
#include <map>

#ifndef CALLBACK_HPP_
#define CALLBACK_HPP_

class CpuEnergy;
typedef CpuEnergy *CpuEnergyPtr;

extern std::map<CpuPtr, CpuEnergyPtr> *surf_energy;

class CpuEnergy {
public:
  CpuEnergy(CpuPtr ptr);
  ~CpuEnergy();

  double getCurrentWattsValue(double cpu_load);
  double getConsumedEnergy();
  xbt_dynar_t getWattsRangeList();

  xbt_dynar_t power_range_watts_list;		/*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
  double total_energy;					/*< Total energy consumed by the host */
  double last_updated;					/*< Timestamp of the last energy update event*/
  CpuPtr cpu;
};

#endif /* CALLBACK_HPP_ */
