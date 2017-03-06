/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/config.hpp>
#include <algorithm>

#include "private.h"
#include "xbt/virtu.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include "xbt/replay.h"
#include <errno.h>
#include "src/simix/smx_private.h"
#include "surf/surf.h"
#include "simgrid/sg_config.h"
#include "smpi/smpi_utils.hpp"
#include "colls/colls.h"
#include <simgrid/s4u/host.hpp>

#include "src/kernel/activity/SynchroComm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_base, smpi, "Logging specific to SMPI (base)");

extern void (*smpi_comm_copy_data_callback) (smx_activity_t, void*, size_t);


static int match_recv(void* a, void* b, smx_activity_t ignored) {
  MPI_Request ref = static_cast<MPI_Request>(a);
  MPI_Request req = static_cast<MPI_Request>(b);
  XBT_DEBUG("Trying to match a recv of src %d against %d, tag %d against %d",ref->src,req->src, ref->tag, req->tag);

  xbt_assert(ref, "Cannot match recv against null reference");
  xbt_assert(req, "Cannot match recv against null request");
  if((ref->src == MPI_ANY_SOURCE || req->src == ref->src)
    && ((ref->tag == MPI_ANY_TAG && req->tag >=0) || req->tag == ref->tag)){
    //we match, we can transfer some values
    if(ref->src == MPI_ANY_SOURCE)
      ref->real_src = req->src;
    if(ref->tag == MPI_ANY_TAG)
      ref->real_tag = req->tag;
    if(ref->real_size < req->real_size) 
      ref->truncated = 1;
    if(req->detached==1)
      ref->detached_sender=req; //tie the sender to the receiver, as it is detached and has to be freed in the receiver
    XBT_DEBUG("match succeeded");
    return 1;
  }else return 0;
}

static int match_send(void* a, void* b,smx_activity_t ignored) {
  MPI_Request ref = static_cast<MPI_Request>(a);
  MPI_Request req = static_cast<MPI_Request>(b);
  XBT_DEBUG("Trying to match a send of src %d against %d, tag %d against %d",ref->src,req->src, ref->tag, req->tag);
  xbt_assert(ref, "Cannot match send against null reference");
  xbt_assert(req, "Cannot match send against null request");

  if((req->src == MPI_ANY_SOURCE || req->src == ref->src)
      && ((req->tag == MPI_ANY_TAG && ref->tag >=0)|| req->tag == ref->tag)){
    if(req->src == MPI_ANY_SOURCE)
      req->real_src = ref->src;
    if(req->tag == MPI_ANY_TAG)
      req->real_tag = ref->tag;
    if(req->real_size < ref->real_size)
      req->truncated = 1;
    if(ref->detached==1)
      req->detached_sender=ref; //tie the sender to the receiver, as it is detached and has to be freed in the receiver
    XBT_DEBUG("match succeeded");
    return 1;
  } else
    return 0;
}

std::vector<s_smpi_factor_t> smpi_os_values;
std::vector<s_smpi_factor_t> smpi_or_values;
std::vector<s_smpi_factor_t> smpi_ois_values;

static simgrid::config::Flag<double> smpi_wtime_sleep(
  "smpi/wtime", "Minimum time to inject inside a call to MPI_Wtime", 0.0);
static simgrid::config::Flag<double> smpi_init_sleep(
  "smpi/init", "Time to inject inside a call to MPI_Init", 0.0);
static simgrid::config::Flag<double> smpi_iprobe_sleep(
  "smpi/iprobe", "Minimum time to inject inside a call to MPI_Iprobe", 1e-4);
static simgrid::config::Flag<double> smpi_test_sleep(
  "smpi/test", "Minimum time to inject inside a call to MPI_Test", 1e-4);


