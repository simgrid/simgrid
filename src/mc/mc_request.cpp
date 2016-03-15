/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>

#include <xbt/log.h>
#include <xbt/str.h>
#include <xbt/sysdep.h>
#include <xbt/dynar.h>

#include "src/mc/mc_request.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_smx.h"
#include "src/mc/mc_xbt.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_request, mc,
                                "Logging specific to MC (request)");

static char *pointer_to_string(void *pointer);
static char *buff_size_to_string(size_t size);

static inline
smx_synchro_t MC_get_comm(smx_simcall_t r)
{
  switch (r->call ) {
  case SIMCALL_COMM_WAIT:
    return simcall_comm_wait__get__comm(r);
  case SIMCALL_COMM_TEST:
    return simcall_comm_test__get__comm(r);
  default:
    return nullptr;
  }
}

static inline
smx_mailbox_t MC_get_rdv(smx_simcall_t r)
{
  switch(r->call) {
  case SIMCALL_COMM_ISEND:
    return simcall_comm_isend__get__rdv(r);
  case SIMCALL_COMM_IRECV:
    return simcall_comm_irecv__get__rdv(r);
  default:
    return nullptr;
  }
}

namespace simgrid {
namespace mc {

// Does half the job
static inline
bool request_depend_asymmetric(smx_simcall_t r1, smx_simcall_t r2)
{
  if (r1->call == SIMCALL_COMM_ISEND && r2->call == SIMCALL_COMM_IRECV)
    return false;

  if (r1->call == SIMCALL_COMM_IRECV && r2->call == SIMCALL_COMM_ISEND)
    return false;

  // Those are internal requests, we do not need indirection
  // because those objects are copies:
  smx_synchro_t synchro1 = MC_get_comm(r1);
  smx_synchro_t synchro2 = MC_get_comm(r2);

  if ((r1->call == SIMCALL_COMM_ISEND || r1->call == SIMCALL_COMM_IRECV)
      && r2->call == SIMCALL_COMM_WAIT) {

    smx_mailbox_t rdv = MC_get_rdv(r1);

    if (rdv != synchro2->comm.rdv_cpy
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->issuer != synchro2->comm.src_proc)
        && (r1->issuer != synchro2->comm.dst_proc)
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call == SIMCALL_COMM_ISEND)
        && (synchro2->comm.type == SIMIX_COMM_SEND)
        && (synchro2->comm.src_buff !=
            simcall_comm_isend__get__src_buff(r1))
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call == SIMCALL_COMM_IRECV)
        && (synchro2->comm.type == SIMIX_COMM_RECEIVE)
        && (synchro2->comm.dst_buff != simcall_comm_irecv__get__dst_buff(r1))
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * test call. */
  /*if(   (r1->call == SIMCALL_COMM_ISEND || r1->call == SIMCALL_COMM_IRECV)
     &&  r2->call == SIMCALL_COMM_TEST)
     return FALSE; */

  if (r1->call == SIMCALL_COMM_WAIT
      && (r2->call == SIMCALL_COMM_WAIT || r2->call == SIMCALL_COMM_TEST)
      && (synchro1->comm.src_proc == nullptr || synchro1->comm.dst_proc == NULL))
    return false;

  if (r1->call == SIMCALL_COMM_TEST &&
      (simcall_comm_test__get__comm(r1) == nullptr
       || synchro1->comm.src_buff == nullptr
       || synchro1->comm.dst_buff == nullptr))
    return false;

  if (r1->call == SIMCALL_COMM_TEST && r2->call == SIMCALL_COMM_WAIT
      && synchro1->comm.src_buff == synchro2->comm.src_buff
      && synchro1->comm.dst_buff == synchro2->comm.dst_buff)
    return false;

  if (r1->call == SIMCALL_COMM_WAIT && r2->call == SIMCALL_COMM_TEST
      && synchro1->comm.src_buff != nullptr
      && synchro1->comm.dst_buff != nullptr
      && synchro2->comm.src_buff != nullptr
      && synchro2->comm.dst_buff != nullptr
      && synchro1->comm.dst_buff != synchro2->comm.src_buff
      && synchro1->comm.dst_buff != synchro2->comm.dst_buff
      && synchro2->comm.dst_buff != synchro1->comm.src_buff)
    return false;

