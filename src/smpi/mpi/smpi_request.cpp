/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_request.hpp"

#include "mc/mc.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/mc_replay.h"
#include "SmpiHost.hpp"
#include "private.h"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_op.hpp"
#include "smpi_process.hpp"

#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_request, smpi, "Logging specific to SMPI (reques)");

static simgrid::config::Flag<double> smpi_iprobe_sleep(
  "smpi/iprobe", "Minimum time to inject inside a call to MPI_Iprobe", 1e-4);
static simgrid::config::Flag<double> smpi_test_sleep(
  "smpi/test", "Minimum time to inject inside a call to MPI_Test", 1e-4);

std::vector<s_smpi_factor_t> smpi_ois_values;

extern void (*smpi_comm_copy_data_callback) (smx_activity_t, void*, size_t);

namespace simgrid{
namespace smpi{

Request::Request(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm, unsigned flags) : buf_(buf), old_type_(datatype), src_(src), dst_(dst), tag_(tag), comm_(comm), flags_(flags)
{
  void *old_buf = nullptr;
// FIXME Handle the case of a partial shared malloc.
  if ((((flags & RECV) != 0) && ((flags & ACCUMULATE) != 0)) || (datatype->flags() & DT_FLAG_DERIVED)) {
    // This part handles the problem of non-contiguous memory
    old_buf = buf;
    if (count==0){
      buf_ = nullptr;
    }else {
      buf_ = xbt_malloc(count*datatype->size());
      if ((datatype->flags() & DT_FLAG_DERIVED) && ((flags & SEND) != 0)) {
        datatype->serialize(old_buf, buf_, count);
      }
    }
  }
  // This part handles the problem of non-contiguous memory (for the unserialisation at the reception)
  old_buf_  = old_buf;
  size_ = datatype->size() * count;
  datatype->ref();
  comm_->ref();
  action_          = nullptr;
  detached_        = 0;
  detached_sender_ = nullptr;
  real_src_        = 0;
  truncated_       = 0;
  real_size_       = 0;
  real_tag_        = 0;
  if (flags & PERSISTENT)
    refcount_ = 1;
  else
    refcount_ = 0;
  op_   = MPI_REPLACE;
}

MPI_Comm Request::comm(){
  return comm_;
}

int Request::src(){
  return src_;
}

int Request::dst(){
  return dst_;
}

int Request::tag(){
  return tag_;
}

int Request::flags(){
  return flags_;
}

int Request::detached(){
  return detached_;
}

size_t Request::size(){
  return size_;
}

size_t Request::real_size(){
  return real_size_;
}

MPI_Datatype Request::old_type(){
  return old_type_;
}

void Request::unref(MPI_Request* request)
{
  if((*request) != MPI_REQUEST_NULL){
    (*request)->refcount_--;
    if((*request)->refcount_<0) xbt_die("wrong refcount");
    if((*request)->refcount_==0){
        Datatype::unref((*request)->old_type_);
        Comm::unref((*request)->comm_);
        (*request)->print_request("Destroying");
        delete *request;
        *request = MPI_REQUEST_NULL;
    }else{
      (*request)->print_request("Decrementing");
    }
  }else{
    xbt_die("freeing an already free request");
  }
}

int Request::match_recv(void* a, void* b, simgrid::kernel::activity::CommImpl* ignored)
{
  MPI_Request ref = static_cast<MPI_Request>(a);
  MPI_Request req = static_cast<MPI_Request>(b);
  XBT_DEBUG("Trying to match a recv of src %d against %d, tag %d against %d",ref->src_,req->src_, ref->tag_, req->tag_);

  xbt_assert(ref, "Cannot match recv against null reference");
  xbt_assert(req, "Cannot match recv against null request");
  if((ref->src_ == MPI_ANY_SOURCE || req->src_ == ref->src_)
    && ((ref->tag_ == MPI_ANY_TAG && req->tag_ >=0) || req->tag_ == ref->tag_)){
    //we match, we can transfer some values
    if(ref->src_ == MPI_ANY_SOURCE)
      ref->real_src_ = req->src_;
    if(ref->tag_ == MPI_ANY_TAG)
      ref->real_tag_ = req->tag_;
    if(ref->real_size_ < req->real_size_)
      ref->truncated_ = 1;
    if(req->detached_==1)
      ref->detached_sender_=req; //tie the sender to the receiver, as it is detached and has to be freed in the receiver
    XBT_DEBUG("match succeeded");
    return 1;
  }else return 0;
}

int Request::match_send(void* a, void* b, simgrid::kernel::activity::CommImpl* ignored)
{
  MPI_Request ref = static_cast<MPI_Request>(a);
  MPI_Request req = static_cast<MPI_Request>(b);
  XBT_DEBUG("Trying to match a send of src %d against %d, tag %d against %d",ref->src_,req->src_, ref->tag_, req->tag_);
  xbt_assert(ref, "Cannot match send against null reference");
  xbt_assert(req, "Cannot match send against null request");

  if((req->src_ == MPI_ANY_SOURCE || req->src_ == ref->src_)
      && ((req->tag_ == MPI_ANY_TAG && ref->tag_ >=0)|| req->tag_ == ref->tag_)){
    if(req->src_ == MPI_ANY_SOURCE)
      req->real_src_ = ref->src_;
    if(req->tag_ == MPI_ANY_TAG)
      req->real_tag_ = ref->tag_;
    if(req->real_size_ < ref->real_size_)
      req->truncated_ = 1;
    if(ref->detached_==1)
      req->detached_sender_=ref; //tie the sender to the receiver, as it is detached and has to be freed in the receiver
    XBT_DEBUG("match succeeded");
    return 1;
  } else
    return 0;
}

void Request::print_request(const char *message)
{
  XBT_VERB("%s  request %p  [buf = %p, size = %zu, src = %d, dst = %d, tag = %d, flags = %x]",
       message, this, buf_, size_, src_, dst_, tag_, flags_);
}


/* factories, to hide the internal flags from the caller */
MPI_Request Request::send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{

  return new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process()->index(),
                          comm->group()->index(dst), tag, comm, PERSISTENT | SEND | PREPARED);
}

MPI_Request Request::ssend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  return new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process()->index(),
                        comm->group()->index(dst), tag, comm, PERSISTENT | SSEND | SEND | PREPARED);
}

