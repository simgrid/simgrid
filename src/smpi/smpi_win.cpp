/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_rma, smpi, "Logging specific to SMPI (RMA operations)");

namespace simgrid{
namespace smpi{

Win::Win(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm): base_(base), size_(size), disp_unit_(disp_unit), assert_(0), info_(info), comm_(comm){
  int comm_size = comm->size();
  int rank      = comm->rank();
  XBT_DEBUG("Creating window");
  if(info!=MPI_INFO_NULL)
    info->ref();
  name_ = nullptr;
  opened_ = 0;
  group_ = MPI_GROUP_NULL;
  requests_ = new std::vector<MPI_Request>();
  mut_=xbt_mutex_init();
  connected_wins_ = new MPI_Win[comm_size];
  connected_wins_[rank] = this;
  count_ = 0;
  if(rank==0){
    bar_ = MSG_barrier_init(comm_size);
  }
  mpi_coll_allgather_fun(&(connected_wins_[rank]), sizeof(MPI_Win), MPI_BYTE, connected_wins_, sizeof(MPI_Win),
                         MPI_BYTE, comm);

  mpi_coll_bcast_fun(&(bar_), sizeof(msg_bar_t), MPI_BYTE, 0, comm);

  mpi_coll_barrier_fun(comm);
}

Win::~Win(){
  //As per the standard, perform a barrier to ensure every async comm is finished
  MSG_barrier_wait(bar_);
  xbt_mutex_acquire(mut_);
  delete requests_;
  xbt_mutex_release(mut_);
  delete[] connected_wins_;
  if (name_ != nullptr){
    xbt_free(name_);
  }
  if(info_!=MPI_INFO_NULL){
    MPI_Info_free(&info_);
  }

  mpi_coll_barrier_fun(comm_);
  int rank=comm_->rank();
  if(rank == 0)
    MSG_barrier_destroy(bar_);
  xbt_mutex_destroy(mut_);
}

void Win::get_name(char* name, int* length){
  if(name_==nullptr){
    *length=0;
    name=nullptr;
    return;
  }
  *length = strlen(name_);
  strncpy(name, name_, *length+1);
}

void Win::get_group(MPI_Group* group){
  if(comm_ != MPI_COMM_NULL){
    *group = comm_->group();
  } else {
    *group = MPI_GROUP_NULL;
  }
}

void Win::set_name(char* name){
  name_ = xbt_strdup(name);
}

int Win::fence(int assert)
{
  XBT_DEBUG("Entering fence");
  if (opened_ == 0)
    opened_=1;
  if (assert != MPI_MODE_NOPRECEDE) {
    // This is not the first fence => finalize what came before
    MSG_barrier_wait(bar_);
    xbt_mutex_acquire(mut_);
    // This (simulated) mutex ensures that no process pushes to the vector of requests during the waitall.
    // Without this, the vector could get redimensionned when another process pushes.
    // This would result in the array used by Request::waitall() to be invalidated.
    // Another solution would be to copy the data and cleanup the vector *before* Request::waitall
    std::vector<MPI_Request> *reqs = requests_;
    int size = static_cast<int>(reqs->size());
    // start all requests that have been prepared by another process
    if (size > 0) {
      for (const auto& req : *reqs) {
        if (req && (req->flags() & PREPARED))
          req->start();
      }

      MPI_Request* treqs = &(*reqs)[0];

      Request::waitall(size, treqs, MPI_STATUSES_IGNORE);
    }
    count_=0;
    xbt_mutex_release(mut_);
  }
  assert_ = assert;

  MSG_barrier_wait(bar_);
  XBT_DEBUG("Leaving fence");

  return MPI_SUCCESS;
}

int Win::put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype)
{
  if(opened_==0)//check that post/start has been done
    return MPI_ERR_WIN;
  //get receiver pointer
  MPI_Win recv_win = connected_wins_[target_rank];

  if(target_count*target_datatype->get_extent()>recv_win->size_)
    return MPI_ERR_ARG;

  void* recv_addr = static_cast<void*> ( static_cast<char*>(recv_win->base_) + target_disp * recv_win->disp_unit_);
  XBT_DEBUG("Entering MPI_Put to %d", target_rank);

  if(target_rank != comm_->rank()){
    //prepare send_request
    MPI_Request sreq = Request::rma_send_init(origin_addr, origin_count, origin_datatype, smpi_process_index(),
        comm_->group()->index(target_rank), SMPI_RMA_TAG+1, comm_, MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = Request::rma_recv_init(recv_addr, target_count, target_datatype, smpi_process_index(),
        comm_->group()->index(target_rank), SMPI_RMA_TAG+1, recv_win->comm_, MPI_OP_NULL);

    //push request to receiver's win
    xbt_mutex_acquire(recv_win->mut_);
    recv_win->requests_->push_back(rreq);
    xbt_mutex_release(recv_win->mut_);
    //start send
    sreq->start();

    //push request to sender's win
    xbt_mutex_acquire(mut_);
    requests_->push_back(sreq);
    xbt_mutex_release(mut_);
  }else{
    Datatype::copy(origin_addr, origin_count, origin_datatype, recv_addr, target_count, target_datatype);
  }

  return MPI_SUCCESS;
}

int Win::get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype)
{
  if(opened_==0)//check that post/start has been done
    return MPI_ERR_WIN;
  //get sender pointer
  MPI_Win send_win = connected_wins_[target_rank];

  if(target_count*target_datatype->get_extent()>send_win->size_)
    return MPI_ERR_ARG;

  void* send_addr = static_cast<void*>(static_cast<char*>(send_win->base_) + target_disp * send_win->disp_unit_);
  XBT_DEBUG("Entering MPI_Get from %d", target_rank);

  if(target_rank != comm_->rank()){
    //prepare send_request
    MPI_Request sreq = Request::rma_send_init(send_addr, target_count, target_datatype,
        comm_->group()->index(target_rank), smpi_process_index(), SMPI_RMA_TAG+2, send_win->comm_,
        MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = Request::rma_recv_init(origin_addr, origin_count, origin_datatype,
        comm_->group()->index(target_rank), smpi_process_index(), SMPI_RMA_TAG+2, comm_,
        MPI_OP_NULL);

    //start the send, with another process than us as sender. 
    sreq->start();
    //push request to receiver's win
    xbt_mutex_acquire(send_win->mut_);
    send_win->requests_->push_back(sreq);
    xbt_mutex_release(send_win->mut_);

    //start recv
    rreq->start();
    //push request to sender's win
    xbt_mutex_acquire(mut_);
    requests_->push_back(rreq);
    xbt_mutex_release(mut_);
  }else{
    Datatype::copy(send_addr, target_count, target_datatype, origin_addr, origin_count, origin_datatype);
  }

  return MPI_SUCCESS;
}


int Win::accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op)
{
  if(opened_==0)//check that post/start has been done
    return MPI_ERR_WIN;
  //FIXME: local version 
  //get receiver pointer
  MPI_Win recv_win = connected_wins_[target_rank];

  if(target_count*target_datatype->get_extent()>recv_win->size_)
    return MPI_ERR_ARG;

  void* recv_addr = static_cast<void*>(static_cast<char*>(recv_win->base_) + target_disp * recv_win->disp_unit_);
  XBT_DEBUG("Entering MPI_Accumulate to %d", target_rank);
    //As the tag will be used for ordering of the operations, add count to it
    //prepare send_request
    MPI_Request sreq = Request::rma_send_init(origin_addr, origin_count, origin_datatype,
        smpi_process_index(), comm_->group()->index(target_rank), SMPI_RMA_TAG+3+count_, comm_, op);

    //prepare receiver request
    MPI_Request rreq = Request::rma_recv_init(recv_addr, target_count, target_datatype,
        smpi_process_index(), comm_->group()->index(target_rank), SMPI_RMA_TAG+3+count_, recv_win->comm_, op);

    count_++;
    //push request to receiver's win
    xbt_mutex_acquire(recv_win->mut_);
    recv_win->requests_->push_back(rreq);
    xbt_mutex_release(recv_win->mut_);
    //start send
    sreq->start();

    //push request to sender's win
    xbt_mutex_acquire(mut_);
    requests_->push_back(sreq);
    xbt_mutex_release(mut_);

  return MPI_SUCCESS;
}

int Win::start(MPI_Group group, int assert){
    /* From MPI forum advices
    The call to MPI_WIN_COMPLETE does not return until the put call has completed at the origin; and the target window
    will be accessed by the put operation only after the call to MPI_WIN_START has matched a call to MPI_WIN_POST by
    the target process. This still leaves much choice to implementors. The call to MPI_WIN_START can block until the
    matching call to MPI_WIN_POST occurs at all target processes. One can also have implementations where the call to
    MPI_WIN_START is nonblocking, but the call to MPI_PUT blocks until the matching call to MPI_WIN_POST occurred; or
    implementations where the first two calls are nonblocking, but the call to MPI_WIN_COMPLETE blocks until the call
    to MPI_WIN_POST occurred; or even implementations where all three calls can complete before any target process
    called MPI_WIN_POST --- the data put must be buffered, in this last case, so as to allow the put to complete at the
    origin ahead of its completion at the target. However, once the call to MPI_WIN_POST is issued, the sequence above
    must complete, without further dependencies.  */

  //naive, blocking implementation.
    int i             = 0;
    int j             = 0;
    int size          = group->size();
    MPI_Request* reqs = xbt_new0(MPI_Request, size);

    while (j != size) {
      int src = group->index(j);
      if (src != smpi_process_index() && src != MPI_UNDEFINED) {
        reqs[i] = Request::irecv_init(nullptr, 0, MPI_CHAR, src, SMPI_RMA_TAG + 4, MPI_COMM_WORLD);
        i++;
      }
      j++;
  }
  size=i;
  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);
  opened_++; //we're open for business !
  group_=group;
  group->ref();
  return MPI_SUCCESS;
}