static double smpi_os(size_t size)
{
  if (smpi_os_values.empty()) {
    smpi_os_values = parse_factor(xbt_cfg_get_string("smpi/os"));
  }
  double current=smpi_os_values.empty()?0.0:smpi_os_values[0].values[0]+smpi_os_values[0].values[1]*size;
  // Iterate over all the sections that were specified and find the right
  // value. (fact.factor represents the interval sizes; we want to find the
  // section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto& fact : smpi_os_values) {
    if (size <= fact.factor) { // Values already too large, use the previously computed value of current!
      XBT_DEBUG("os : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    }else{
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current = fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("Searching for smpi/os: %zu is larger than the largest boundary, return %.10f", size, current);

  return current;
}

static double smpi_ois(size_t size)
{
  if (smpi_ois_values.empty()) {
    smpi_ois_values = parse_factor(xbt_cfg_get_string("smpi/ois"));
  }
  double current=smpi_ois_values.empty()?0.0:smpi_ois_values[0].values[0]+smpi_ois_values[0].values[1]*size;
  // Iterate over all the sections that were specified and find the right value. (fact.factor represents the interval
  // sizes; we want to find the section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto& fact : smpi_ois_values) {
    if (size <= fact.factor) { // Values already too large, use the previously  computed value of current!
      XBT_DEBUG("ois : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    }else{
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current = fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("Searching for smpi/ois: %zu is larger than the largest boundary, return %.10f", size, current);

  return current;
}

static double smpi_or(size_t size)
{
  if (smpi_or_values.empty()) {
    smpi_or_values = parse_factor(xbt_cfg_get_string("smpi/or"));
  }
  
  double current=smpi_or_values.empty()?0.0:smpi_or_values.front().values[0]+smpi_or_values.front().values[1]*size;

  // Iterate over all the sections that were specified and find the right value. (fact.factor represents the interval
  // sizes; we want to find the section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto fact : smpi_or_values) {
    if (size <= fact.factor) { // Values already too large, use the previously computed value of current!
      XBT_DEBUG("or : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    } else {
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current=fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("smpi_or: %zu is larger than largest boundary, return %.10f", size, current);

  return current;
}

void smpi_mpi_init() {
  if(smpi_init_sleep > 0) 
    simcall_process_sleep(smpi_init_sleep);
}

double smpi_mpi_wtime(){
  double time;
  if (smpi_process_initialized() != 0 && smpi_process_finalized() == 0 && smpi_process_get_sampling() == 0) {
    smpi_bench_end();
    time = SIMIX_get_clock();
    // to avoid deadlocks if used as a break condition, such as
    //     while (MPI_Wtime(...) < time_limit) {
    //       ....
    //     }
    // because the time will not normally advance when only calls to MPI_Wtime
    // are made -> deadlock (MPI_Wtime never reaches the time limit)
    if(smpi_wtime_sleep > 0) 
      simcall_process_sleep(smpi_wtime_sleep);
    smpi_bench_begin();
  } else {
    time = SIMIX_get_clock();
  }
  return time;
}

static MPI_Request build_request(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,
                                 unsigned flags)
{
  MPI_Request request = nullptr;

  void *old_buf = nullptr;

  request = xbt_new(s_smpi_mpi_request_t, 1);

  s_smpi_subtype_t *subtype = static_cast<s_smpi_subtype_t*>(datatype->substruct);

  if((((flags & RECV) != 0) && ((flags & ACCUMULATE) !=0)) || (datatype->sizeof_substruct != 0)){
    // This part handles the problem of non-contiguous memory
    old_buf = buf;
    buf = count==0 ? nullptr : xbt_malloc(count*smpi_datatype_size(datatype));
    if ((datatype->sizeof_substruct != 0) && ((flags & SEND) != 0)) {
      subtype->serialize(old_buf, buf, count, datatype->substruct);
    }
  }

  request->buf      = buf;
  // This part handles the problem of non-contiguous memory (for the unserialisation at the reception)
  request->old_buf  = old_buf;
  request->old_type = datatype;

  request->size = smpi_datatype_size(datatype) * count;
  smpi_datatype_use(datatype);
  request->src  = src;
  request->dst  = dst;
  request->tag  = tag;
  request->comm = comm;
  request->comm->use();
  request->action          = nullptr;
  request->flags           = flags;
  request->detached        = 0;
  request->detached_sender = nullptr;
  request->real_src        = 0;
  request->truncated       = 0;
  request->real_size       = 0;
  request->real_tag        = 0;
  if (flags & PERSISTENT)
    request->refcount = 1;
  else
    request->refcount = 0;
  request->op   = MPI_REPLACE;
  request->send = 0;
  request->recv = 0;

  return request;
}

void smpi_empty_status(MPI_Status * status)
{
  if(status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = MPI_ANY_SOURCE;
    status->MPI_TAG = MPI_ANY_TAG;
    status->MPI_ERROR = MPI_SUCCESS;
    status->count=0;
  }
}

static void smpi_mpi_request_free_voidp(void* request)
{
  MPI_Request req = static_cast<MPI_Request>(request);
  smpi_mpi_request_free(&req);
}

/* MPI Low level calls */
MPI_Request smpi_mpi_send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process_index(),
                          comm->group()->index(dst), tag, comm, PERSISTENT | SEND | PREPARED);
  return request;
}

MPI_Request smpi_mpi_ssend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process_index(),
                        comm->group()->index(dst), tag, comm, PERSISTENT | SSEND | SEND | PREPARED);
  return request;
}

MPI_Request smpi_mpi_recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,
                          src == MPI_ANY_SOURCE ? MPI_ANY_SOURCE : comm->group()->index(src),
                          smpi_process_index(), tag, comm, PERSISTENT | RECV | PREPARED);
  return request;
}