  return true;
}

// Those are MC_state_get_internal_request(state)
bool request_depend(smx_simcall_t r1, smx_simcall_t r2)
{
  if (simgrid::mc::reduction_mode == simgrid::mc::ReductionMode::none)
    return true;

  if (r1->issuer == r2->issuer)
    return false;

  /* Wait with timeout transitions are not considered by the independance theorem, thus we consider them as dependant with all other transitions */
  if ((r1->call == SIMCALL_COMM_WAIT && simcall_comm_wait__get__timeout(r1) > 0)
      || (r2->call == SIMCALL_COMM_WAIT
          && simcall_comm_wait__get__timeout(r2) > 0))
    return TRUE;

  if (r1->call != r2->call)
    return request_depend_asymmetric(r1, r2)
      && request_depend_asymmetric(r2, r1);

  // Those are internal requests, we do not need indirection
  // because those objects are copies:
  smx_synchro_t synchro1 = MC_get_comm(r1);
  smx_synchro_t synchro2 = MC_get_comm(r2);

  switch(r1->call) {
  case SIMCALL_COMM_ISEND:
    return simcall_comm_isend__get__rdv(r1) == simcall_comm_isend__get__rdv(r2);
  case SIMCALL_COMM_IRECV:
    return simcall_comm_irecv__get__rdv(r1) == simcall_comm_irecv__get__rdv(r2);
  case SIMCALL_COMM_WAIT:
    if (synchro1->comm.src_buff == synchro2->comm.src_buff
        && synchro1->comm.dst_buff == synchro2->comm.dst_buff)
      return false;
    else if (synchro1->comm.src_buff != nullptr
        && synchro1->comm.dst_buff != nullptr
        && synchro2->comm.src_buff != nullptr
        && synchro2->comm.dst_buff != nullptr
        && synchro1->comm.dst_buff != synchro2->comm.src_buff
        && synchro1->comm.dst_buff != synchro2->comm.dst_buff
        && synchro2->comm.dst_buff != synchro1->comm.src_buff)
      return false;
    else
      return true;
  default:
    return true;
  }
}

}
}

static char *pointer_to_string(void *pointer)
{

  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%p", pointer);

  return xbt_strdup("(verbose only)");
}

static char *buff_size_to_string(size_t buff_size)
{

  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%zu", buff_size);

  return xbt_strdup("(verbose only)");
}


