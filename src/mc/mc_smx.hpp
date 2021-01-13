/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SMX_HPP
#define SIMGRID_MC_SMX_HPP

#include "src/mc/remote/RemoteSimulation.hpp"

/** @file
 *  @brief (Cross-process, MCer/MCed) Access to SMX structures
 *
 *  We copy some C data structure from the MCed process in the MCer process.
 *  This is implemented by:
 *
 *   - `model_checker->process.smx_process_infos`
 *      (copy of `simix_global->process_list`);
 *
 *   - `model_checker->process.smx_old_process_infos`
 *      (copy of `simix_global->actors_to_destroy`);
 *
 *   - `model_checker->hostnames`.
 *
 * The process lists are currently refreshed each time MCed code is executed.
 * We don't try to give a persistent MCer address for a given MCed process.
 * For this reason, a MCer simgrid::mc::Process* is currently not reusable after
 * MCed code.
 */

/** Get the issuer of  a simcall (`req->issuer`)
 *
 *  In split-process mode, it does the black magic necessary to get an address
 *  of a (shallow) copy of the data structure the issuer SIMIX process in the local
 *  address space.
 *
 *  @param process the MCed process
 *  @param req     the simcall (copied in the local process)
 */
XBT_PRIVATE smx_actor_t MC_smx_simcall_get_issuer(s_smx_simcall const* req);

XBT_PRIVATE const char* MC_smx_actor_get_name(smx_actor_t p);
XBT_PRIVATE const char* MC_smx_actor_get_host_name(smx_actor_t p);

XBT_PRIVATE unsigned long MC_smx_get_maxpid();

#endif
