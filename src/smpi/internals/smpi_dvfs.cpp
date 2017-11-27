/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/log.h>

// FIXME: this plugin should be separated from the core
#include "simgrid/s4u/Host.hpp"
#include <simgrid/plugins/energy.h>
#include <simgrid/simix.h>

#include <smpi/smpi.h>

#include "src/internal_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_dvfs, smpi, "Logging specific to SMPI (experimental DVFS support)");

/**
 * \brief Return the speed of the processor (in flop/s) at a given pstate
 *
 * \param pstate_index pstate to test
 * \return Returns the processor speed associated with pstate_index
 */
double smpi_get_host_power_peak_at(int pstate_index)
{
  return sg_host_self()->getPstateSpeed(pstate_index);
}

/**
 * \brief Return the current speed of the processor (in flop/s)
 *
 * \return Returns the current processor speed
 */
double smpi_get_host_current_power_peak()
{
  return sg_host_self()->getSpeed();
}

/**
 * \brief Return the number of pstates defined for the current host
 */
int smpi_get_host_nb_pstates()
{
  return sg_host_get_nb_pstates(sg_host_self());
}

/**
 * \brief Sets the pstate at which the processor should run
 *
 * \param pstate_index pstate to switch to
 */
void smpi_set_host_pstate(int pstate_index)
{
  sg_host_set_pstate(sg_host_self(), pstate_index);
}
/** @brief Gets the pstate at which the processor currently running */
int smpi_get_host_pstate() {
  return sg_host_get_pstate(sg_host_self());
}

/**
 * \brief Return the total energy consumed by a host (in Joules)
 *
 * \return Returns the consumed energy
 */
double smpi_get_host_consumed_energy() {
  return sg_host_get_consumed_energy(sg_host_self());
}

#if SMPI_FORTRAN

#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__)
typedef int integer;
#else
typedef long int integer;
#endif
typedef char *address;
typedef float real;
typedef double doublereal;
struct complex {
  real r;
  real i;
};
struct doublecomplex {
  doublereal r;
  doublereal i;
};

extern "C" XBT_PUBLIC(doublereal) smpi_get_host_power_peak_at_(integer *pstate_index);
doublereal smpi_get_host_power_peak_at_(integer *pstate_index)
{
  return static_cast<doublereal>(smpi_get_host_power_peak_at((int)*pstate_index));
}

extern "C" XBT_PUBLIC(doublereal) smpi_get_host_current_power_peak_();
doublereal smpi_get_host_current_power_peak_()
{
  return smpi_get_host_current_power_peak();
}

extern "C" XBT_PUBLIC(integer) smpi_get_host_nb_pstates_();
integer smpi_get_host_nb_pstates_()
{
  return static_cast<integer>(smpi_get_host_nb_pstates());
}

extern "C" XBT_PUBLIC(void) smpi_set_host_pstate_(integer *pstate_index);
void smpi_set_host_pstate_(integer *pstate_index)
{
  smpi_set_host_pstate(static_cast<int>(*pstate_index));
}

extern "C" XBT_PUBLIC(doublereal) smpi_get_host_consumed_energy_();
doublereal smpi_get_host_consumed_energy_()
{
  return static_cast<doublereal>(smpi_get_host_consumed_energy());
}

#endif
