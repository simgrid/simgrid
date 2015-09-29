/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_COMM_PATTERN_H
#define SIMGRID_MC_COMM_PATTERN_H

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/dynar.h>

#include "../simix/smx_private.h"
#include "../smpi/private.h"
#include <smpi/smpi.h>

#include "mc_state.h"

SG_BEGIN_DECL()

typedef struct s_mc_comm_pattern{
  int num;
  smx_synchro_t comm_addr;
  e_smx_comm_type_t type;
  unsigned long src_proc;
  unsigned long dst_proc;
  const char *src_host;
  const char *dst_host;
  char *rdv;
  ssize_t data_size;
  void *data;
  int tag;
  int index;
} s_mc_comm_pattern_t, *mc_comm_pattern_t;

typedef struct s_mc_list_comm_pattern{
  unsigned int index_comm;
  xbt_dynar_t list;
}s_mc_list_comm_pattern_t, *mc_list_comm_pattern_t;

/**
 *  Type: `xbt_dynar_t<mc_list_comm_pattenr_t>`
 */
extern XBT_PRIVATE xbt_dynar_t initial_communications_pattern;

/**
 *  Type: `xbt_dynar_t<xbt_dynar_t<mc_comm_pattern_t>>`
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
XBT_PRIVATE void MC_comm_pattern_free_voidp(void *p);
XBT_PRIVATE void MC_list_comm_pattern_free_voidp(void *p);
XBT_PRIVATE void MC_complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm_addr, unsigned int issuer, int backtracking);
void MC_modelcheck_comm_determinism(void);

XBT_PRIVATE void MC_restore_communications_pattern(mc_state_t state);

XBT_PRIVATE mc_comm_pattern_t MC_comm_pattern_dup(mc_comm_pattern_t comm);
XBT_PRIVATE xbt_dynar_t MC_comm_patterns_dup(xbt_dynar_t state);

XBT_PRIVATE void MC_state_copy_incomplete_communications_pattern(mc_state_t state);
XBT_PRIVATE void MC_state_copy_index_communications_pattern(mc_state_t state);

XBT_PRIVATE void MC_comm_pattern_free(mc_comm_pattern_t p);

SG_END_DECL()

#endif
