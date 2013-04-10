/* Copyright (c) 2008-2013 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_request, mc,
                                "Logging specific to MC (request)");

static char* pointer_to_string(void* pointer);
static char* buff_size_to_string(size_t size);

int MC_request_depend(smx_simcall_t r1, smx_simcall_t r2) {
  if(mc_reduce_kind == e_mc_reduce_none)
    return TRUE;

  if (r1->issuer == r2->issuer)
      return FALSE;

  if(r1->call == SIMCALL_COMM_ISEND && r2->call == SIMCALL_COMM_IRECV)
    return FALSE;

  if(r1->call == SIMCALL_COMM_IRECV && r2->call == SIMCALL_COMM_ISEND)
    return FALSE;

  if(   (r1->call == SIMCALL_COMM_ISEND || r1->call == SIMCALL_COMM_IRECV)
        &&  r2->call == SIMCALL_COMM_WAIT){

    if(simcall_comm_wait__get__comm(r2)->comm.rdv == NULL)
      return FALSE;

    smx_rdv_t rdv = r1->call == SIMCALL_COMM_ISEND ? simcall_comm_isend__get__rdv(r1) : simcall_comm_irecv__get__rdv(r1);

    if(simcall_comm_wait__get__comm(r2)->comm.rdv != rdv)
      return FALSE;

    if(simcall_comm_wait__get__comm(r2)->comm.type == SIMIX_COMM_SEND && r1->call == SIMCALL_COMM_ISEND)
      return FALSE;

    if(simcall_comm_wait__get__comm(r2)->comm.type == SIMIX_COMM_RECEIVE && r1->call == SIMCALL_COMM_IRECV)
      return FALSE;
  }

  if(   (r2->call == SIMCALL_COMM_ISEND || r2->call == SIMCALL_COMM_IRECV)
        &&  r1->call == SIMCALL_COMM_WAIT){

    if(simcall_comm_wait__get__comm(r1)->comm.rdv != NULL)
      return FALSE;

    smx_rdv_t rdv = r2->call == SIMCALL_COMM_ISEND ? simcall_comm_isend__get__rdv(r2) : simcall_comm_irecv__get__rdv(r2);

    if(simcall_comm_wait__get__comm(r1)->comm.rdv != rdv)
      return FALSE;

    if(simcall_comm_wait__get__comm(r1)->comm.type == SIMIX_COMM_SEND && r2->call == SIMCALL_COMM_ISEND)
      return FALSE;

    if(simcall_comm_wait__get__comm(r1)->comm.type == SIMIX_COMM_RECEIVE && r2->call == SIMCALL_COMM_IRECV)
      return FALSE;
  }

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * test call. */
  /*if(   (r1->call == SIMCALL_COMM_ISEND || r1->call == SIMCALL_COMM_IRECV)
        &&  r2->call == SIMCALL_COMM_TEST)
        return FALSE;*/

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * test call.*/
  /*if(   (r2->call == SIMCALL_COMM_ISEND || r2->call == SIMCALL_COMM_IRECV)
        && r1->call == SIMCALL_COMM_TEST)
        return FALSE;*/

  if(r1->call == SIMCALL_COMM_ISEND && r2->call == SIMCALL_COMM_ISEND
     && simcall_comm_isend__get__rdv(r1) != simcall_comm_isend__get__rdv(r2))
    return FALSE;

  if(r1->call == SIMCALL_COMM_IRECV && r2->call == SIMCALL_COMM_IRECV
     && simcall_comm_irecv__get__rdv(r1) != simcall_comm_irecv__get__rdv(r2))
    return FALSE;

  if(r1->call == SIMCALL_COMM_WAIT && (r2->call == SIMCALL_COMM_WAIT || r2->call == SIMCALL_COMM_TEST)
     && (simcall_comm_wait__get__comm(r1)->comm.src_proc == NULL
         || simcall_comm_wait__get__comm(r1)->comm.dst_proc == NULL))
    return FALSE;

  if(r2->call == SIMCALL_COMM_WAIT && (r1->call == SIMCALL_COMM_WAIT || r1->call == SIMCALL_COMM_TEST)
     && (simcall_comm_wait__get__comm(r2)->comm.src_proc == NULL
         || simcall_comm_wait__get__comm(r2)->comm.dst_proc == NULL))
    return FALSE;

  if(r1->call == SIMCALL_COMM_WAIT && r2->call == SIMCALL_COMM_WAIT
     && simcall_comm_wait__get__comm(r1)->comm.src_buff == simcall_comm_wait__get__comm(r2)->comm.src_buff
     && simcall_comm_wait__get__comm(r1)->comm.dst_buff == simcall_comm_wait__get__comm(r2)->comm.dst_buff)
    return FALSE;

  if (r1->call == SIMCALL_COMM_WAIT && r2->call == SIMCALL_COMM_WAIT
      && simcall_comm_wait__get__comm(r1)->comm.src_buff != NULL
      && simcall_comm_wait__get__comm(r1)->comm.dst_buff != NULL
      && simcall_comm_wait__get__comm(r2)->comm.src_buff != NULL
      && simcall_comm_wait__get__comm(r2)->comm.dst_buff != NULL
      && simcall_comm_wait__get__comm(r1)->comm.dst_buff != simcall_comm_wait__get__comm(r2)->comm.src_buff
      && simcall_comm_wait__get__comm(r1)->comm.dst_buff != simcall_comm_wait__get__comm(r2)->comm.dst_buff
      && simcall_comm_wait__get__comm(r2)->comm.dst_buff != simcall_comm_wait__get__comm(r1)->comm.src_buff)
    return FALSE;

  if(r1->call == SIMCALL_COMM_TEST &&
     (simcall_comm_test__get__comm(r1) == NULL
      || simcall_comm_test__get__comm(r1)->comm.src_buff == NULL
      || simcall_comm_test__get__comm(r1)->comm.dst_buff == NULL))
    return FALSE;

  if(r2->call == SIMCALL_COMM_TEST &&
     (simcall_comm_test__get__comm(r2) == NULL
      || simcall_comm_test__get__comm(r2)->comm.src_buff == NULL
      || simcall_comm_test__get__comm(r2)->comm.dst_buff == NULL))
    return FALSE;

  if(r1->call == SIMCALL_COMM_TEST && r2->call == SIMCALL_COMM_WAIT
     && simcall_comm_test__get__comm(r1)->comm.src_buff == simcall_comm_wait__get__comm(r2)->comm.src_buff
     && simcall_comm_test__get__comm(r1)->comm.dst_buff == simcall_comm_wait__get__comm(r2)->comm.dst_buff)
    return FALSE;

  if(r1->call == SIMCALL_COMM_WAIT && r2->call == SIMCALL_COMM_TEST
     && simcall_comm_wait__get__comm(r1)->comm.src_buff == simcall_comm_test__get__comm(r2)->comm.src_buff
     && simcall_comm_wait__get__comm(r1)->comm.dst_buff == simcall_comm_test__get__comm(r2)->comm.dst_buff)
    return FALSE;

  if (r1->call == SIMCALL_COMM_WAIT && r2->call == SIMCALL_COMM_TEST
      && simcall_comm_wait__get__comm(r1)->comm.src_buff != NULL
      && simcall_comm_wait__get__comm(r1)->comm.dst_buff != NULL
      && simcall_comm_test__get__comm(r2)->comm.src_buff != NULL
      && simcall_comm_test__get__comm(r2)->comm.dst_buff != NULL
      && simcall_comm_wait__get__comm(r1)->comm.dst_buff != simcall_comm_test__get__comm(r2)->comm.src_buff
      && simcall_comm_wait__get__comm(r1)->comm.dst_buff != simcall_comm_test__get__comm(r2)->comm.dst_buff
      && simcall_comm_test__get__comm(r2)->comm.dst_buff != simcall_comm_wait__get__comm(r1)->comm.src_buff)
    return FALSE;

  if (r1->call == SIMCALL_COMM_TEST && r2->call == SIMCALL_COMM_WAIT
      && simcall_comm_test__get__comm(r1)->comm.src_buff != NULL
      && simcall_comm_test__get__comm(r1)->comm.dst_buff != NULL
      && simcall_comm_wait__get__comm(r2)->comm.src_buff != NULL
      && simcall_comm_wait__get__comm(r2)->comm.dst_buff != NULL
      && simcall_comm_test__get__comm(r1)->comm.dst_buff != simcall_comm_wait__get__comm(r2)->comm.src_buff
      && simcall_comm_test__get__comm(r1)->comm.dst_buff != simcall_comm_wait__get__comm(r2)->comm.dst_buff
      && simcall_comm_wait__get__comm(r2)->comm.dst_buff != simcall_comm_test__get__comm(r1)->comm.src_buff)
    return FALSE;


  return TRUE;
}