char *simgrid::mc::request_to_string(smx_simcall_t req, int value, simgrid::mc::RequestType request_type)
{
  bool use_remote_comm = true;
  switch(request_type) {
  case simgrid::mc::RequestType::simix:
    use_remote_comm = true;
    break;
  case simgrid::mc::RequestType::executed:
  case simgrid::mc::RequestType::internal:
    use_remote_comm = false;
    break;
  }

  const char* type = nullptr;
  char *args = nullptr;

  smx_process_t issuer = MC_smx_simcall_get_issuer(req);

  switch (req->call) {

  case SIMCALL_COMM_ISEND: {
    type = "iSend";
    char* p = pointer_to_string(simcall_comm_isend__get__src_buff(req));
    char* bs = buff_size_to_string(simcall_comm_isend__get__src_buff_size(req));
    if (issuer->host)
      args =
          bprintf("src=(%lu)%s (%s), buff=%s, size=%s", issuer->pid,
                  MC_smx_process_get_host_name(issuer),
                  MC_smx_process_get_name(issuer),
                  p, bs);
    else
      args =
          bprintf("src=(%lu)%s, buff=%s, size=%s", issuer->pid,
                  MC_smx_process_get_name(issuer), p, bs);
    xbt_free(bs);
    xbt_free(p);
    break;
  }

  case SIMCALL_COMM_IRECV: {
    size_t* remote_size = simcall_comm_irecv__get__dst_buff_size(req);

    // size_t size = size_pointer ? *size_pointer : 0;
    size_t size = 0;
    if (remote_size)
      mc_model_checker->process().read_bytes(&size, sizeof(size),
        remote(remote_size));

    type = "iRecv";
    char* p = pointer_to_string(simcall_comm_irecv__get__dst_buff(req));
    char* bs = buff_size_to_string(size);
    if (issuer->host)
      args =
          bprintf("dst=(%lu)%s (%s), buff=%s, size=%s", issuer->pid,
                  MC_smx_process_get_host_name(issuer),
                  MC_smx_process_get_name(issuer),
                  p, bs);
    else
      args =
          bprintf("dst=(%lu)%s, buff=%s, size=%s", issuer->pid,
                  MC_smx_process_get_name(issuer),
                  p, bs);
    xbt_free(bs);
    xbt_free(p);
    break;
  }

  case SIMCALL_COMM_WAIT: {
    smx_synchro_t remote_act = simcall_comm_wait__get__comm(req);
    char* p;
    if (value == -1) {
      type = "WaitTimeout";
      p = pointer_to_string(remote_act);
      args = bprintf("comm=%s", p);
    } else {
      type = "Wait";
      p = pointer_to_string(remote_act);

      s_smx_synchro_t synchro;
      smx_synchro_t act;
      if (use_remote_comm) {
        mc_model_checker->process().read_bytes(&synchro,
          sizeof(synchro), remote(remote_act));
        act = &synchro;
      } else
        act = remote_act;

      smx_process_t src_proc = MC_smx_resolve_process(act->comm.src_proc);
      smx_process_t dst_proc = MC_smx_resolve_process(act->comm.dst_proc);
      args = bprintf("comm=%s [(%lu)%s (%s)-> (%lu)%s (%s)]", p,
                     src_proc ? src_proc->pid : 0,
                     src_proc ? MC_smx_process_get_host_name(src_proc) : "",
                     src_proc ? MC_smx_process_get_name(src_proc) : "",
                     dst_proc ? dst_proc->pid : 0,
                     dst_proc ? MC_smx_process_get_host_name(dst_proc) : "",
                     dst_proc ? MC_smx_process_get_name(dst_proc) : "");
    }
    xbt_free(p);
    break;
  }

  case SIMCALL_COMM_TEST: {
    smx_synchro_t remote_act = simcall_comm_test__get__comm(req);
    s_smx_synchro_t synchro;
      smx_synchro_t act;
      if (use_remote_comm) {
        mc_model_checker->process().read_bytes(&synchro,
          sizeof(synchro), remote(remote_act));
        act = &synchro;
      } else
        act = remote_act;

    char* p;
    if (act->comm.src_proc == nullptr || act->comm.dst_proc == NULL) {
      type = "Test FALSE";
      p = pointer_to_string(remote_act);
      args = bprintf("comm=%s", p);
    } else {
      type = "Test TRUE";
      p = pointer_to_string(remote_act);

      smx_process_t src_proc = MC_smx_resolve_process(act->comm.src_proc);
      smx_process_t dst_proc = MC_smx_resolve_process(act->comm.dst_proc);
      args = bprintf("comm=%s [(%lu)%s (%s) -> (%lu)%s (%s)]", p,
                     src_proc->pid,
                     MC_smx_process_get_name(src_proc),
                     MC_smx_process_get_host_name(src_proc),
                     dst_proc->pid,
                     MC_smx_process_get_name(dst_proc),
                     MC_smx_process_get_host_name(dst_proc));
    }
    xbt_free(p);
    break;
  }

  case SIMCALL_COMM_WAITANY: {
    type = "WaitAny";
    s_xbt_dynar_t comms;
    mc_model_checker->process().read_bytes(
      &comms, sizeof(comms), remote(simcall_comm_waitany__get__comms(req)));
    if (!xbt_dynar_is_empty(&comms)) {
      smx_synchro_t remote_sync;
      read_element(mc_model_checker->process(),
        &remote_sync, remote(simcall_comm_waitany__get__comms(req)), value,
        sizeof(remote_sync));
      char* p = pointer_to_string(remote_sync);
      args = bprintf("comm=%s (%d of %lu)",
        p, value + 1, xbt_dynar_length(&comms));
      xbt_free(p);
    } else
      args = bprintf("comm at idx %d", value);
    break;
  }

  case SIMCALL_COMM_TESTANY:
    if (value == -1) {
      type = "TestAny FALSE";
      args = xbt_strdup("-");
    } else {
      type = "TestAny";
      args =
          bprintf("(%d of %zu)", value + 1,
                  read_length(mc_model_checker->process(),
                    simcall_comm_testany__get__comms(req)));
    }
    break;

  case SIMCALL_MUTEX_TRYLOCK:
  case SIMCALL_MUTEX_LOCK: {
    if (req->call == SIMCALL_MUTEX_LOCK)
      type = "Mutex LOCK";
    else
      type = "Mutex TRYLOCK";

    s_smx_mutex_t mutex;
    mc_model_checker->process().read_bytes(&mutex, sizeof(mutex),
      remote(
        req->call == SIMCALL_MUTEX_LOCK
        ? simcall_mutex_lock__get__mutex(req)
        : simcall_mutex_trylock__get__mutex(req)
      ));
    s_xbt_swag_t mutex_sleeping;
    mc_model_checker->process().read_bytes(&mutex_sleeping, sizeof(mutex_sleeping),
      remote(mutex.sleeping));

    args = bprintf("locked = %d, owner = %d, sleeping = %d",
      mutex.locked,
      mutex.owner != nullptr ? (int) MC_smx_resolve_process(mutex.owner)->pid : -1,
      mutex_sleeping.count);
    break;
  }

  case SIMCALL_MC_RANDOM:
    type = "MC_RANDOM";
    args = bprintf("%d", value);
    break;

  default:
    THROW_UNIMPLEMENTED;
  }

  char* str;
  if (args != nullptr)
    str =
        bprintf("[(%lu)%s (%s)] %s(%s)", issuer->pid,
                MC_smx_process_get_host_name(issuer),
                MC_smx_process_get_name(issuer),
                type, args);
  else
    str =
        bprintf("[(%lu)%s (%s)] %s ", issuer->pid,
                MC_smx_process_get_host_name(issuer),
                MC_smx_process_get_name(issuer),
                type);
  xbt_free(args);
  return str;
}