void smpi_mpi_start(MPI_Request request)
{
  smx_mailbox_t mailbox;

  xbt_assert(request->action == nullptr, "Cannot (re-)start unfinished communication");
  request->flags &= ~PREPARED;
  request->flags &= ~FINISHED;
  request->refcount++;

  if ((request->flags & RECV) != 0) {
    print_request("New recv", request);

    int async_small_thresh = xbt_cfg_get_int("smpi/async-small-thresh");

    xbt_mutex_t mut = smpi_process_mailboxes_mutex();
    if (async_small_thresh != 0 || (request->flags & RMA) != 0)
      xbt_mutex_acquire(mut);

    if (async_small_thresh == 0 && (request->flags & RMA) == 0 ) {
      mailbox = smpi_process_mailbox();
    } 
    else if (((request->flags & RMA) != 0) || static_cast<int>(request->size) < async_small_thresh) {
      //We have to check both mailboxes (because SSEND messages are sent to the large mbox).
      //begin with the more appropriate one : the small one.
      mailbox = smpi_process_mailbox_small();
      XBT_DEBUG("Is there a corresponding send already posted in the small mailbox %p (in case of SSEND)?", mailbox);
      smx_activity_t action = simcall_comm_iprobe(mailbox, 0, request->src,request->tag, &match_recv,
                                                  static_cast<void*>(request));

      if (action == nullptr) {
        mailbox = smpi_process_mailbox();
        XBT_DEBUG("No, nothing in the small mailbox test the other one : %p", mailbox);
        action = simcall_comm_iprobe(mailbox, 0, request->src,request->tag, &match_recv, static_cast<void*>(request));
        if (action == nullptr) {
          XBT_DEBUG("Still nothing, switch back to the small mailbox : %p", mailbox);
          mailbox = smpi_process_mailbox_small();
        }
      } else {
        XBT_DEBUG("yes there was something for us in the large mailbox");
      }
    } else {
      mailbox = smpi_process_mailbox_small();
      XBT_DEBUG("Is there a corresponding send already posted the small mailbox?");
      smx_activity_t action = simcall_comm_iprobe(mailbox, 0, request->src,request->tag, &match_recv, (void*)request);

      if (action == nullptr) {
        XBT_DEBUG("No, nothing in the permanent receive mailbox");
        mailbox = smpi_process_mailbox();
      } else {
        XBT_DEBUG("yes there was something for us in the small mailbox");
      }
    }

    // we make a copy here, as the size is modified by simix, and we may reuse the request in another receive later
    request->real_size=request->size;
    request->action = simcall_comm_irecv(SIMIX_process_self(), mailbox, request->buf, &request->real_size, &match_recv,
                                         ! smpi_process_get_replaying()? smpi_comm_copy_data_callback
                                         : &smpi_comm_null_copy_buffer_callback, request, -1.0);
    XBT_DEBUG("recv simcall posted");

    if (async_small_thresh != 0 || (request->flags & RMA) != 0 )
      xbt_mutex_release(mut);
  } else { /* the RECV flag was not set, so this is a send */
    int receiver = request->dst;

    int rank = request->src;
    if (TRACE_smpi_view_internals()) {
      TRACE_smpi_send(rank, rank, receiver, request->tag, request->size);
    }
    print_request("New send", request);

    void* buf = request->buf;
    if ((request->flags & SSEND) == 0 && ( (request->flags & RMA) != 0
        || static_cast<int>(request->size) < xbt_cfg_get_int("smpi/send-is-detached-thresh") ) ) {
      void *oldbuf = nullptr;
      request->detached = 1;
      XBT_DEBUG("Send request %p is detached", request);
      request->refcount++;
      if(request->old_type->sizeof_substruct == 0){
        oldbuf = request->buf;
        if (!smpi_process_get_replaying() && oldbuf != nullptr && request->size!=0){
          if((smpi_privatize_global_variables != 0)
            && (static_cast<char*>(request->buf) >= smpi_start_data_exe)
            && (static_cast<char*>(request->buf) < smpi_start_data_exe + smpi_size_data_exe )){
            XBT_DEBUG("Privatization : We are sending from a zone inside global memory. Switch data segment ");
            smpi_switch_data_segment(request->src);
          }
          buf = xbt_malloc(request->size);
          memcpy(buf,oldbuf,request->size);
          XBT_DEBUG("buf %p copied into %p",oldbuf,buf);
        }
      }
    }

    //if we are giving back the control to the user without waiting for completion, we have to inject timings
    double sleeptime = 0.0;
    if(request->detached != 0 || ((request->flags & (ISEND|SSEND)) != 0)){// issend should be treated as isend
      //isend and send timings may be different
      sleeptime = ((request->flags & ISEND) != 0) ? smpi_ois(request->size) : smpi_os(request->size);
    }

    if(sleeptime > 0.0){
      simcall_process_sleep(sleeptime);
      XBT_DEBUG("sending size of %zu : sleep %f ", request->size, sleeptime);
    }

    int async_small_thresh = xbt_cfg_get_int("smpi/async-small-thresh");

    xbt_mutex_t mut=smpi_process_remote_mailboxes_mutex(receiver);

    if (async_small_thresh != 0 || (request->flags & RMA) != 0)
      xbt_mutex_acquire(mut);

    if (!(async_small_thresh != 0 || (request->flags & RMA) !=0)) {
      mailbox = smpi_process_remote_mailbox(receiver);
    } else if (((request->flags & RMA) != 0) || static_cast<int>(request->size) < async_small_thresh) { // eager mode
      mailbox = smpi_process_remote_mailbox(receiver);
      XBT_DEBUG("Is there a corresponding recv already posted in the large mailbox %p?", mailbox);
      smx_activity_t action = simcall_comm_iprobe(mailbox, 1,request->dst, request->tag, &match_send,
                                                  static_cast<void*>(request));
      if (action == nullptr) {
        if ((request->flags & SSEND) == 0){
          mailbox = smpi_process_remote_mailbox_small(receiver);
          XBT_DEBUG("No, nothing in the large mailbox, message is to be sent on the small one %p", mailbox);
        } else {
          mailbox = smpi_process_remote_mailbox_small(receiver);
          XBT_DEBUG("SSEND : Is there a corresponding recv already posted in the small mailbox %p?", mailbox);
          action = simcall_comm_iprobe(mailbox, 1,request->dst, request->tag, &match_send, static_cast<void*>(request));
          if (action == nullptr) {
            XBT_DEBUG("No, we are first, send to large mailbox");
            mailbox = smpi_process_remote_mailbox(receiver);
          }
        }
      } else {
        XBT_DEBUG("Yes there was something for us in the large mailbox");
      }
    } else {
      mailbox = smpi_process_remote_mailbox(receiver);
      XBT_DEBUG("Send request %p is in the large mailbox %p (buf: %p)",mailbox, request,request->buf);
    }

    // we make a copy here, as the size is modified by simix, and we may reuse the request in another receive later
    request->real_size=request->size;
    request->action = simcall_comm_isend(SIMIX_process_from_PID(request->src+1), mailbox, request->size, -1.0,
                                         buf, request->real_size, &match_send,
                         &xbt_free_f, // how to free the userdata if a detached send fails
                         !smpi_process_get_replaying() ? smpi_comm_copy_data_callback
                         : &smpi_comm_null_copy_buffer_callback, request,
                         // detach if msg size < eager/rdv switch limit
                         request->detached);
    XBT_DEBUG("send simcall posted");

    /* FIXME: detached sends are not traceable (request->action == nullptr) */
    if (request->action != nullptr)
      simcall_set_category(request->action, TRACE_internal_smpi_get_category());

    if (async_small_thresh != 0 || ((request->flags & RMA)!=0))
      xbt_mutex_release(mut);
  }
}

void smpi_mpi_startall(int count, MPI_Request * requests)
{
  if(requests== nullptr) 
    return;

  for(int i = 0; i < count; i++) {
    smpi_mpi_start(requests[i]);
  }
}

void smpi_mpi_request_free(MPI_Request * request)
{
  if((*request) != MPI_REQUEST_NULL){
    (*request)->refcount--;
    if((*request)->refcount<0) xbt_die("wrong refcount");

    if((*request)->refcount==0){
        smpi_datatype_unuse((*request)->old_type);
        (*request)->comm->unuse();
        print_request("Destroying", (*request));
        xbt_free(*request);
        *request = MPI_REQUEST_NULL;
    }else{
      print_request("Decrementing", (*request));
    }
  }else{
    xbt_die("freeing an already free request");
  }
}