MPI_Request Request::isend_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  return new Request(buf==MPI_BOTTOM ? nullptr : buf , count, datatype, smpi_process()->index(),
                          comm->group()->index(dst), tag,comm, PERSISTENT | ISEND | SEND | PREPARED);
}


MPI_Request Request::rma_send_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,
                               MPI_Op op)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  if(op==MPI_OP_NULL){
    request = new Request(buf==MPI_BOTTOM ? nullptr : buf , count, datatype, src, dst, tag,
                            comm, RMA | NON_PERSISTENT | ISEND | SEND | PREPARED);
  }else{
    request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,  src, dst, tag,
                            comm, RMA | NON_PERSISTENT | ISEND | SEND | PREPARED | ACCUMULATE);
    request->op_ = op;
  }
  return request;
}

MPI_Request Request::recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm)
{
  return new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,
                          src == MPI_ANY_SOURCE ? MPI_ANY_SOURCE : comm->group()->index(src),
                          smpi_process()->index(), tag, comm, PERSISTENT | RECV | PREPARED);
}

MPI_Request Request::rma_recv_init(void *buf, int count, MPI_Datatype datatype, int src, int dst, int tag, MPI_Comm comm,
                               MPI_Op op)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  if(op==MPI_OP_NULL){
    request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,  src, dst, tag,
                            comm, RMA | NON_PERSISTENT | RECV | PREPARED);
  }else{
    request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype,  src, dst, tag,
                            comm, RMA | NON_PERSISTENT | RECV | PREPARED | ACCUMULATE);
    request->op_ = op;
  }
  return request;
}

MPI_Request Request::irecv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm)
{
  return new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, src == MPI_ANY_SOURCE ? MPI_ANY_SOURCE :
                          comm->group()->index(src), smpi_process()->index(), tag,
                          comm, PERSISTENT | RECV | PREPARED);
}

MPI_Request Request::isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request =  new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process()->index(),
                           comm->group()->index(dst), tag, comm, NON_PERSISTENT | ISEND | SEND);
  request->start();
  return request;
}

MPI_Request Request::issend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process()->index(),
                        comm->group()->index(dst), tag,comm, NON_PERSISTENT | ISEND | SSEND | SEND);
  request->start();
  return request;
}


MPI_Request Request::irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, src == MPI_ANY_SOURCE ? MPI_ANY_SOURCE :
                          comm->group()->index(src), smpi_process()->index(), tag, comm,
                          NON_PERSISTENT | RECV);
  request->start();
  return request;
}

