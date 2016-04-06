/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_COMM_PATTERN_H
#define SIMGRID_MC_COMM_PATTERN_H

#include <cstddef>
#include <cstring>

#include <string>
#include <vector>

#include <simgrid_config.h>
#include <xbt/dynar.h>

#include "src/simix/smx_private.h"
#include "src/smpi/private.h"
#include <smpi/smpi.h>

#include "src/mc/mc_state.h"

namespace simgrid {
namespace mc {

struct PatternCommunication {
  int num = 0;
  smx_synchro_t comm_addr;
  e_smx_comm_type_t type = SIMIX_COMM_SEND;
  unsigned long src_proc = 0;
  unsigned long dst_proc = 0;
  const char *src_host = nullptr;
  const char *dst_host = nullptr;
  std::string rdv;
  std::vector<char> data;
  int tag = 0;
  int index = 0;

  PatternCommunication()
  {
    std::memset(&comm_addr, 0, sizeof(comm_addr));
  }

  PatternCommunication dup() const
  {
    simgrid::mc::PatternCommunication res;
    // num?
    res.comm_addr = this->comm_addr;
    res.type = this->type;
    // src_proc?
    // dst_proc?
    res.dst_proc = this->dst_proc;
    res.dst_host = this->dst_host;
    res.rdv = this->rdv;
    res.data = this->data;
    // tag?
    res.index = this->index;
    return res;
  }

};

struct PatternCommunicationList {
  unsigned int index_comm = 0;
  xbt_dynar_t list = nullptr;

  PatternCommunicationList() {}
  ~PatternCommunicationList()
  {
    xbt_dynar_free(&(this->list));
  }
};

}
}

SG_BEGIN_DECL()

/**
 *  Type: `xbt_dynar_t<mc_list_comm_pattenr_t>`
 */
extern XBT_PRIVATE xbt_dynar_t initial_communications_pattern;

/**
 *  Type: `xbt_dynar_t<xbt_dynar_t<simgrid::mc::PatternCommunication*>>`
 */
extern XBT_PRIVATE xbt_dynar_t incomplete_communications_pattern;

typedef enum {
  MC_CALL_TYPE_NONE,
  MC_CALL_TYPE_SEND,
  MC_CALL_TYPE_RECV,
  MC_CALL_TYPE_WAIT,
  MC_CALL_TYPE_WAITANY,
} e_mc_call_type_t;

typedef enum {
  NONE_DIFF,
  TYPE_DIFF,
  RDV_DIFF,
  TAG_DIFF,
  SRC_PROC_DIFF,
  DST_PROC_DIFF,
  DATA_SIZE_DIFF,
  DATA_DIFF,
} e_mc_comm_pattern_difference_t;

static inline e_mc_call_type_t MC_get_call_type(smx_simcall_t req)
{
  switch(req->call) {
  case SIMCALL_COMM_ISEND:
    return MC_CALL_TYPE_SEND;
  case SIMCALL_COMM_IRECV:
    return MC_CALL_TYPE_RECV;
  case SIMCALL_COMM_WAIT:
    return MC_CALL_TYPE_WAIT;
  case SIMCALL_COMM_WAITANY:
    return MC_CALL_TYPE_WAITANY;
  default:
    return MC_CALL_TYPE_NONE;
  }
}

XBT_PRIVATE void MC_get_comm_pattern(xbt_dynar_t communications_pattern, smx_simcall_t request, e_mc_call_type_t call_type, int backtracking);
XBT_PRIVATE void MC_handle_comm_pattern(e_mc_call_type_t call_type, smx_simcall_t request, int value, xbt_dynar_t current_pattern, int backtracking);
XBT_PRIVATE void MC_complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm_addr, unsigned int issuer, int backtracking);

XBT_PRIVATE void MC_restore_communications_pattern(simgrid::mc::State* state);

XBT_PRIVATE void MC_state_copy_incomplete_communications_pattern(simgrid::mc::State* state);
XBT_PRIVATE void MC_state_copy_index_communications_pattern(simgrid::mc::State* state);

SG_END_DECL()

#endif