namespace simgrid {
namespace mc {

bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx)
{
  smx_synchro_t remote_act = nullptr;
  switch (req->call) {

  case SIMCALL_COMM_WAIT:
    /* FIXME: check also that src and dst processes are not suspended */
    remote_act = simcall_comm_wait__get__comm(req);
    break;

  case SIMCALL_COMM_WAITANY: {
    read_element(
      mc_model_checker->process(), &remote_act,
      remote(simcall_comm_waitany__get__comms(req)),
      idx, sizeof(remote_act));
    }
    break;

  case SIMCALL_COMM_TESTANY: {
    read_element(
      mc_model_checker->process(), &remote_act,
      remote(simcall_comm_testany__get__comms(req)),
      idx, sizeof(remote_act));
    }
    break;

  default:
    return true;
  }

  s_smx_synchro_t synchro;
  mc_model_checker->process().read_bytes(
    &synchro, sizeof(synchro), remote(remote_act));
  return synchro.comm.src_proc && synchro.comm.dst_proc;
}

bool process_is_enabled(smx_process_t process)
{
  return simgrid::mc::request_is_enabled(&process->simcall);
}

static const char* colors[] = {
  "blue",
  "red",
  "green3",
  "goldenrod",
  "brown",
  "purple",
  "magenta",
  "turquoise4",
  "gray25",
  "forestgreen",
  "hotpink",
  "lightblue",
  "tan",
};

static inline const char* get_color(int id)
{
  return colors[id % (sizeof(colors) / sizeof(colors[0])) ];
}

char *request_get_dot_output(smx_simcall_t req, int value)
{
  char *label = nullptr;

  const smx_process_t issuer = MC_smx_simcall_get_issuer(req);

  switch (req->call) {
  case SIMCALL_COMM_ISEND:
    if (issuer->host)
      label =
          bprintf("[(%lu)%s] iSend", issuer->pid,
                  MC_smx_process_get_host_name(issuer));
    else
      label = bprintf("[(%lu)] iSend", issuer->pid);
    break;

  case SIMCALL_COMM_IRECV:
    if (issuer->host)
      label =
          bprintf("[(%lu)%s] iRecv", issuer->pid,
                  MC_smx_process_get_host_name(issuer));
    else
      label = bprintf("[(%lu)] iRecv", issuer->pid);
    break;

  case SIMCALL_COMM_WAIT: {
    if (value == -1) {
      if (issuer->host)
        label =
            bprintf("[(%lu)%s] WaitTimeout", issuer->pid,
                    MC_smx_process_get_host_name(issuer));
      else
        label = bprintf("[(%lu)] WaitTimeout", issuer->pid);
    } else {
      smx_synchro_t remote_act = simcall_comm_wait__get__comm(req);
      s_smx_synchro_t synchro;
      mc_model_checker->process().read_bytes(&synchro,
        sizeof(synchro), remote(remote_act));

      smx_process_t src_proc = MC_smx_resolve_process(synchro.comm.src_proc);
      smx_process_t dst_proc = MC_smx_resolve_process(synchro.comm.dst_proc);
      if (issuer->host)
        label =
            bprintf("[(%lu)%s] Wait [(%lu)->(%lu)]", issuer->pid,
                    MC_smx_process_get_host_name(issuer),
                    src_proc ? src_proc->pid : 0,
                    dst_proc ? dst_proc->pid : 0);
      else
        label =
            bprintf("[(%lu)] Wait [(%lu)->(%lu)]", issuer->pid,
                    src_proc ? src_proc->pid : 0,
                    dst_proc ? dst_proc->pid : 0);
    }
    break;
  }

  case SIMCALL_COMM_TEST: {
    smx_synchro_t remote_act = simcall_comm_test__get__comm(req);
    s_smx_synchro_t synchro;
    mc_model_checker->process().read_bytes(&synchro,
      sizeof(synchro), remote(remote_act));
    if (synchro.comm.src_proc == nullptr || synchro.comm.dst_proc == NULL) {
      if (issuer->host)
        label =
            bprintf("[(%lu)%s] Test FALSE", issuer->pid,
                    MC_smx_process_get_host_name(issuer));
      else
        label = bprintf("[(%lu)] Test FALSE", issuer->pid);
    } else {
      if (issuer->host)
        label =
            bprintf("[(%lu)%s] Test TRUE", issuer->pid,
                    MC_smx_process_get_host_name(issuer));
      else
        label = bprintf("[(%lu)] Test TRUE", issuer->pid);
    }
    break;
  }

  case SIMCALL_COMM_WAITANY: {
    unsigned long comms_size = read_length(
      mc_model_checker->process(), remote(simcall_comm_waitany__get__comms(req)));
    if (issuer->host)
      label =
          bprintf("[(%lu)%s] WaitAny [%d of %lu]", issuer->pid,
                  MC_smx_process_get_host_name(issuer), value + 1,
                  comms_size);
    else
      label =
          bprintf("[(%lu)] WaitAny [%d of %lu]", issuer->pid, value + 1,
                  comms_size);
    break;
  }

  case SIMCALL_COMM_TESTANY:
    if (value == -1) {
      if (issuer->host)
        label =
            bprintf("[(%lu)%s] TestAny FALSE", issuer->pid,
                    MC_smx_process_get_host_name(issuer));
      else
        label = bprintf("[(%lu)] TestAny FALSE", issuer->pid);
    } else {
      if (issuer->host)
        label =
            bprintf("[(%lu)%s] TestAny TRUE [%d of %lu]", issuer->pid,
                    MC_smx_process_get_host_name(issuer), value + 1,
                    xbt_dynar_length(simcall_comm_testany__get__comms(req)));
      else
        label =
            bprintf("[(%lu)] TestAny TRUE [%d of %lu]", issuer->pid,
                    value + 1,
                    xbt_dynar_length(simcall_comm_testany__get__comms(req)));
    }
    break;

  case SIMCALL_MUTEX_TRYLOCK:
    label = bprintf("[(%lu)] Mutex TRYLOCK", issuer->pid);
    break;

  case SIMCALL_MUTEX_LOCK:
    label = bprintf("[(%lu)] Mutex LOCK", issuer->pid);
    break;

  case SIMCALL_MC_RANDOM:
    if (issuer->host)
      label =
          bprintf("[(%lu)%s] MC_RANDOM (%d)", issuer->pid,
                  MC_smx_process_get_host_name(issuer), value);
    else
      label = bprintf("[(%lu)] MC_RANDOM (%d)", issuer->pid, value);
    break;

  default:
    THROW_UNIMPLEMENTED;
  }

  const char* color = get_color(issuer->pid - 1);
  char* str =
      bprintf("label = \"%s\", color = %s, fontcolor = %s", label,
              color, color);
  xbt_free(label);
  return str;

}

}
}
