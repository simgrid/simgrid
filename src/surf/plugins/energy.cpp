/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "energy.hpp"
#include "../cpu_cas01.hpp"
#include "../virtual_machine.hpp"

/** @addtogroup SURF_plugin_energy


This is the energy plugin, enabling to account not only for computation time,
but also for the dissipated energy in the simulated platform.

The energy consumption of a CPU depends directly of its current load. Specify that consumption in your platform file as follows:

\verbatim
<host id="HostA" power="100.0Mf" >
    <prop id="watt_per_state" value="100.0:200.0" />
    <prop id="watt_off" value="10" />
</host>
\endverbatim

The first property means that when your host is up and running, but without anything to do, it will dissipate 100 Watts.
If it's fully loaded, it will dissipate 200 Watts. If its load is at 50%, then it will dissipate 150 Watts.
The second property means that when your host is turned off, it will dissipate only 10 Watts (please note that these values are arbitrary).

If your CPU is using pstates, then you can provide one consumption interval per pstate.

\verbatim
<host id="HostB" power="100.0Mf,50.0Mf,20.0Mf" pstate="0" >
    <prop id="watt_per_state" value="95.0:200.0, 93.0:170.0, 90.0:150.0" />
    <prop id="watt_off" value="10" />
</host>
\endverbatim

That host has 3 levels of performance with the following performance: 100 Mflop/s, 50 Mflop/s or 20 Mflop/s.
It starts at pstate 0 (ie, at 100 Mflop/s). In this case, you have to specify one interval per pstate in the watt_per_state property.
In this example, the idle consumption is 95 Watts, 93 Watts and 90 Watts in each pstate while the CPU burn consumption are at 200 Watts,
170 Watts and 150 Watts respectively.

To change the pstate of a given CPU, use the following functions: #MSG_host_get_nb_pstates(), #MSG_host_set_pstate(), #MSG_host_get_power_peak_at().

To simulate the energy-related elements, first call the #sg_energy_plugin_init() before your #MSG_init(),
and then use the following function to retrieve the consumption of a given host: #MSG_host_get_consumed_energy().
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_energy, surf,
                                "Logging specific to the SURF energy plugin");

std::map<Host*, HostEnergy*> *surf_energy=NULL;

static void energyHostCreatedCallback(Host *host){
  (*surf_energy)[host] = new HostEnergy(host);
}

static void energyVMCreatedCallback(VirtualMachine* vm) {
  std::map<Host*, HostEnergy*>::iterator host_energy_it = surf_energy->find(vm->p_subWs);
  xbt_assert(host_energy_it != surf_energy->end(), "The host is not in surf_energy.");
  (*surf_energy)[vm] = host_energy_it->second;
  host_energy_it->second->ref(); // protect the HostEnergy from getting deleted too early
}

/* Computes the consumption so far.  Called lazily on need. */
static void update_consumption(Host *host, HostEnergy *host_energy) {
	double cpu_load = lmm_constraint_get_usage(host->p_cpu->getConstraint()) / host->p_cpu->m_speedPeak;
	double start_time = host_energy->last_updated;
	double finish_time = surf_get_clock();

	double previous_energy = host_energy->total_energy;

	double instantaneous_consumption;
	if (host->getState() == SURF_RESOURCE_OFF)
		instantaneous_consumption = host_energy->watts_off;
	else
		instantaneous_consumption = host_energy->getCurrentWattsValue(cpu_load);

	double energy_this_step = instantaneous_consumption*(finish_time-start_time);

	host_energy->total_energy = previous_energy + energy_this_step;
	host_energy->last_updated = finish_time;

	XBT_DEBUG("[cpu_update_energy] period=[%.2f-%.2f]; current power peak=%.0E flop/s; consumption change: %.2f J -> %.2f J",
  		  start_time, finish_time, host->p_cpu->m_speedPeak, previous_energy, energy_this_step);
}

static void energyHostDestructedCallback(Host *host){
  std::map<Host*, HostEnergy*>::iterator host_energy_it = surf_energy->find(host);
  xbt_assert(host_energy_it != surf_energy->end(), "The host is not in surf_energy.");

  HostEnergy *host_energy = host_energy_it->second;
  update_consumption(host, host_energy);

  if (host_energy_it->second->refcount == 1) // Don't display anything for virtual CPUs
	  XBT_INFO("Total energy of host %s: %f Joules", host->getName(), host_energy->getConsumedEnergy());
  host_energy_it->second->unref();
  surf_energy->erase(host_energy_it);
}