MPI_Request smpi_rma_send_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,
                               MPI_Op op)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  if(op==MPI_OP_NULL){
    request = build_request(buf==MPI_BOTTOM ? nullptr : buf , count, datatype, src, dst, tag,
                            comm, RMA | NON_PERSISTENT | ISEND | SEND | PREPARED);
  }else{
    request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,  src, dst, tag,
                            comm, RMA | NON_PERSISTENT | ISEND | SEND | PREPARED | ACCUMULATE);
    request->op = op;
  }
  return request;
}

MPI_Request smpi_rma_recv_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,
                               MPI_Op op)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  if(op==MPI_OP_NULL){
    request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,  src, dst, tag,
                            comm, RMA | NON_PERSISTENT | RECV | PREPARED);
  }else{
    request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,  src, dst, tag,
                            comm, RMA | NON_PERSISTENT | RECV | PREPARED | ACCUMULATE);
    request->op = op;
  }
  return request;
}

MPI_Request smpi_isend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf , count, datatype, smpi_process_index(),
                          comm->group()->index(dst), tag,comm, PERSISTENT | ISEND | SEND | PREPARED);
  return request;
}

MPI_Request smpi_mpi_isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request =  build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process_index(),
                           comm->group()->index(dst), tag, comm, NON_PERSISTENT | ISEND | SEND);
  smpi_mpi_start(request);
  return request;
}

MPI_Request smpi_mpi_issend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process_index(),
                        comm->group()->index(dst), tag,comm, NON_PERSISTENT | ISEND | SSEND | SEND);
  smpi_mpi_start(request);
  return request;
}

MPI_Request smpi_irecv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, src == MPI_ANY_SOURCE ? MPI_ANY_SOURCE :
                          comm->group()->index(src), smpi_process_index(), tag,
                          comm, PERSISTENT | RECV | PREPARED);
  return request;
}

MPI_Request smpi_mpi_irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, src == MPI_ANY_SOURCE ? MPI_ANY_SOURCE :
                          comm->group()->index(src), smpi_process_index(), tag, comm,
                          NON_PERSISTENT | RECV);
  smpi_mpi_start(request);
  return request;
}

void smpi_mpi_recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
  smpi_mpi_wait(&request, status);
  request = nullptr;
}

void smpi_mpi_send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process_index(),
                          comm->group()->index(dst), tag, comm, NON_PERSISTENT | SEND);

  smpi_mpi_start(request);
  smpi_mpi_wait(&request, MPI_STATUS_IGNORE);
  request = nullptr;
}

void smpi_mpi_ssend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = build_request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process_index(),
                          comm->group()->index(dst), tag, comm, NON_PERSISTENT | SSEND | SEND);

  smpi_mpi_start(request);
  smpi_mpi_wait(&request, MPI_STATUS_IGNORE);
  request = nullptr;
}

void smpi_mpi_sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,int dst, int sendtag,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag,
                       MPI_Comm comm, MPI_Status * status)
{
  MPI_Request requests[2];
  MPI_Status stats[2];
  int myid=smpi_process_index();
  if ((comm->group()->index(dst) == myid) && (comm->group()->index(src) == myid)){
      smpi_datatype_copy(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype);
      return;
  }
  requests[0] = smpi_isend_init(sendbuf, sendcount, sendtype, dst, sendtag, comm);
  requests[1] = smpi_irecv_init(recvbuf, recvcount, recvtype, src, recvtag, comm);
  smpi_mpi_startall(2, requests);
  smpi_mpi_waitall(2, requests, stats);
  smpi_mpi_request_free(&requests[0]);
  smpi_mpi_request_free(&requests[1]);
  if(status != MPI_STATUS_IGNORE) {
    // Copy receive status
    *status = stats[1];
  }
}

int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype)
{
  return status->count / smpi_datatype_size(datatype);
}

static void finish_wait(MPI_Request * request, MPI_Status * status)
{
  MPI_Request req = *request;
  smpi_empty_status(status);

  if(!((req->detached != 0) && ((req->flags & SEND) != 0)) && ((req->flags & PREPARED) == 0)){
    if(status != MPI_STATUS_IGNORE) {
      int src = req->src == MPI_ANY_SOURCE ? req->real_src : req->src;
      status->MPI_SOURCE = req->comm->group()->rank(src);
      status->MPI_TAG = req->tag == MPI_ANY_TAG ? req->real_tag : req->tag;
      status->MPI_ERROR = req->truncated != 0 ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
      // this handles the case were size in receive differs from size in send
      status->count = req->real_size;
    }

    print_request("Finishing", req);
    MPI_Datatype datatype = req->old_type;

    if(((req->flags & ACCUMULATE) != 0) || (datatype->sizeof_substruct != 0)){
      if (!smpi_process_get_replaying()){
        if( smpi_privatize_global_variables != 0 && (static_cast<char*>(req->old_buf) >= smpi_start_data_exe)
            && ((char*)req->old_buf < smpi_start_data_exe + smpi_size_data_exe )){
            XBT_VERB("Privatization : We are unserializing to a zone in global memory - Switch data segment ");
            smpi_switch_data_segment(smpi_process_index());
        }
      }

      if(datatype->sizeof_substruct != 0){
        // This part handles the problem of non-contignous memory the unserialization at the reception
        s_smpi_subtype_t *subtype = static_cast<s_smpi_subtype_t*>(datatype->substruct);
        if(req->flags & RECV)
          subtype->unserialize(req->buf, req->old_buf, req->real_size/smpi_datatype_size(datatype) ,
                               datatype->substruct, req->op);
        xbt_free(req->buf);
      }else if(req->flags & RECV){//apply op on contiguous buffer for accumulate
          int n =req->real_size/smpi_datatype_size(datatype);
          smpi_op_apply(req->op, req->buf, req->old_buf, &n, &datatype);
          xbt_free(req->buf);
      }
    }
  }

  if (TRACE_smpi_view_internals() && ((req->flags & RECV) != 0)){
    int rank = smpi_process_index();
    int src_traced = (req->src == MPI_ANY_SOURCE ? req->real_src : req->src);
    TRACE_smpi_recv(rank, src_traced, rank,req->tag);
  }

  if(req->detached_sender != nullptr){
    //integrate pseudo-timing for buffering of small messages, do not bother to execute the simcall if 0
    double sleeptime = smpi_or(req->real_size);
    if(sleeptime > 0.0){
      simcall_process_sleep(sleeptime);
      XBT_DEBUG("receiving size of %zu : sleep %f ", req->real_size, sleeptime);
    }
    smpi_mpi_request_free(&(req->detached_sender));
  }
  if(req->flags & PERSISTENT)
    req->action = nullptr;
  req->flags |= FINISHED;

  smpi_mpi_request_free(request);
}