void Request::recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = irecv(buf, count, datatype, src, tag, comm);
  wait(&request,status);
  request = nullptr;
}

void Request::send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process()->index(),
                          comm->group()->index(dst), tag, comm, NON_PERSISTENT | SEND);

  request->start();
  wait(&request, MPI_STATUS_IGNORE);
  request = nullptr;
}

void Request::ssend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = nullptr; /* MC needs the comm to be set to nullptr during the call */
  request = new Request(buf==MPI_BOTTOM ? nullptr : buf, count, datatype, smpi_process()->index(),
                          comm->group()->index(dst), tag, comm, NON_PERSISTENT | SSEND | SEND);

  request->start();
  wait(&request,MPI_STATUS_IGNORE);
  request = nullptr;
}

void Request::sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,int dst, int sendtag,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag,
                       MPI_Comm comm, MPI_Status * status)
{
  MPI_Request requests[2];
  MPI_Status stats[2];
  int myid=smpi_process()->index();
  if ((comm->group()->index(dst) == myid) && (comm->group()->index(src) == myid)){
      Datatype::copy(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype);
      return;
  }
  requests[0] = isend_init(sendbuf, sendcount, sendtype, dst, sendtag, comm);
  requests[1] = irecv_init(recvbuf, recvcount, recvtype, src, recvtag, comm);
  startall(2, requests);
  waitall(2, requests, stats);
  unref(&requests[0]);
  unref(&requests[1]);
  if(status != MPI_STATUS_IGNORE) {
    // Copy receive status
    *status = stats[1];
  }
}