static void energyCpuActionStateChangedCallback(CpuAction *action, e_surf_action_state_t old, e_surf_action_state_t cur){
  const char *name = getActionCpu(action)->getName();
  Host *host = static_cast<Host*>(surf_host_resource_priv(sg_host_by_name(name)));

  HostEnergy *host_energy = (*surf_energy)[host];

  if(host_energy->last_updated < surf_get_clock())
	  update_consumption(host, host_energy);
}

static void energyStateChangedCallback(Host *host, e_surf_resource_state_t oldState, e_surf_resource_state_t newState){
  HostEnergy *host_energy = (*surf_energy)[host];

  if(host_energy->last_updated < surf_get_clock())
	  update_consumption(host, host_energy);
}

static void sg_energy_plugin_exit()
{
  delete surf_energy;
  surf_energy = NULL;
}

/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */
void sg_energy_plugin_init() {
  if (surf_energy == NULL) {
    surf_energy = new std::map<Host*, HostEnergy*>();
    surf_callback_connect(hostCreatedCallbacks, energyHostCreatedCallback);
    surf_callback_connect(VMCreatedCallbacks, energyVMCreatedCallback);
    surf_callback_connect(hostDestructedCallbacks, energyHostDestructedCallback);
    surf_callback_connect(cpuActionStateChangedCallbacks, energyCpuActionStateChangedCallback);
    surf_callback_connect(surfExitCallbacks, sg_energy_plugin_exit);
    surf_callback_connect(hostStateChangedCallbacks, energyStateChangedCallback);
  }
}

/**
 *
 */
HostEnergy::HostEnergy(Host *ptr)
{
  host = ptr;
  total_energy = 0;
  power_range_watts_list = getWattsRangeList();
  last_updated = surf_get_clock();

  if (host->getProperties() != NULL) {
	char* off_power_str = (char*)xbt_dict_get_or_null(host->getProperties(), "watt_off");
	if (off_power_str != NULL)
		watts_off = atof(off_power_str);
	else
		watts_off = 0;
  }

}

HostEnergy::~HostEnergy(){
  unsigned int iter;
  xbt_dynar_t power_tuple = NULL;
  xbt_dynar_foreach(power_range_watts_list, iter, power_tuple)
    xbt_dynar_free(&power_tuple);
  xbt_dynar_free(&power_range_watts_list);
}


double HostEnergy::getWattMinAt(int pstate) {
  xbt_dynar_t power_range_list = power_range_watts_list;
  xbt_assert(power_range_watts_list, "No power range properties specified for host %s", host->getName());
  xbt_dynar_t current_power_values = xbt_dynar_get_as(power_range_list, static_cast<CpuCas01*>(host->p_cpu)->getPState(), xbt_dynar_t);
  double min_power = xbt_dynar_get_as(current_power_values, 0, double);
  return min_power;
}
double HostEnergy::getWattMaxAt(int pstate) {
  xbt_dynar_t power_range_list = power_range_watts_list;
  xbt_assert(power_range_watts_list, "No power range properties specified for host %s", host->getName());
  xbt_dynar_t current_power_values = xbt_dynar_get_as(power_range_list, static_cast<CpuCas01*>(host->p_cpu)->getPState(), xbt_dynar_t);
  double max_power = xbt_dynar_get_as(current_power_values, 1, double);
  return max_power;
}

/**
 * Computes the power consumed by the host according to the current pstate and processor load
 *
 */
double HostEnergy::getCurrentWattsValue(double cpu_load)
{
	xbt_dynar_t power_range_list = power_range_watts_list;
	xbt_assert(power_range_watts_list, "No power range properties specified for host %s", host->getName());

    /* retrieve the power values associated with the current pstate */
    xbt_dynar_t current_power_values = xbt_dynar_get_as(power_range_list, static_cast<CpuCas01*>(host->p_cpu)->getPState(), xbt_dynar_t);

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

double HostEnergy::getConsumedEnergy()
{

	if(last_updated < surf_get_clock())
		update_consumption(host, this);
	return total_energy;

}

xbt_dynar_t HostEnergy::getWattsRangeList()
{
	xbt_dynar_t power_range_list;
	xbt_dynar_t power_tuple;
	int i = 0, pstate_nb=0;
	xbt_dynar_t current_power_values;
	double min_power, max_power;

	if (host->getProperties() == NULL)
		return NULL;

	char* all_power_values_str = (char*)xbt_dict_get_or_null(host->getProperties(), "watt_per_state");

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
				host->getName());

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
