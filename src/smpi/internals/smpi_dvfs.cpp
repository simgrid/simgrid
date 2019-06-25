/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/log.h>

// FIXME: this plugin should be separated from the core
#include "simgrid/s4u/Host.hpp"
#include <simgrid/plugins/energy.h>
#include <simgrid/simix.h>

#include <smpi/smpi.h>

#include "src/internal_config.h"

#if SMPI_FORTRAN

// FIXME: rework these bindings, or kill them completely

#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__)
typedef int integer;
#else
typedef long int integer;
#endif
typedef double doublereal;

/**
 * @brief Return the speed of the processor (in flop/s) at a given pstate
 *
 * @param pstate_index pstate to test
 * @return Returns the processor speed associated with pstate_index
 */
extern "C" XBT_PUBLIC doublereal smpi_get_host_power_peak_at_(integer* pstate_index);
doublereal smpi_get_host_power_peak_at_(integer *pstate_index)
{
  return static_cast<doublereal>(sg_host_self()->get_pstate_speed(static_cast<int>(*pstate_index)));
}

/**
 * @brief Return the current speed of the processor (in flop/s)
 *
 * @return Returns the current processor speed
 */
extern "C" XBT_PUBLIC doublereal smpi_get_host_current_power_peak_();
doublereal smpi_get_host_current_power_peak_()
{
  return static_cast<doublereal>(sg_host_self()->get_speed());
}

/**
 * @brief Return the number of pstates defined for the current host
 */
extern "C" XBT_PUBLIC integer smpi_get_host_nb_pstates_();
integer smpi_get_host_nb_pstates_()
{
  return static_cast<integer>(sg_host_self()->get_pstate_count());
}

/**
 * @brief Sets the pstate at which the processor should run
 *
 * @param pstate_index pstate to switch to
 */
extern "C" XBT_PUBLIC void smpi_set_host_pstate_(integer* pstate_index);
void smpi_set_host_pstate_(integer *pstate_index)
{
  sg_host_set_pstate(sg_host_self(), (static_cast<int>(*pstate_index)));
}

/**
 * @brief Gets the pstate at which the processor currently running
 */
extern "C" XBT_PUBLIC integer smpi_get_host_pstate_();
integer smpi_get_host_pstate_()
{
  return static_cast<integer>(sg_host_get_pstate(sg_host_self()));
}
/**
 * @brief Return the total energy consumed by a host (in Joules)
 *
 * @return Returns the consumed energy
 */
extern "C" XBT_PUBLIC doublereal smpi_get_host_consumed_energy_();
doublereal smpi_get_host_consumed_energy_()
{
  return static_cast<doublereal>(sg_host_get_consumed_energy(sg_host_self()));
}

#endif