void Request::start()
{
  smx_mailbox_t mailbox;

  xbt_assert(action_ == nullptr, "Cannot (re-)start unfinished communication");
  flags_ &= ~PREPARED;
  flags_ &= ~FINISHED;
  refcount_++;

  if ((flags_ & RECV) != 0) {
    this->print_request("New recv");

    simgrid::smpi::Process* process = smpi_process_remote(dst_);

    int async_small_thresh = xbt_cfg_get_int("smpi/async-small-thresh");

    xbt_mutex_t mut = process->mailboxes_mutex();
    if (async_small_thresh != 0 || (flags_ & RMA) != 0)
      xbt_mutex_acquire(mut);

    if (async_small_thresh == 0 && (flags_ & RMA) == 0 ) {
      mailbox = process->mailbox();
    }
    else if (((flags_ & RMA) != 0) || static_cast<int>(size_) < async_small_thresh) {
      //We have to check both mailboxes (because SSEND messages are sent to the large mbox).
      //begin with the more appropriate one : the small one.
      mailbox = process->mailbox_small();
      XBT_DEBUG("Is there a corresponding send already posted in the small mailbox %p (in case of SSEND)?", mailbox);
      smx_activity_t action = simcall_comm_iprobe(mailbox, 0, &match_recv, static_cast<void*>(this));

      if (action == nullptr) {
        mailbox = process->mailbox();
        XBT_DEBUG("No, nothing in the small mailbox test the other one : %p", mailbox);
        action = simcall_comm_iprobe(mailbox, 0, &match_recv, static_cast<void*>(this));
        if (action == nullptr) {
          XBT_DEBUG("Still nothing, switch back to the small mailbox : %p", mailbox);
          mailbox = process->mailbox_small();
        }
      } else {
        XBT_DEBUG("yes there was something for us in the large mailbox");
      }
    } else {
      mailbox = process->mailbox_small();
      XBT_DEBUG("Is there a corresponding send already posted the small mailbox?");
      smx_activity_t action = simcall_comm_iprobe(mailbox, 0, &match_recv, static_cast<void*>(this));

      if (action == nullptr) {
        XBT_DEBUG("No, nothing in the permanent receive mailbox");
        mailbox = process->mailbox();
      } else {
        XBT_DEBUG("yes there was something for us in the small mailbox");
      }
    }

    // we make a copy here, as the size is modified by simix, and we may reuse the request in another receive later
    real_size_=size_;
    action_   = simcall_comm_irecv(
        process->process(), mailbox, buf_, &real_size_, &match_recv,
        process->replaying() ? &smpi_comm_null_copy_buffer_callback : smpi_comm_copy_data_callback, this, -1.0);
    XBT_DEBUG("recv simcall posted");

    if (async_small_thresh != 0 || (flags_ & RMA) != 0 )
      xbt_mutex_release(mut);
  } else { /* the RECV flag was not set, so this is a send */
    simgrid::smpi::Process* process = smpi_process_remote(dst_);
    int rank = src_;
    if (TRACE_smpi_view_internals()) {
      TRACE_smpi_send(rank, rank, dst_, tag_, size_);
    }
    this->print_request("New send");

    void* buf = buf_;
    if ((flags_ & SSEND) == 0 && ( (flags_ & RMA) != 0
        || static_cast<int>(size_) < xbt_cfg_get_int("smpi/send-is-detached-thresh") ) ) {
      void *oldbuf = nullptr;
      detached_ = 1;
      XBT_DEBUG("Send request %p is detached", this);
      refcount_++;
      if (not(old_type_->flags() & DT_FLAG_DERIVED)) {
        oldbuf = buf_;
        if (not process->replaying() && oldbuf != nullptr && size_ != 0) {
          if((smpi_privatize_global_variables != 0)
            && (static_cast<char*>(buf_) >= smpi_start_data_exe)
            && (static_cast<char*>(buf_) < smpi_start_data_exe + smpi_size_data_exe )){
            XBT_DEBUG("Privatization : We are sending from a zone inside global memory. Switch data segment ");
            smpi_switch_data_segment(src_);
          }
          buf = xbt_malloc(size_);
          memcpy(buf,oldbuf,size_);
          XBT_DEBUG("buf %p copied into %p",oldbuf,buf);
        }
      }
    }

    //if we are giving back the control to the user without waiting for completion, we have to inject timings
    double sleeptime = 0.0;
    if (detached_ != 0 || ((flags_ & (ISEND | SSEND)) != 0)) { // issend should be treated as isend
      // isend and send timings may be different
      sleeptime = ((flags_ & ISEND) != 0)
                      ? simgrid::s4u::Actor::self()->getHost()->extension<simgrid::smpi::SmpiHost>()->oisend(size_)
                      : simgrid::s4u::Actor::self()->getHost()->extension<simgrid::smpi::SmpiHost>()->osend(size_);
    }

    if(sleeptime > 0.0){
      simcall_process_sleep(sleeptime);
      XBT_DEBUG("sending size of %zu : sleep %f ", size_, sleeptime);
    }

    int async_small_thresh = xbt_cfg_get_int("smpi/async-small-thresh");

    xbt_mutex_t mut=process->mailboxes_mutex();

    if (async_small_thresh != 0 || (flags_ & RMA) != 0)
      xbt_mutex_acquire(mut);

    if (not(async_small_thresh != 0 || (flags_ & RMA) != 0)) {
      mailbox = process->mailbox();
    } else if (((flags_ & RMA) != 0) || static_cast<int>(size_) < async_small_thresh) { // eager mode
      mailbox = process->mailbox();
      XBT_DEBUG("Is there a corresponding recv already posted in the large mailbox %p?", mailbox);
      smx_activity_t action = simcall_comm_iprobe(mailbox, 1, &match_send, static_cast<void*>(this));
      if (action == nullptr) {
        if ((flags_ & SSEND) == 0){
          mailbox = process->mailbox_small();
          XBT_DEBUG("No, nothing in the large mailbox, message is to be sent on the small one %p", mailbox);
        } else {
          mailbox = process->mailbox_small();
          XBT_DEBUG("SSEND : Is there a corresponding recv already posted in the small mailbox %p?", mailbox);
          action = simcall_comm_iprobe(mailbox, 1, &match_send, static_cast<void*>(this));
          if (action == nullptr) {
            XBT_DEBUG("No, we are first, send to large mailbox");
            mailbox = process->mailbox();
          }
        }
      } else {
        XBT_DEBUG("Yes there was something for us in the large mailbox");
      }
    } else {
      mailbox = process->mailbox();
      XBT_DEBUG("Send request %p is in the large mailbox %p (buf: %p)",mailbox, this,buf_);
    }

    // we make a copy here, as the size is modified by simix, and we may reuse the request in another receive later
    real_size_=size_;
    action_   = simcall_comm_isend(
        SIMIX_process_from_PID(src_ + 1), mailbox, size_, -1.0, buf, real_size_, &match_send,
        &xbt_free_f, // how to free the userdata if a detached send fails
        not process->replaying() ? smpi_comm_copy_data_callback : &smpi_comm_null_copy_buffer_callback, this,
        // detach if msg size < eager/rdv switch limit
        detached_);
    XBT_DEBUG("send simcall posted");

    /* FIXME: detached sends are not traceable (action_ == nullptr) */
    if (action_ != nullptr)
      simcall_set_category(action_, TRACE_internal_smpi_get_category());
    if (async_small_thresh != 0 || ((flags_ & RMA)!=0))
      xbt_mutex_release(mut);
  }
}

