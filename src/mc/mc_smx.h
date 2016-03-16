/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SMX_H
#define SIMGRID_MC_SMX_H

#include <stddef.h>

#include <xbt/base.h>
#include <xbt/log.h>

#include <simgrid/simix.h>

#include "src/smpi/private.h"

#include "src/mc/Process.hpp"
#include "src/mc/mc_protocol.h"

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
 *      (copy of `simix_global->process_to_destroy`);
 *
 *   - `model_checker->hostnames`.
 *
 * The process lists are currently refreshed each time MCed code is executed.
 * We don't try to give a persistent MCer address for a given MCed process.
 * For this reason, a MCer simgrid::mc::Process* is currently not reusable after
 * MCed code.
 */

SG_BEGIN_DECL()

XBT_PRIVATE void MC_process_smx_refresh(simgrid::mc::Process* process);

/** Get the issuer of  a simcall (`req->issuer`)
 *
 *  In split-process mode, it does the black magic necessary to get an address
 *  of a (shallow) copy of the data structure the issuer SIMIX process in the local
 *  address space.
 *
 *  @param process the MCed process
 *  @param req     the simcall (copied in the local process)
 */
XBT_PRIVATE smx_process_t MC_smx_simcall_get_issuer(smx_simcall_t req);

XBT_PRIVATE const char* MC_smx_process_get_name(smx_process_t p);
XBT_PRIVATE const char* MC_smx_process_get_host_name(smx_process_t p);

XBT_PRIVATE int MC_smpi_process_count(void);


/* ***** Resolve (local/MCer structure from remote/MCed addresses) ***** */

/** Get a local copy of the process from the process remote address */
XBT_PRIVATE smx_process_t MC_smx_resolve_process(smx_process_t process_remote_address);

/** Get the process info structure from the process remote address */
XBT_PRIVATE simgrid::mc::SimixProcessInformation* MC_smx_resolve_process_info(smx_process_t process_remote_address);

XBT_PRIVATE unsigned long MC_smx_get_maxpid(void);

SG_END_DECL()

#endif