static char* pointer_to_string(void* pointer) {

  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%p", pointer);

  return xbt_strdup("(verbose only)");
}

static char* buff_size_to_string(size_t buff_size) {

  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%zu", buff_size);

  return xbt_strdup("(verbose only)");
}


char *MC_request_to_string(smx_simcall_t req, int value)
{
  char *type = NULL, *args = NULL, *str = NULL, *p = NULL, *bs = NULL;
  smx_action_t act = NULL;
  size_t size = 0;

  switch(req->call){
    case SIMCALL_COMM_ISEND:
    type = xbt_strdup("iSend");
    p = pointer_to_string(simcall_comm_isend__get__src_buff(req));
    bs = buff_size_to_string(simcall_comm_isend__get__src_buff_size(req));
    args = bprintf("src=%s, buff=%s, size=%s", req->issuer->name, p, bs);
    break;
  case SIMCALL_COMM_IRECV:
    size = simcall_comm_irecv__get__dst_buff_size(req) ? *simcall_comm_irecv__get__dst_buff_size(req) : 0;
    type = xbt_strdup("iRecv");
    p = pointer_to_string(simcall_comm_irecv__get__dst_buff(req)); 
    bs = buff_size_to_string(size);
    args = bprintf("dst=%s, buff=%s, size=%s", req->issuer->name, p, bs);
    break;
  case SIMCALL_COMM_WAIT:
    act = simcall_comm_wait__get__comm(req);
    if(value == -1){
      type = xbt_strdup("WaitTimeout");
      p = pointer_to_string(act);
      args = bprintf("comm=%p", p);
    }else{
      type = xbt_strdup("Wait");
      p = pointer_to_string(act);
      args  = bprintf("comm=%s [(%lu)%s -> (%lu)%s]", p,
                      act->comm.src_proc ? act->comm.src_proc->pid : 0,
                      act->comm.src_proc ? act->comm.src_proc->name : "",
                      act->comm.dst_proc ? act->comm.dst_proc->pid : 0,
                      act->comm.dst_proc ? act->comm.dst_proc->name : "");
    }
    break;
  case SIMCALL_COMM_TEST:
    act = simcall_comm_test__get__comm(req);
    if(act->comm.src_proc == NULL || act->comm.dst_proc == NULL){
      type = xbt_strdup("Test FALSE");
      p = pointer_to_string(act);
      args = bprintf("comm=%s", p);
    }else{
      type = xbt_strdup("Test TRUE");
      p = pointer_to_string(act);
      args  = bprintf("comm=%s [(%lu)%s -> (%lu)%s]", p,
                      act->comm.src_proc->pid, act->comm.src_proc->name,
                      act->comm.dst_proc->pid, act->comm.dst_proc->name);
    }
    break;

  case SIMCALL_COMM_WAITANY:
    type = xbt_strdup("WaitAny");
    p = pointer_to_string(xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), value, smx_action_t));
    args = bprintf("comm=%s (%d of %lu)", p,
                   value+1, xbt_dynar_length(simcall_comm_waitany__get__comms(req)));
    break;

  case SIMCALL_COMM_TESTANY:
    if(value == -1){
      type = xbt_strdup("TestAny FALSE");
      args = xbt_strdup("-");
    }else{
      type = xbt_strdup("TestAny");
      args = bprintf("(%d of %lu)", value+1, xbt_dynar_length(simcall_comm_testany__get__comms(req)));
    }
    break;

  case SIMCALL_MC_SNAPSHOT:
    type = xbt_strdup("MC_SNAPSHOT");
    args = '\0';
    break;

  case SIMCALL_MC_COMPARE_SNAPSHOTS:
    type = xbt_strdup("MC_COMPARE_SNAPSHOTS");
    args = '\0';
    break;

  case SIMCALL_MC_RANDOM:
    type = xbt_strdup("MC_RANDOM");
    args = bprintf("%d", value);
    break;

  default:
    THROW_UNIMPLEMENTED;
  }

  if(args != NULL){
    str = bprintf("[(%lu)%s] %s (%s)", req->issuer->pid ,req->issuer->name, type, args);
    xbt_free(args);
  }else{
    str = bprintf("[(%lu)%s] %s ", req->issuer->pid ,req->issuer->name, type);
  }
  xbt_free(type);
  xbt_free(p);
  xbt_free(bs);
  return str;
}

