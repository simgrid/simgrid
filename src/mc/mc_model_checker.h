/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
 
#ifndef MC_MODEL_CHECKER_H
#define MC_MODEL_CHECKER_H

#include <sys/types.h>

#include <simgrid_config.h>
#include <xbt/dynar.h>

#include "mc_forward.h"
#include "mc_process.h"
#include "mc_page_store.h"
#include "mc_protocol.h"

SG_BEGIN_DECL()

/** @brief State of the model-checker (global variables for the model checker)
 *
 *  Each part of the state of the model chercker represented as a global
 *  variable prevents some sharing between snapshots and must be ignored.
 *  By moving as much state as possible in this structure allocated
 *  on the model-chercker heap, we avoid those issues.
 */
struct s_mc_model_checker {
  // This is the parent snapshot of the current state:
  mc_snapshot_t parent_snapshot;
  mc_pages_store_t pages;
  int fd_clear_refs;
  int fd_pagemap;
  xbt_dynar_t record;
  s_mc_process_t process;
};

mc_model_checker_t MC_model_checker_new(pid_t pid, int socket);
void MC_model_checker_delete(mc_model_checker_t mc);

#define MC_EACH_SIMIX_PROCESS(process, code) \
  if (MC_process_is_self(&mc_model_checker->process)) { \
    xbt_swag_foreach(process, simix_global->process_list) { \
      code; \
    } \
  } else { \
    MC_process_refresh_simix_processes(&mc_model_checker->process); \
    unsigned int _smx_process_index; \
    mc_smx_process_info_t _smx_process_info; \
    xbt_dynar_foreach_ptr(mc_model_checker->process.smx_process_infos, _smx_process_index, _smx_process_info) { \
      smx_process_t process = &_smx_process_info->copy; \
      code; \
    } \
  }

SG_END_DECL()

#endif
