/* Copyright (c) 2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "energy.hpp"
#include "../cpu_cas01.hpp"

/** @addtogroup SURF_plugin_energy
 *
 *
 *  BlaBla energy
 */

XBT_LOG_EXTERNAL_CATEGORY(surf_kernel);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_energy, surf,
                                "Logging specific to the SURF energy plugin");

std::map<CpuPtr, CpuEnergyPtr> *surf_energy=NULL;

static void energyCpuCreatedCallback(CpuPtr cpu){
  (*surf_energy)[cpu] = new CpuEnergy(cpu);
}

static void energyCpuDestructedCallback(CpuPtr cpu){
  std::map<CpuPtr, CpuEnergyPtr>::iterator cpuIt = surf_energy->find(cpu);
  xbt_assert(cpuIt != surf_energy->end(), "The cpu is not in surf_energy.");
  XBT_INFO("Total energy (Joules) of host %s: %f", cpu->getName(), cpuIt->second->getConsumedEnergy());
  delete cpuIt->second;
  surf_energy->erase(cpuIt);
}

static void energyCpuActionStateChangedCallback(CpuActionPtr action, e_surf_action_state_t old, e_surf_action_state_t cur){
  CpuPtr cpu  = getActionCpu(action);
  CpuEnergyPtr cpu_energy = (*surf_energy)[cpu];

  if(cpu_energy->last_updated < surf_get_clock()) {
   	double cpu_load = lmm_constraint_get_usage(cpu->getConstraint()) / cpu->m_powerPeak;
    double start_time = cpu_energy->last_updated;
    double finish_time = surf_get_clock();

    /*XBT_DEBUG("[cpu_update_energy] action time interval=(%f-%f), current power peak=%f, current pstate=%d",
  		  start_time, finish_time, cpu->m_powerPeak, cpu->m_pstate);*/
    XBT_DEBUG("[cpu_update_energy] action time interval=(%f-%f), current power peak=%f",
  		  start_time, finish_time, cpu->m_powerPeak);
    double current_energy = cpu_energy->total_energy;
    double action_energy = cpu_energy->getCurrentWattsValue(cpu_load)*(finish_time-start_time);

    cpu_energy->total_energy = current_energy + action_energy;
    cpu_energy->last_updated = finish_time;

    XBT_DEBUG("[cpu_update_energy] old_energy_value=%f, action_energy_value=%f", current_energy, action_energy);
  }
}

static void sg_energy_plugin_exit()
{
  delete surf_energy;
  surf_energy = NULL;
}

/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu.
 */
void sg_energy_plugin_init() {
  if (surf_energy == NULL) {
    surf_energy = new std::map<CpuPtr, CpuEnergyPtr>();
    surf_callback_connect(cpuCreatedCallbacks, energyCpuCreatedCallback);
    surf_callback_connect(cpuDestructedCallbacks, energyCpuDestructedCallback);
    surf_callback_connect(cpuActionStateChangedCallbacks, energyCpuActionStateChangedCallback);
    surf_callback_connect(surfExitCallbacks, sg_energy_plugin_exit);
  }
}

/**
 *
 */
CpuEnergy::CpuEnergy(CpuPtr ptr)
 : cpu(ptr)
{
  total_energy = 0;
  power_range_watts_list = getWattsRangeList();
  last_updated = surf_get_clock();
}

CpuEnergy::~CpuEnergy(){
  unsigned int iter;
  xbt_dynar_t power_tuple = NULL;
  xbt_dynar_foreach(power_range_watts_list, iter, power_tuple)
    xbt_dynar_free(&power_tuple);
  xbt_dynar_free(&power_range_watts_list);
}


/**
 * Computes the power consumed by the host according to the current pstate and processor load
 *
 */
double CpuEnergy::getCurrentWattsValue(double cpu_load)
{
	xbt_dynar_t power_range_list = power_range_watts_list;

	if (power_range_list == NULL)
	{
		XBT_DEBUG("No power range properties specified for host %s", cpu->getName());
		return 0;
	}
	/*xbt_assert(xbt_dynar_length(power_range_list) == xbt_dynar_length(cpu->p_powerPeakList),
						"The number of power ranges in the properties does not match the number of pstates for host %s",
						cpu->getName());*/

    /* retrieve the power values associated with the current pstate */
    xbt_dynar_t current_power_values = xbt_dynar_get_as(power_range_list, static_cast<CpuCas01Ptr>(cpu)->getPState(), xbt_dynar_t);

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    double min_power = xbt_dynar_get_as(current_power_values, 0, double);
    double max_power = xbt_dynar_get_as(current_power_values, 1, double);
    double power_slope = max_power - min_power;

    double current_power = min_power + cpu_load * power_slope;

	XBT_DEBUG("[get_current_watts] min_power=%f, max_power=%f, slope=%f", min_power, max_power, power_slope);
    XBT_DEBUG("[get_current_watts] Current power (watts) = %f, load = %f", current_power, cpu_load);

	return current_power;
}

double CpuEnergy::getConsumedEnergy()
{
  return total_energy;
}

xbt_dynar_t CpuEnergy::getWattsRangeList()
{
	xbt_dynar_t power_range_list;
	xbt_dynar_t power_tuple;
	int i = 0, pstate_nb=0;
	xbt_dynar_t current_power_values;
	double min_power, max_power;

	if (cpu->getProperties() == NULL)
		return NULL;

	char* all_power_values_str = (char*)xbt_dict_get_or_null(cpu->getProperties(), "power_per_state");

	if (all_power_values_str == NULL)
		return NULL;


	power_range_list = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
	xbt_dynar_t all_power_values = xbt_str_split(all_power_values_str, ",");

	pstate_nb = xbt_dynar_length(all_power_values);
	for (i=0; i< pstate_nb; i++)
	{
		/* retrieve the power values associated with the current pstate */
		current_power_values = xbt_str_split(xbt_dynar_get_as(all_power_values, i, char*), ":");
		xbt_assert(xbt_dynar_length(current_power_values) > 1,
				"Power properties incorrectly defined - could not retrieve min and max power values for host %s",
				cpu->getName());

		/* min_power corresponds to the idle power (cpu load = 0) */
		/* max_power is the power consumed at 100% cpu load       */
		min_power = atof(xbt_dynar_get_as(current_power_values, 0, char*));
		max_power = atof(xbt_dynar_get_as(current_power_values, 1, char*));

		power_tuple = xbt_dynar_new(sizeof(double), NULL);
		xbt_dynar_push_as(power_tuple, double, min_power);
		xbt_dynar_push_as(power_tuple, double, max_power);

		xbt_dynar_push_as(power_range_list, xbt_dynar_t, power_tuple);
		xbt_dynar_free(&current_power_values);
	}
	xbt_dynar_free(&all_power_values);
	return power_range_list;
}