unsigned int MC_request_testany_fail(smx_simcall_t req)
{
  unsigned int cursor;
  smx_action_t action;

  xbt_dynar_foreach(simcall_comm_testany__get__comms(req), cursor, action){
    if(action->comm.src_proc && action->comm.dst_proc)
      return FALSE;
  }

  return TRUE;
}

int MC_request_is_visible(smx_simcall_t req)
{
  return req->call == SIMCALL_COMM_ISEND
    || req->call == SIMCALL_COMM_IRECV
    || req->call == SIMCALL_COMM_WAIT
    || req->call == SIMCALL_COMM_WAITANY
    || req->call == SIMCALL_COMM_TEST
    || req->call == SIMCALL_COMM_TESTANY
    || req->call == SIMCALL_MC_RANDOM
    || req->call == SIMCALL_MC_SNAPSHOT
    || req->call == SIMCALL_MC_COMPARE_SNAPSHOTS;
}

int MC_request_is_enabled(smx_simcall_t req)
{
  unsigned int index = 0;
  smx_action_t act;

  switch (req->call) {

  case SIMCALL_COMM_WAIT:
    /* FIXME: check also that src and dst processes are not suspended */

    /* If it has a timeout it will be always be enabled, because even if the
     * communication is not ready, it can timeout and won't block.
     * On the other hand if it hasn't a timeout, check if the comm is ready.*/
    if(simcall_comm_wait__get__timeout(req) >= 0){
      if(_sg_mc_timeout == 1){
        return TRUE;
      }else{
        act = simcall_comm_wait__get__comm(req);
        return (act->comm.src_proc && act->comm.dst_proc);
      }
    }else{
      act = simcall_comm_wait__get__comm(req);
      return (act->comm.src_proc && act->comm.dst_proc);
    }
    break;

  case SIMCALL_COMM_WAITANY:
    /* Check if it has at least one communication ready */
    xbt_dynar_foreach(simcall_comm_waitany__get__comms(req), index, act) {
      if (act->comm.src_proc && act->comm.dst_proc){
        return TRUE;
      }
    }
    return FALSE;
    break;

  default:
    /* The rest of the request are always enabled */
    return TRUE;
  }
}