int smpi_mpi_test(MPI_Request * request, MPI_Status * status) {
  //assume that request is not MPI_REQUEST_NULL (filtered in PMPI_Test or smpi_mpi_testall before)

  // to avoid deadlocks if used as a break condition, such as
  //     while (MPI_Test(request, flag, status) && flag) {
  //     }
  // because the time will not normally advance when only calls to MPI_Test are made -> deadlock
  // multiplier to the sleeptime, to increase speed of execution, each failed test will increase it
  static int nsleeps = 1;
  if(smpi_test_sleep > 0)  
    simcall_process_sleep(nsleeps*smpi_test_sleep);

  smpi_empty_status(status);
  int flag = 1;
  if (((*request)->flags & PREPARED) == 0) {
    if ((*request)->action != nullptr)
      flag = simcall_comm_test((*request)->action);
    if (flag) {
      finish_wait(request, status);
      nsleeps=1;//reset the number of sleeps we will do next time
      if (*request != MPI_REQUEST_NULL && ((*request)->flags & PERSISTENT)==0)
      *request = MPI_REQUEST_NULL;
    } else if (xbt_cfg_get_boolean("smpi/grow-injected-times")){
      nsleeps++;
    }
  }
  return flag;
}

int smpi_mpi_testany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  std::vector<simgrid::kernel::activity::ActivityImpl*> comms;
  comms.reserve(count);

  int i;
  int flag = 0;

  *index = MPI_UNDEFINED;

  std::vector<int> map; /** Maps all matching comms back to their location in requests **/
  for(i = 0; i < count; i++) {
    if ((requests[i] != MPI_REQUEST_NULL) && requests[i]->action && !(requests[i]->flags & PREPARED)) {
       comms.push_back(requests[i]->action);
       map.push_back(i);
    }
  }
  if(!map.empty()) {
    //multiplier to the sleeptime, to increase speed of execution, each failed testany will increase it
    static int nsleeps = 1;
    if(smpi_test_sleep > 0) 
      simcall_process_sleep(nsleeps*smpi_test_sleep);

    i = simcall_comm_testany(comms.data(), comms.size()); // The i-th element in comms matches!
    if (i != -1) { // -1 is not MPI_UNDEFINED but a SIMIX return code. (nothing matches)
      *index = map[i]; 
      finish_wait(&requests[*index], status);
      flag             = 1;
      nsleeps          = 1;
      if (requests[*index] != MPI_REQUEST_NULL && (requests[*index]->flags & NON_PERSISTENT)) {
        requests[*index] = MPI_REQUEST_NULL;
      }
    } else {
      nsleeps++;
    }
  } else {
      //all requests are null or inactive, return true
      flag = 1;
      smpi_empty_status(status);
  }

  return flag;
}

int smpi_mpi_testall(int count, MPI_Request requests[], MPI_Status status[])
{
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;
  int flag=1;
  for(int i=0; i<count; i++){
    if (requests[i] != MPI_REQUEST_NULL && !(requests[i]->flags & PREPARED)) {
      if (smpi_mpi_test(&requests[i], pstat)!=1){
        flag=0;
      }else{
          requests[i]=MPI_REQUEST_NULL;
      }
    }else{
      smpi_empty_status(pstat);
    }
    if(status != MPI_STATUSES_IGNORE) {
      status[i] = *pstat;
    }
  }
  return flag;
}

void smpi_mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status* status){
  int flag=0;
  //FIXME find another way to avoid busy waiting ?
  // the issue here is that we have to wait on a nonexistent comm
  while(flag==0){
    smpi_mpi_iprobe(source, tag, comm, &flag, status);
    XBT_DEBUG("Busy Waiting on probing : %d", flag);
  }
}

void smpi_mpi_iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status){
  MPI_Request request = build_request(nullptr, 0, MPI_CHAR, source == MPI_ANY_SOURCE ? MPI_ANY_SOURCE :
                 comm->group()->index(source), comm->rank(), tag, comm, PERSISTENT | RECV);

  // to avoid deadlock, we have to sleep some time here, or the timer won't advance and we will only do iprobe simcalls
  // (especially when used as a break condition, such as while(MPI_Iprobe(...)) ... )
  // multiplier to the sleeptime, to increase speed of execution, each failed iprobe will increase it
  static int nsleeps = 1;
  double speed       = simgrid::s4u::Actor::self()->host()->speed();
  double maxrate = xbt_cfg_get_double("smpi/iprobe-cpu-usage");
  if (smpi_iprobe_sleep > 0) {
    smx_activity_t iprobe_sleep = simcall_execution_start("iprobe", /* flops to executek*/nsleeps*smpi_iprobe_sleep*speed*maxrate, /* priority */1.0, /* performance bound */maxrate*speed);
    simcall_execution_wait(iprobe_sleep);
  }
  // behave like a receive, but don't do it
  smx_mailbox_t mailbox;

  print_request("New iprobe", request);
  // We have to test both mailboxes as we don't know if we will receive one one or another
  if (xbt_cfg_get_int("smpi/async-small-thresh") > 0){
      mailbox = smpi_process_mailbox_small();
      XBT_DEBUG("Trying to probe the perm recv mailbox");
      request->action = simcall_comm_iprobe(mailbox, 0, request->src, request->tag, &match_recv,
                                            static_cast<void*>(request));
  }

  if (request->action == nullptr){
    mailbox = smpi_process_mailbox();
    XBT_DEBUG("trying to probe the other mailbox");
    request->action = simcall_comm_iprobe(mailbox, 0, request->src,request->tag, &match_recv,
                                          static_cast<void*>(request));
  }

  if (request->action != nullptr){
    simgrid::kernel::activity::Comm *sync_comm = static_cast<simgrid::kernel::activity::Comm*>(request->action);
    MPI_Request req                            = static_cast<MPI_Request>(sync_comm->src_data);
    *flag = 1;
    if(status != MPI_STATUS_IGNORE && (req->flags & PREPARED) == 0) {
      status->MPI_SOURCE = comm->group()->rank(req->src);
      status->MPI_TAG    = req->tag;
      status->MPI_ERROR  = MPI_SUCCESS;
      status->count      = req->real_size;
    }
    nsleeps = 1;//reset the number of sleeps we will do next time
  }
  else {
    *flag = 0;
    if (xbt_cfg_get_boolean("smpi/grow-injected-times"))
      nsleeps++;
  }
  smpi_mpi_request_free(&request);
}

