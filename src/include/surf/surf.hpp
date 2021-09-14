/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURF_H
#define SURF_SURF_H

#include "simgrid/forward.h"

/*** SURF Globals **************************/

/** @ingroup SURF_simulation
 *  @brief Initialize SURF
 *  @param argc argument number
 *  @param argv arguments
 *
 *  This function has to be called to initialize the common structures. Then you will have to create the environment by
 *  calling  e.g. surf_host_model_init_CM02()
 *
 *  @see surf_host_model_init_CM02(), surf_host_model_init_compound()
 */
XBT_PUBLIC void surf_init(int* argc, char** argv); /* initialize common structures */

/** @ingroup SURF_simulation
 *  @brief Finish simulation initialization
 *
 *  This function must be called before the first call to surf_solve()
 */
XBT_PUBLIC void surf_presolve();

/** @ingroup SURF_simulation
 *  @brief Performs a part of the simulation
 *  @param max_date Maximum date to update the simulation to, or -1
 *  @return the elapsed time, or -1.0 if no event could be executed
 *
 *  This function execute all possible events, update the action states  and returns the time elapsed.
 *  When you call execute or communicate on a model, the corresponding actions are not executed immediately but only
 *  when you call surf_solve.
 *  Note that the returned elapsed time can be zero.
 */
XBT_PUBLIC double surf_solve(double max_date);

/** @ingroup SURF_simulation
 *  @brief Return the current time
 *
 *  Return the current time in millisecond.
 */
XBT_PUBLIC double surf_get_clock();

/* surf parse file related (public because called from a test suite) */
XBT_PUBLIC void parse_platform_file(const std::string& file);

#endif
