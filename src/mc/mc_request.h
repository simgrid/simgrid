/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REQUEST_H
#define SIMGRID_MC_REQUEST_H

#include <xbt/base.h>

#include <simgrid_config.h>

#include "src/simix/smx_private.h"

SG_BEGIN_DECL()

typedef enum e_mc_request_type {
  MC_REQUEST_SIMIX,
  MC_REQUEST_EXECUTED,
  MC_REQUEST_INTERNAL,
} e_mc_request_type_t;

XBT_PRIVATE int MC_request_depend(smx_simcall_t req1, smx_simcall_t req2);
XBT_PRIVATE char* MC_request_to_string(smx_simcall_t req, int value, e_mc_request_type_t type);
XBT_PRIVATE unsigned int MC_request_testany_fail(smx_simcall_t req);
/* XBT_PRIVATE int MC_waitany_is_enabled_by_comm(smx_req_t req, unsigned int comm);*/
XBT_PRIVATE int MC_request_is_visible(smx_simcall_t req);

/** Can this requests can be executed.
 *
 *  Most requests are always enabled but WAIT and WAITANY
 *  are not always enabled: a WAIT where the communication does not
 *  have both a source and a destination yet is not enabled
 *  (unless timeout is enabled in the wait and enabeld in SimGridMC).
 */
XBT_PRIVATE int MC_request_is_enabled(smx_simcall_t req);
XBT_PRIVATE int MC_request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx);

/** Is the process ready to execute its simcall?
 *
 *  This is true if the request associated with the process is ready.
 */
XBT_PRIVATE int MC_process_is_enabled(smx_process_t process);

XBT_PRIVATE char *MC_request_get_dot_output(smx_simcall_t req, int value);

SG_END_DECL()

#endif