int MC_request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx)
{
  smx_action_t act;

  switch (req->call) {

  case SIMCALL_COMM_WAIT:
    /* FIXME: check also that src and dst processes are not suspended */
    act = simcall_comm_wait__get__comm(req);
    return (act->comm.src_proc && act->comm.dst_proc);
    break;

  case SIMCALL_COMM_WAITANY:
    act = xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), idx, smx_action_t);
    return (act->comm.src_proc && act->comm.dst_proc);
    break;

  case SIMCALL_COMM_TESTANY:
    act = xbt_dynar_get_as(simcall_comm_testany__get__comms(req), idx, smx_action_t);
    return (act->comm.src_proc && act->comm.dst_proc);
    break;

  default:
    return TRUE;
  }
}

int MC_process_is_enabled(smx_process_t process)
{
  if (process->simcall.call != SIMCALL_NONE && MC_request_is_enabled(&process->simcall))
    return TRUE;

  return FALSE;
}

char *MC_request_get_dot_output(smx_simcall_t req, int value){

  char *str = NULL, *label = NULL;
  smx_action_t act = NULL;

  switch(req->call){
  case SIMCALL_COMM_ISEND:
    label = xbt_strdup("iSend");
    break;
    
  case SIMCALL_COMM_IRECV:
    label = xbt_strdup("iRecv");
    break;
 
 case SIMCALL_COMM_WAIT:
    if(value == -1)
      label = xbt_strdup("WaitTimeout");
    else
      label = xbt_strdup("Wait");
    break;

  case SIMCALL_COMM_TEST:
    act = simcall_comm_test__get__comm(req);
    if(act->comm.src_proc == NULL || act->comm.dst_proc == NULL)
      label = xbt_strdup("Test FALSE");
    else
      label = xbt_strdup("Test TRUE");
    break;

  case SIMCALL_MC_RANDOM:
    if(value == 0)
      label = xbt_strdup("MC_RANDOM (0)");
    else
      label = xbt_strdup("MC_RANDOM (1)");
    break;

  case SIMCALL_MC_SNAPSHOT:
    label = xbt_strdup("MC_SNAPSHOT");
    break;

  case SIMCALL_MC_COMPARE_SNAPSHOTS:
    label = xbt_strdup("MC_COMPARE_SNAPSHOTS");
    break;

  default:
    THROW_UNIMPLEMENTED;
  }

  str = bprintf("label = \"%s\", color = %s", label, colors[req->issuer->pid-1]);
  xbt_free(label);
  return str;

}