void Request::startall(int count, MPI_Request * requests)
{
  if(requests== nullptr)
    return;

  for(int i = 0; i < count; i++) {
    requests[i]->start();
  }
}

int Request::test(MPI_Request * request, MPI_Status * status) {
  //assume that request is not MPI_REQUEST_NULL (filtered in PMPI_Test or testall before)
  // to avoid deadlocks if used as a break condition, such as
  //     while (MPI_Test(request, flag, status) && flag) dostuff...
  // because the time will not normally advance when only calls to MPI_Test are made -> deadlock
  // multiplier to the sleeptime, to increase speed of execution, each failed test will increase it
  static int nsleeps = 1;
  if(smpi_test_sleep > 0)
    simcall_process_sleep(nsleeps*smpi_test_sleep);

  Status::empty(status);
  int flag = 1;
  if (((*request)->flags_ & PREPARED) == 0) {
    if ((*request)->action_ != nullptr)
      flag = simcall_comm_test((*request)->action_);
    if (flag) {
      finish_wait(request,status);
      nsleeps=1;//reset the number of sleeps we will do next time
      if (*request != MPI_REQUEST_NULL && ((*request)->flags_ & PERSISTENT)==0)
      *request = MPI_REQUEST_NULL;
    } else if (xbt_cfg_get_boolean("smpi/grow-injected-times")){
      nsleeps++;
    }
  }
  return flag;
}

int Request::testsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[])
{
  int count = 0;
  int count_dead = 0;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;

  for (int i = 0; i < incount; i++) {
    if (requests[i] != MPI_REQUEST_NULL) {
      if (test(&requests[i], pstat)) {
        indices[i] = 1;
        count++;
        if (status != MPI_STATUSES_IGNORE)
          status[i] = *pstat;
        if ((requests[i] != MPI_REQUEST_NULL) && (requests[i]->flags_ & NON_PERSISTENT))
          requests[i] = MPI_REQUEST_NULL;
      }
    } else {
      count_dead++;
    }
  }
  if(count_dead==incount)
    return MPI_UNDEFINED;
  else return count;
}

int Request::testany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  std::vector<simgrid::kernel::activity::ActivityImplPtr> comms;
  comms.reserve(count);

  int i;
  int flag = 0;

  *index = MPI_UNDEFINED;

  std::vector<int> map; /** Maps all matching comms back to their location in requests **/
  for(i = 0; i < count; i++) {
    if ((requests[i] != MPI_REQUEST_NULL) && requests[i]->action_ && not(requests[i]->flags_ & PREPARED)) {
      comms.push_back(requests[i]->action_);
      map.push_back(i);
    }
  }
  if (not map.empty()) {
    //multiplier to the sleeptime, to increase speed of execution, each failed testany will increase it
    static int nsleeps = 1;
    if(smpi_test_sleep > 0)
      simcall_process_sleep(nsleeps*smpi_test_sleep);

    i = simcall_comm_testany(comms.data(), comms.size()); // The i-th element in comms matches!
    if (i != -1) { // -1 is not MPI_UNDEFINED but a SIMIX return code. (nothing matches)
      *index = map[i];
      finish_wait(&requests[*index],status);
      flag             = 1;
      nsleeps          = 1;
      if (requests[*index] != MPI_REQUEST_NULL && (requests[*index]->flags_ & NON_PERSISTENT)) {
        requests[*index] = MPI_REQUEST_NULL;
      }
    } else {
      nsleeps++;
    }
  } else {
      //all requests are null or inactive, return true
      flag = 1;
      Status::empty(status);
  }

  return flag;
}

int Request::testall(int count, MPI_Request requests[], MPI_Status status[])
{
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;
  int flag=1;
  for(int i=0; i<count; i++){
    if (requests[i] != MPI_REQUEST_NULL && not(requests[i]->flags_ & PREPARED)) {
      if (test(&requests[i], pstat)!=1){
        flag=0;
      }else{
          requests[i]=MPI_REQUEST_NULL;
      }
    }else{
      Status::empty(pstat);
    }
    if(status != MPI_STATUSES_IGNORE) {
      status[i] = *pstat;
    }
  }
  return flag;
}