void smpi_mpi_wait(MPI_Request * request, MPI_Status * status)
{
  print_request("Waiting", *request);
  if ((*request)->flags & PREPARED) {
    smpi_empty_status(status);
    return;
  }

  if ((*request)->action != nullptr)
    // this is not a detached send
    simcall_comm_wait((*request)->action, -1.0);

  finish_wait(request, status);
  if (*request != MPI_REQUEST_NULL && (((*request)->flags & NON_PERSISTENT)!=0))
    *request = MPI_REQUEST_NULL;
}

static int sort_accumulates(MPI_Request a, MPI_Request b)
{
  return (a->tag < b->tag);
}

int smpi_mpi_waitany(int count, MPI_Request requests[], MPI_Status * status)
{
  s_xbt_dynar_t comms; // Keep it on stack to save some extra mallocs
  int i;
  int size = 0;
  int index = MPI_UNDEFINED;
  int *map;

  if(count > 0) {
    // Wait for a request to complete
    xbt_dynar_init(&comms, sizeof(smx_activity_t), nullptr);
    map = xbt_new(int, count);
    XBT_DEBUG("Wait for one of %d", count);
    for(i = 0; i < count; i++) {
      if (requests[i] != MPI_REQUEST_NULL && !(requests[i]->flags & PREPARED) && !(requests[i]->flags & FINISHED)) {
        if (requests[i]->action != nullptr) {
          XBT_DEBUG("Waiting any %p ", requests[i]);
          xbt_dynar_push(&comms, &requests[i]->action);
          map[size] = i;
          size++;
        } else {
          // This is a finished detached request, let's return this one
          size  = 0; // so we free the dynar but don't do the waitany call
          index = i;
          finish_wait(&requests[i], status); // cleanup if refcount = 0
          if (requests[i] != MPI_REQUEST_NULL && (requests[i]->flags & NON_PERSISTENT))
            requests[i] = MPI_REQUEST_NULL; // set to null
          break;
        }
      }
    }
    if(size > 0) {
      i = simcall_comm_waitany(&comms, -1);

      // not MPI_UNDEFINED, as this is a simix return code
      if (i != -1) {
        index = map[i];
        //in case of an accumulate, we have to wait the end of all requests to apply the operation, ordered correctly.
        if ((requests[index] == MPI_REQUEST_NULL)
             ||  (!((requests[index]->flags & ACCUMULATE) && (requests[index]->flags & RECV)))){
          finish_wait(&requests[index], status);
          if (requests[i] != MPI_REQUEST_NULL && (requests[i]->flags & NON_PERSISTENT))
            requests[index] = MPI_REQUEST_NULL;
        }else{
            XBT_WARN("huu?");
        }
      }
    }

    xbt_dynar_free_data(&comms);
    xbt_free(map);
  }

  if (index==MPI_UNDEFINED)
    smpi_empty_status(status);

  return index;
}

int smpi_mpi_waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  std::vector<MPI_Request> accumulates;
  int index;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;
  int retvalue = MPI_SUCCESS;
  //tag invalid requests in the set
  if (status != MPI_STATUSES_IGNORE) {
    for (int c = 0; c < count; c++) {
      if (requests[c] == MPI_REQUEST_NULL || requests[c]->dst == MPI_PROC_NULL || (requests[c]->flags & PREPARED)) {
        smpi_empty_status(&status[c]);
      } else if (requests[c]->src == MPI_PROC_NULL) {
        smpi_empty_status(&status[c]);
        status[c].MPI_SOURCE = MPI_PROC_NULL;
      }
    }
  }
  for (int c = 0; c < count; c++) {
    if (MC_is_active() || MC_record_replay_is_active()) {
      smpi_mpi_wait(&requests[c], pstat);
      index = c;
    } else {
      index = smpi_mpi_waitany(count, requests, pstat);
      if (index == MPI_UNDEFINED)
        break;

      if (requests[index] != MPI_REQUEST_NULL
           && (requests[index]->flags & RECV)
           && (requests[index]->flags & ACCUMULATE))
        accumulates.push_back(requests[index]);
      if (requests[index] != MPI_REQUEST_NULL && (requests[index]->flags & NON_PERSISTENT))
        requests[index] = MPI_REQUEST_NULL;
    }
    if (status != MPI_STATUSES_IGNORE) {
      status[index] = *pstat;
      if (status[index].MPI_ERROR == MPI_ERR_TRUNCATE)
        retvalue = MPI_ERR_IN_STATUS;
    }
  }

  if (!accumulates.empty()) {
    std::sort(accumulates.begin(), accumulates.end(), sort_accumulates);
    for (auto req : accumulates) {
      finish_wait(&req, status);
    }
  }

  return retvalue;
}

