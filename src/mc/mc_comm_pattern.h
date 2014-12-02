/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/dynar.h>

#include "../simix/smx_private.h"

#ifndef MC_COMM_PATTERN_H
#define MC_COMM_PATTERN_H

SG_BEGIN_DECL()

typedef struct s_mc_comm_pattern{
  int num;
  smx_synchro_t comm;
  e_smx_comm_type_t type;
  unsigned long src_proc;
  unsigned long dst_proc;
  const char *src_host;
  const char *dst_host;
  char *rdv;
  ssize_t data_size;
  void *data;
} s_mc_comm_pattern_t, *mc_comm_pattern_t;

extern xbt_dynar_t initial_communications_pattern;
extern xbt_dynar_t communications_pattern;
extern xbt_dynar_t incomplete_communications_pattern;

// Can we use the SIMIX syscall for this?
typedef enum mc_call_type {
  MC_CALL_TYPE_NONE,
  MC_CALL_TYPE_SEND,
  MC_CALL_TYPE_RECV,
  MC_CALL_TYPE_WAIT,
  MC_CALL_TYPE_WAITANY,
} mc_call_type;

static inline mc_call_type mc_get_call_type(smx_simcall_t req)
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

void get_comm_pattern(xbt_dynar_t communications_pattern, smx_simcall_t request, mc_call_type call_type);
void mc_update_comm_pattern(mc_call_type call_type, smx_simcall_t request, int value, xbt_dynar_t current_pattern);
void complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm);
void MC_pre_modelcheck_comm_determinism(void);
void MC_modelcheck_comm_determinism(void);

SG_END_DECL()

#endif