void Request::probe(int source, int tag, MPI_Comm comm, MPI_Status* status){
  int flag=0;
  //FIXME find another way to avoid busy waiting ?
  // the issue here is that we have to wait on a nonexistent comm
  while(flag==0){
    iprobe(source, tag, comm, &flag, status);
    XBT_DEBUG("Busy Waiting on probing : %d", flag);
  }
}

void Request::iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status){
  // to avoid deadlock, we have to sleep some time here, or the timer won't advance and we will only do iprobe simcalls
  // especially when used as a break condition, such as while (MPI_Iprobe(...)) dostuff...
  // nsleeps is a multiplier to the sleeptime, to increase speed of execution, each failed iprobe will increase it
  // This can speed up the execution of certain applications by an order of magnitude, such as HPL
  static int nsleeps = 1;
  double speed        = simgrid::s4u::Actor::self()->getHost()->getSpeed();
  double maxrate = xbt_cfg_get_double("smpi/iprobe-cpu-usage");
  MPI_Request request = new Request(nullptr, 0, MPI_CHAR, source == MPI_ANY_SOURCE ? MPI_ANY_SOURCE :
                 comm->group()->index(source), comm->rank(), tag, comm, PERSISTENT | RECV);
  if (smpi_iprobe_sleep > 0) {
    smx_activity_t iprobe_sleep = simcall_execution_start("iprobe", /* flops to executek*/nsleeps*smpi_iprobe_sleep*speed*maxrate, /* priority */1.0, /* performance bound */maxrate*speed);
    simcall_execution_wait(iprobe_sleep);
  }
  // behave like a receive, but don't do it
  smx_mailbox_t mailbox;

  request->print_request("New iprobe");
  // We have to test both mailboxes as we don't know if we will receive one one or another
  if (xbt_cfg_get_int("smpi/async-small-thresh") > 0){
      mailbox = smpi_process()->mailbox_small();
      XBT_DEBUG("Trying to probe the perm recv mailbox");
      request->action_ = simcall_comm_iprobe(mailbox, 0, &match_recv, static_cast<void*>(request));
  }

  if (request->action_ == nullptr){
    mailbox = smpi_process()->mailbox();
    XBT_DEBUG("trying to probe the other mailbox");
    request->action_ = simcall_comm_iprobe(mailbox, 0, &match_recv, static_cast<void*>(request));
  }

  if (request->action_ != nullptr){
    simgrid::kernel::activity::CommImplPtr sync_comm =
        boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(request->action_);
    MPI_Request req                            = static_cast<MPI_Request>(sync_comm->src_data);
    *flag = 1;
    if(status != MPI_STATUS_IGNORE && (req->flags_ & PREPARED) == 0) {
      status->MPI_SOURCE = comm->group()->rank(req->src_);
      status->MPI_TAG    = req->tag_;
      status->MPI_ERROR  = MPI_SUCCESS;
      status->count      = req->real_size_;
    }
    nsleeps = 1;//reset the number of sleeps we will do next time
  }
  else {
    *flag = 0;
    if (xbt_cfg_get_boolean("smpi/grow-injected-times"))
      nsleeps++;
  }
  unref(&request);
}