int smpi_mpi_waitsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[])
{
  int i;
  int count = 0;
  int index;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;

  for(i = 0; i < incount; i++)
  {
    index=smpi_mpi_waitany(incount, requests, pstat);
    if(index!=MPI_UNDEFINED){
      indices[count] = index;
      count++;
      if(status != MPI_STATUSES_IGNORE) {
        status[index] = *pstat;
      }
     if (requests[index] != MPI_REQUEST_NULL && (requests[index]->flags & NON_PERSISTENT))
     requests[index]=MPI_REQUEST_NULL;
    }else{
      return MPI_UNDEFINED;
    }
  }
  return count;
}

int smpi_mpi_testsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[])
{
  int i;
  int count = 0;
  int count_dead = 0;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;

  for(i = 0; i < incount; i++) {
    if((requests[i] != MPI_REQUEST_NULL)) {
      if(smpi_mpi_test(&requests[i], pstat)) {
         indices[i] = 1;
         count++;
         if(status != MPI_STATUSES_IGNORE) {
           status[i] = *pstat;
         }
         if ((requests[i] != MPI_REQUEST_NULL) && requests[i]->flags & NON_PERSISTENT)
         requests[i]=MPI_REQUEST_NULL;
      }
    }else{
      count_dead++;
    }
  }
  if(count_dead==incount)
    return MPI_UNDEFINED;
  else return count;
}

void smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  smpi_coll_tuned_bcast_binomial_tree(buf, count, datatype, root, comm);
}

void smpi_mpi_barrier(MPI_Comm comm)
{
  smpi_coll_tuned_barrier_ompi_basic_linear(comm);
}