int Win::post(MPI_Group group, int assert){
  //let's make a synchronous send here
  int i             = 0;
  int j             = 0;
  int size = group->size();
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int dst=group->index(j);
    if(dst!=smpi_process_index() && dst!=MPI_UNDEFINED){
      reqs[i]=Request::send_init(nullptr, 0, MPI_CHAR, dst, SMPI_RMA_TAG+4, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;

  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);
  opened_++; //we're open for business !
  group_=group;
  group->ref();
  return MPI_SUCCESS;
}

int Win::complete(){
  if(opened_==0)
    xbt_die("Complete called on already opened MPI_Win");

  XBT_DEBUG("Entering MPI_Win_Complete");
  int i             = 0;
  int j             = 0;
  int size = group_->size();
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int dst=group_->index(j);
    if(dst!=smpi_process_index() && dst!=MPI_UNDEFINED){
      reqs[i]=Request::send_init(nullptr, 0, MPI_CHAR, dst, SMPI_RMA_TAG+5, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  XBT_DEBUG("Win_complete - Sending sync messages to %d processes", size);
  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);

  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);

  //now we can finish RMA calls
  xbt_mutex_acquire(mut_);
  std::vector<MPI_Request> *reqqs = requests_;
  size = static_cast<int>(reqqs->size());

  XBT_DEBUG("Win_complete - Finishing %d RMA calls", size);
  if (size > 0) {
    // start all requests that have been prepared by another process
    for (const auto& req : *reqqs) {
      if (req && (req->flags() & PREPARED))
        req->start();
    }

    MPI_Request* treqs = &(*reqqs)[0];
    Request::waitall(size, treqs, MPI_STATUSES_IGNORE);
    reqqs->clear();
  }
  xbt_mutex_release(mut_);

  Group::unref(group_);
  opened_--; //we're closed for business !
  return MPI_SUCCESS;
}

int Win::wait(){
  //naive, blocking implementation.
  XBT_DEBUG("Entering MPI_Win_Wait");
  int i=0,j=0;
  int size = group_->size();
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int src=group_->index(j);
    if(src!=smpi_process_index() && src!=MPI_UNDEFINED){
      reqs[i]=Request::irecv_init(nullptr, 0, MPI_CHAR, src,SMPI_RMA_TAG+5, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  XBT_DEBUG("Win_wait - Receiving sync messages from %d processes", size);
  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);
  xbt_mutex_acquire(mut_);
  std::vector<MPI_Request> *reqqs = requests_;
  size = static_cast<int>(reqqs->size());

  XBT_DEBUG("Win_wait - Finishing %d RMA calls", size);
  if (size > 0) {
    // start all requests that have been prepared by another process
    for (const auto& req : *reqqs) {
      if (req && (req->flags() & PREPARED))
        req->start();
    }

    MPI_Request* treqs = &(*reqqs)[0];
    Request::waitall(size, treqs, MPI_STATUSES_IGNORE);
    reqqs->clear();
  }
  xbt_mutex_release(mut_);

  Group::unref(group_);
  opened_--; //we're opened for business !
  return MPI_SUCCESS;
}

Win* Win::f2c(int id){
  return static_cast<Win*>(F2C::f2c(id));
}

}
}