void Request::finish_wait(MPI_Request* request, MPI_Status * status)
{
  MPI_Request req = *request;
  Status::empty(status);

  if (not((req->detached_ != 0) && ((req->flags_ & SEND) != 0)) && ((req->flags_ & PREPARED) == 0)) {
    if(status != MPI_STATUS_IGNORE) {
      int src = req->src_ == MPI_ANY_SOURCE ? req->real_src_ : req->src_;
      status->MPI_SOURCE = req->comm_->group()->rank(src);
      status->MPI_TAG = req->tag_ == MPI_ANY_TAG ? req->real_tag_ : req->tag_;
      status->MPI_ERROR = req->truncated_ != 0 ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
      // this handles the case were size in receive differs from size in send
      status->count = req->real_size_;
    }

    req->print_request("Finishing");
    MPI_Datatype datatype = req->old_type_;

// FIXME Handle the case of a partial shared malloc.
    if (((req->flags_ & ACCUMULATE) != 0) ||
        (datatype->flags() & DT_FLAG_DERIVED)) { // && (not smpi_is_shared(req->old_buf_))){

      if (not smpi_process()->replaying()) {
        if( smpi_privatize_global_variables != 0 && (static_cast<char*>(req->old_buf_) >= smpi_start_data_exe)
            && ((char*)req->old_buf_ < smpi_start_data_exe + smpi_size_data_exe )){
            XBT_VERB("Privatization : We are unserializing to a zone in global memory  Switch data segment ");
            smpi_switch_data_segment(smpi_process()->index());
        }
      }

      if(datatype->flags() & DT_FLAG_DERIVED){
        // This part handles the problem of non-contignous memory the unserialization at the reception
        if((req->flags_ & RECV) && datatype->size()!=0)
          datatype->unserialize(req->buf_, req->old_buf_, req->real_size_/datatype->size() , req->op_);
        xbt_free(req->buf_);
      }else if(req->flags_ & RECV){//apply op on contiguous buffer for accumulate
          if(datatype->size()!=0){
            int n =req->real_size_/datatype->size();
            req->op_->apply(req->buf_, req->old_buf_, &n, datatype);
          }
          xbt_free(req->buf_);
      }
    }
  }

  if (TRACE_smpi_view_internals() && ((req->flags_ & RECV) != 0)){
    int rank = smpi_process()->index();
    int src_traced = (req->src_ == MPI_ANY_SOURCE ? req->real_src_ : req->src_);
    TRACE_smpi_recv(rank, src_traced, rank,req->tag_);
  }
  if(req->detached_sender_ != nullptr){
    //integrate pseudo-timing for buffering of small messages, do not bother to execute the simcall if 0
    double sleeptime =
        simgrid::s4u::Actor::self()->getHost()->extension<simgrid::smpi::SmpiHost>()->orecv(req->real_size());
    if(sleeptime > 0.0){
      simcall_process_sleep(sleeptime);
      XBT_DEBUG("receiving size of %zu : sleep %f ", req->real_size_, sleeptime);
    }
    unref(&(req->detached_sender_));
  }
  if(req->flags_ & PERSISTENT)
    req->action_ = nullptr;
  req->flags_ |= FINISHED;
  unref(request);
}

void Request::wait(MPI_Request * request, MPI_Status * status)
{
  (*request)->print_request("Waiting");
  if ((*request)->flags_ & PREPARED) {
    Status::empty(status);
    return;
  }

  if ((*request)->action_ != nullptr)
    // this is not a detached send
    simcall_comm_wait((*request)->action_, -1.0);

  finish_wait(request,status);
  if (*request != MPI_REQUEST_NULL && (((*request)->flags_ & NON_PERSISTENT)!=0))
    *request = MPI_REQUEST_NULL;
}

int Request::waitany(int count, MPI_Request requests[], MPI_Status * status)
{
  s_xbt_dynar_t comms; // Keep it on stack to save some extra mallocs
  int index = MPI_UNDEFINED;

  if(count > 0) {
    int size = 0;
    // Wait for a request to complete
    xbt_dynar_init(&comms, sizeof(smx_activity_t), [](void*ptr){
      intrusive_ptr_release(*(simgrid::kernel::activity::ActivityImpl**)ptr);
    });
    int *map = xbt_new(int, count);
    XBT_DEBUG("Wait for one of %d", count);
    for(int i = 0; i < count; i++) {
      if (requests[i] != MPI_REQUEST_NULL && not(requests[i]->flags_ & PREPARED) &&
          not(requests[i]->flags_ & FINISHED)) {
        if (requests[i]->action_ != nullptr) {
          XBT_DEBUG("Waiting any %p ", requests[i]);
          intrusive_ptr_add_ref(requests[i]->action_.get());
          xbt_dynar_push_as(&comms, simgrid::kernel::activity::ActivityImpl*, requests[i]->action_.get());
          map[size] = i;
          size++;
        } else {
          // This is a finished detached request, let's return this one
          size  = 0; // so we free the dynar but don't do the waitany call
          index = i;
          finish_wait(&requests[i], status); // cleanup if refcount = 0
          if (requests[i] != MPI_REQUEST_NULL && (requests[i]->flags_ & NON_PERSISTENT))
            requests[i] = MPI_REQUEST_NULL; // set to null
          break;
        }
      }
    }
    if (size > 0) {
      XBT_DEBUG("Enter waitany for %lu comms", xbt_dynar_length(&comms));
      int i = simcall_comm_waitany(&comms, -1);

      // not MPI_UNDEFINED, as this is a simix return code
      if (i != -1) {
        index = map[i];
        //in case of an accumulate, we have to wait the end of all requests to apply the operation, ordered correctly.
        if ((requests[index] == MPI_REQUEST_NULL) ||
            (not((requests[index]->flags_ & ACCUMULATE) && (requests[index]->flags_ & RECV)))) {
          finish_wait(&requests[index],status);
          if (requests[i] != MPI_REQUEST_NULL && (requests[i]->flags_ & NON_PERSISTENT))
            requests[index] = MPI_REQUEST_NULL;
        }
      }
    }

    xbt_dynar_free_data(&comms);
    xbt_free(map);
  }

  if (index==MPI_UNDEFINED)
    Status::empty(status);

  return index;
}