void smpi_mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_GATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    smpi_datatype_extent(recvtype, &lb, &recvext);
    // Local copy from root
    smpi_datatype_copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + root * recvcount * recvext,
                       recvcount, recvtype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = smpi_irecv_init(static_cast<char*>(recvbuf) + src * recvcount * recvext, recvcount, recvtype,
                                          src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int src = 0; src < size-1; src++) {
      smpi_mpi_request_free(&requests[src]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm)
{
  int rank = comm->rank();

  /* arbitrarily choose root as rank 0 */
  int size = comm->size();
  int count = 0;
  int *displs = xbt_new(int, size);
  for (int i = 0; i < size; i++) {
    displs[i] = count;
    count += recvcounts[i];
  }
  void *tmpbuf = static_cast<void*>(smpi_get_tmp_sendbuffer(count*smpi_datatype_get_extent(datatype)));

  mpi_coll_reduce_fun(sendbuf, tmpbuf, count, datatype, op, 0, comm);
  smpi_mpi_scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
  xbt_free(displs);
  smpi_free_tmp_buffer(tmpbuf);
}

void smpi_mpi_gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_GATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if (rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    smpi_datatype_extent(recvtype, &lb, &recvext);
    // Local copy from root
    smpi_datatype_copy(sendbuf, sendcount, sendtype, static_cast<char*>(recvbuf) + displs[root] * recvext,
                       recvcounts[root], recvtype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = smpi_irecv_init(static_cast<char*>(recvbuf) + displs[src] * recvext,
                          recvcounts[src], recvtype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int src = 0; src < size-1; src++) {
      smpi_mpi_request_free(&requests[src]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf,int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLGATHER;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  // FIXME: check for errors
  smpi_datatype_extent(recvtype, &lb, &recvext);
  // Local copy from self
  smpi_datatype_copy(sendbuf, sendcount, sendtype, static_cast<char *>(recvbuf) + rank * recvcount * recvext, recvcount,
                     recvtype);
  // Send/Recv buffers to/from others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  int index = 0;
  for (int other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] = smpi_isend_init(sendbuf, sendcount, sendtype, other, system_tag,comm);
      index++;
      requests[index] = smpi_irecv_init(static_cast<char *>(recvbuf) + other * recvcount * recvext, recvcount, recvtype,
                                        other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(2 * (size - 1), requests);
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  for (int other = 0; other < 2*(size-1); other++) {
    smpi_mpi_request_free(&requests[other]);
  }
  xbt_free(requests);
}

void smpi_mpi_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = COLL_TAG_ALLGATHERV;
  MPI_Aint lb = 0;
  MPI_Aint recvext = 0;

  int rank = comm->rank();
  int size = comm->size();
  smpi_datatype_extent(recvtype, &lb, &recvext);
  // Local copy from self
  smpi_datatype_copy(sendbuf, sendcount, sendtype,
                     static_cast<char *>(recvbuf) + displs[rank] * recvext,recvcounts[rank], recvtype);
  // Send buffers to others;
  MPI_Request *requests = xbt_new(MPI_Request, 2 * (size - 1));
  int index = 0;
  for (int other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] =
        smpi_isend_init(sendbuf, sendcount, sendtype, other, system_tag, comm);
      index++;
      requests[index] = smpi_irecv_init(static_cast<char *>(recvbuf) + displs[other] * recvext, recvcounts[other],
                          recvtype, other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(2 * (size - 1), requests);
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  for (int other = 0; other < 2*(size-1); other++) {
    smpi_mpi_request_free(&requests[other]);
  }
  xbt_free(requests);
}

void smpi_mpi_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_SCATTER;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;
  MPI_Request *requests;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    smpi_datatype_extent(sendtype, &lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
        smpi_datatype_copy(static_cast<char *>(sendbuf) + root * sendcount * sendext,
                           sendcount, sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for(int dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] = smpi_isend_init(static_cast<char *>(sendbuf) + dst * sendcount * sendext, sendcount, sendtype,
                                          dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int dst = 0; dst < size-1; dst++) {
      smpi_mpi_request_free(&requests[dst]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = COLL_TAG_SCATTERV;
  MPI_Aint lb = 0;
  MPI_Aint sendext = 0;

  int rank = comm->rank();
  int size = comm->size();
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    smpi_datatype_extent(sendtype, &lb, &sendext);
    // Local copy from root
    if(recvbuf!=MPI_IN_PLACE){
      smpi_datatype_copy(static_cast<char *>(sendbuf) + displs[root] * sendext, sendcounts[root],
                       sendtype, recvbuf, recvcount, recvtype);
    }
    // Send buffers to receivers
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    int index = 0;
    for (int dst = 0; dst < size; dst++) {
      if (dst != root) {
        requests[index] = smpi_isend_init(static_cast<char *>(sendbuf) + displs[dst] * sendext, sendcounts[dst],
                            sendtype, dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    for (int dst = 0; dst < size-1; dst++) {
      smpi_mpi_request_free(&requests[dst]);
    }
    xbt_free(requests);
  }
}

void smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  int system_tag = COLL_TAG_REDUCE;
  MPI_Aint lb = 0;
  MPI_Aint dataext = 0;

  char* sendtmpbuf = static_cast<char *>(sendbuf);

  int rank = comm->rank();
  int size = comm->size();
  //non commutative case, use a working algo from openmpi
  if(!smpi_op_is_commute(op)){
    smpi_coll_tuned_reduce_ompi_basic_linear(sendtmpbuf, recvbuf, count, datatype, op, root, comm);
    return;
  }

  if( sendbuf == MPI_IN_PLACE ) {
    sendtmpbuf = static_cast<char *>(smpi_get_tmp_sendbuffer(count*smpi_datatype_get_extent(datatype)));
    smpi_datatype_copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
  }
  
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendtmpbuf, count, datatype, root, system_tag, comm);
  } else {
    smpi_datatype_extent(datatype, &lb, &dataext);
    // Local copy from root
    if (sendtmpbuf != nullptr && recvbuf != nullptr)
      smpi_datatype_copy(sendtmpbuf, count, datatype, recvbuf, count, datatype);
    // Receive buffers from senders
    MPI_Request *requests = xbt_new(MPI_Request, size - 1);
    void **tmpbufs = xbt_new(void *, size - 1);
    int index = 0;
    for (int src = 0; src < size; src++) {
      if (src != root) {
         if (!smpi_process_get_replaying())
          tmpbufs[index] = xbt_malloc(count * dataext);
         else
           tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
        requests[index] =
          smpi_irecv_init(tmpbufs[index], count, datatype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    for (int src = 0; src < size - 1; src++) {
      index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
      XBT_DEBUG("finished waiting any request with index %d", index);
      if(index == MPI_UNDEFINED) {
        break;
      }else{
        smpi_mpi_request_free(&requests[index]);
      }
      if(op) /* op can be MPI_OP_NULL that does nothing */
        smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
    }
      for(index = 0; index < size - 1; index++) {
        smpi_free_tmp_buffer(tmpbufs[index]);
      }
    xbt_free(tmpbufs);
    xbt_free(requests);

  }
  if( sendbuf == MPI_IN_PLACE ) {
    smpi_free_tmp_buffer(sendtmpbuf);
  }
}

void smpi_mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  smpi_mpi_bcast(recvbuf, count, datatype, 0, comm);
}

void smpi_mpi_scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb      = 0;
  MPI_Aint dataext = 0;

  int rank = comm->rank();
  int size = comm->size();

  smpi_datatype_extent(datatype, &lb, &dataext);

  // Local copy from self
  smpi_datatype_copy(sendbuf, count, datatype, recvbuf, count, datatype);

  // Send/Recv buffers to/from others;
  MPI_Request *requests = xbt_new(MPI_Request, size - 1);
  void **tmpbufs = xbt_new(void *, rank);
  int index = 0;
  for (int other = 0; other < rank; other++) {
    tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
    requests[index] = smpi_irecv_init(tmpbufs[index], count, datatype, other, system_tag, comm);
    index++;
  }
  for (int other = rank + 1; other < size; other++) {
    requests[index] = smpi_isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(size - 1, requests);

  if(smpi_op_is_commute(op)){
    for (int other = 0; other < size - 1; other++) {
      index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(index < rank) {
        // #Request is below rank: it's a irecv
        smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
      smpi_mpi_wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank) {
        smpi_op_apply(op, tmpbufs[other], recvbuf, &count, &datatype);
      }
    }
  }
  for(index = 0; index < rank; index++) {
    smpi_free_tmp_buffer(tmpbufs[index]);
  }
  for(index = 0; index < size-1; index++) {
    smpi_mpi_request_free(&requests[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
}

void smpi_mpi_exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = -888;
  MPI_Aint lb         = 0;
  MPI_Aint dataext    = 0;
  int recvbuf_is_empty=1;
  int rank = comm->rank();
  int size = comm->size();

  smpi_datatype_extent(datatype, &lb, &dataext);

  // Send/Recv buffers to/from others;
  MPI_Request *requests = xbt_new(MPI_Request, size - 1);
  void **tmpbufs = xbt_new(void *, rank);
  int index = 0;
  for (int other = 0; other < rank; other++) {
    tmpbufs[index] = smpi_get_tmp_sendbuffer(count * dataext);
    requests[index] = smpi_irecv_init(tmpbufs[index], count, datatype, other, system_tag, comm);
    index++;
  }
  for (int other = rank + 1; other < size; other++) {
    requests[index] = smpi_isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(size - 1, requests);

  if(smpi_op_is_commute(op)){
    for (int other = 0; other < size - 1; other++) {
      index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(index < rank) {
        if(recvbuf_is_empty){
          smpi_datatype_copy(tmpbufs[index], count, datatype, recvbuf, count, datatype);
          recvbuf_is_empty=0;
        } else
          // #Request is below rank: it's a irecv
          smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
      }
    }
  }else{
    //non commutative case, wait in order
    for (int other = 0; other < size - 1; other++) {
      smpi_mpi_wait(&(requests[other]), MPI_STATUS_IGNORE);
      if(index < rank) {
        if (recvbuf_is_empty) {
          smpi_datatype_copy(tmpbufs[other], count, datatype, recvbuf, count, datatype);
          recvbuf_is_empty = 0;
        } else
          smpi_op_apply(op, tmpbufs[other], recvbuf, &count, &datatype);
      }
    }
  }
  for(index = 0; index < rank; index++) {
    smpi_free_tmp_buffer(tmpbufs[index]);
  }
  for(index = 0; index < size-1; index++) {
    smpi_mpi_request_free(&requests[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
}
