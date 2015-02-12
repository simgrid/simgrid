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
  int index;
} s_mc_comm_pattern_t, *mc_comm_pattern_t;

typedef struct s_mc_list_comm_pattern{
  unsigned int index_comm;
  xbt_dynar_t list;
}s_mc_list_comm_pattern_t, *mc_list_comm_pattern_t;

extern xbt_dynar_t initial_communications_pattern;
extern xbt_dynar_t incomplete_communications_pattern;

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
  SRC_PROC_DIFF,
  DST_PROC_DIFF,
  DATA_SIZE_DIFF,
  DATA_DIFF,
} e_mc_comm_pattern_difference_t;

static inline e_mc_call_type_t mc_get_call_type(smx_simcall_t req)
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

void get_comm_pattern(xbt_dynar_t communications_pattern, smx_simcall_t request, e_mc_call_type_t call_type);
void handle_comm_pattern(e_mc_call_type_t call_type, smx_simcall_t request, int value, xbt_dynar_t current_pattern, int backtracking);
void comm_pattern_free_voidp(void *p);
void list_comm_pattern_free_voidp(void *p);
void complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm, int backtracking);
void MC_pre_modelcheck_comm_determinism(void);
void MC_modelcheck_comm_determinism(void);

SG_END_DECL()

#endif