static int sort_accumulates(MPI_Request a, MPI_Request b)
{
  return (a->tag() > b->tag());
}

int Request::waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  std::vector<MPI_Request> accumulates;
  int index;
  MPI_Status stat;
  MPI_Status *pstat = (status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat);
  int retvalue = MPI_SUCCESS;
  //tag invalid requests in the set
  if (status != MPI_STATUSES_IGNORE) {
    for (int c = 0; c < count; c++) {
      if (requests[c] == MPI_REQUEST_NULL || requests[c]->dst_ == MPI_PROC_NULL || (requests[c]->flags_ & PREPARED)) {
        Status::empty(&status[c]);
      } else if (requests[c]->src_ == MPI_PROC_NULL) {
        Status::empty(&status[c]);
        status[c].MPI_SOURCE = MPI_PROC_NULL;
      }
    }
  }
  for (int c = 0; c < count; c++) {
    if (MC_is_active() || MC_record_replay_is_active()) {
      wait(&requests[c],pstat);
      index = c;
    } else {
      index = waitany(count, (MPI_Request*)requests, pstat);
      if (index == MPI_UNDEFINED)
        break;

      if (requests[index] != MPI_REQUEST_NULL
           && (requests[index]->flags_ & RECV)
           && (requests[index]->flags_ & ACCUMULATE))
        accumulates.push_back(requests[index]);
      if (requests[index] != MPI_REQUEST_NULL && (requests[index]->flags_ & NON_PERSISTENT))
        requests[index] = MPI_REQUEST_NULL;
    }
    if (status != MPI_STATUSES_IGNORE) {
      status[index] = *pstat;
      if (status[index].MPI_ERROR == MPI_ERR_TRUNCATE)
        retvalue = MPI_ERR_IN_STATUS;
    }
  }

  if (not accumulates.empty()) {
    std::sort(accumulates.begin(), accumulates.end(), sort_accumulates);
    for (auto req : accumulates) {
      finish_wait(&req, status);
    }
  }

  return retvalue;
}

int Request::waitsome(int incount, MPI_Request requests[], int *indices, MPI_Status status[])
{
  int count = 0;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;

  for (int i = 0; i < incount; i++) {
    int index = waitany(incount, requests, pstat);
    if(index!=MPI_UNDEFINED){
      indices[count] = index;
      count++;
      if(status != MPI_STATUSES_IGNORE) {
        status[index] = *pstat;
      }
     if (requests[index] != MPI_REQUEST_NULL && (requests[index]->flags_ & NON_PERSISTENT))
     requests[index]=MPI_REQUEST_NULL;
    }else{
      return MPI_UNDEFINED;
    }
  }
  return count;
}

MPI_Request Request::f2c(int id) {
  char key[KEY_SIZE];
  if(id==MPI_FORTRAN_REQUEST_NULL)
    return static_cast<MPI_Request>(MPI_REQUEST_NULL);
  return static_cast<MPI_Request>(xbt_dict_get(F2C::f2c_lookup(), get_key_id(key, id)));
}

int Request::add_f() {
  if(F2C::f2c_lookup()==nullptr){
    F2C::set_f2c_lookup(xbt_dict_new_homogeneous(nullptr));
  }
  char key[KEY_SIZE];
  xbt_dict_set(F2C::f2c_lookup(), get_key_id(key, F2C::f2c_id()), this, nullptr);
  F2C::f2c_id_increment();
  return F2C::f2c_id()-1;
}

void Request::free_f(int id) {
  if (id != MPI_FORTRAN_REQUEST_NULL) {
    char key[KEY_SIZE];
    xbt_dict_remove(F2C::f2c_lookup(), get_key_id(key, id));
  }
}

}
}



