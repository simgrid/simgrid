/* Copyright (c) 2009-2011, 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <vector>

#include <xbt/signal.hpp>

#include <simgrid/s4u/host.hpp>

#include "src/surf/surf_routing.hpp"

#include "simgrid/sg_config.h"
#include "src/surf/storage_interface.hpp"

#include "src/kernel/routing/AsImpl.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

/**
 * @ingroup SURF_build_api
 * @brief A library containing all known hosts
 */

int MSG_FILE_LEVEL = -1;             //Msg file level

int SIMIX_STORAGE_LEVEL = -1;        //Simix storage level
int MSG_STORAGE_LEVEL = -1;          //Msg storage level

xbt_lib_t as_router_lib;
int ROUTING_ASR_LEVEL = -1;          //Routing level
int ROUTING_PROP_ASR_LEVEL = -1;     //Where the properties are stored

void sg_platf_new_trace(sg_platf_trace_cbarg_t trace)
{
  tmgr_trace_t tmgr_trace;
  if (trace->file && strcmp(trace->file, "") != 0) {
    tmgr_trace = tmgr_trace_new_from_file(trace->file);
  } else {
    xbt_assert(strcmp(trace->pc_data, ""),
        "Trace '%s' must have either a content, or point to a file on disk.",trace->id);
    tmgr_trace = tmgr_trace_new_from_string(trace->id, trace->pc_data, trace->periodicity);
  }
  xbt_dict_set(traces_set_list, trace->id, (void *) tmgr_trace, nullptr);
}
