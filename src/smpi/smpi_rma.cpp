/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_rma, smpi, "Logging specific to SMPI (RMA operations)");

msg_bar_t creation_bar = nullptr;

typedef struct s_smpi_mpi_win{
  void* base;
  MPI_Aint size;
  int disp_unit;
  MPI_Comm comm;
  MPI_Info info;
  int assert;
  std::vector<MPI_Request> *requests;
  xbt_mutex_t mut;
  msg_bar_t bar;
  MPI_Win* connected_wins;
  char* name;
  int opened;
  MPI_Group group;
  int count; //for ordering the accs
} s_smpi_mpi_win_t;


MPI_Win smpi_mpi_win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm){
  MPI_Win win;

  int comm_size = smpi_comm_size(comm);
  int rank=smpi_comm_rank(comm);
  XBT_DEBUG("Creating window");

  win = xbt_new(s_smpi_mpi_win_t, 1);
  win->base = base;
  win->size = size;
  win->disp_unit = disp_unit;
  win->assert = 0;
  win->info = info;
  if(info!=MPI_INFO_NULL)
    info->refcount++;
  win->comm = comm;
  win->name = nullptr;
  win->opened = 0;
  win->group = MPI_GROUP_NULL;
  win->requests = new std::vector<MPI_Request>();
  win->mut=xbt_mutex_init();
  win->connected_wins = xbt_new0(MPI_Win, comm_size);
  win->connected_wins[rank] = win;
  win->count = 0;
  if(rank==0){
    win->bar = MSG_barrier_init(comm_size);
  }
  mpi_coll_allgather_fun(&(win->connected_wins[rank]), sizeof(MPI_Win), MPI_BYTE, win->connected_wins, sizeof(MPI_Win),
                         MPI_BYTE, comm);

  mpi_coll_bcast_fun(&(win->bar), sizeof(msg_bar_t), MPI_BYTE, 0, comm);

  mpi_coll_barrier_fun(comm);

  return win;
}

int smpi_mpi_win_free( MPI_Win* win){
  //As per the standard, perform a barrier to ensure every async comm is finished
  MSG_barrier_wait((*win)->bar);
  xbt_mutex_acquire((*win)->mut);
  delete (*win)->requests;
  xbt_mutex_release((*win)->mut);
  xbt_free((*win)->connected_wins);
  if ((*win)->name != nullptr){
    xbt_free((*win)->name);
  }
  if((*win)->info!=MPI_INFO_NULL){
    MPI_Info_free(&(*win)->info);
  }

  mpi_coll_barrier_fun((*win)->comm);
  int rank=smpi_comm_rank((*win)->comm);
  if(rank == 0)
    MSG_barrier_destroy((*win)->bar);
  xbt_mutex_destroy((*win)->mut);
  xbt_free(*win);
  *win = MPI_WIN_NULL;
  return MPI_SUCCESS;
}

void smpi_mpi_win_get_name(MPI_Win win, char* name, int* length){
  if(win->name==nullptr){
    *length=0;
    name=nullptr;
    return;
  }
  *length = strlen(win->name);
  strncpy(name, win->name, *length+1);
}

void smpi_mpi_win_get_group(MPI_Win win, MPI_Group* group){
  if(win->comm != MPI_COMM_NULL){
    *group = smpi_comm_group(win->comm);
  } else {
    *group = MPI_GROUP_NULL;
  }
}

void smpi_mpi_win_set_name(MPI_Win win, char* name){
  win->name = xbt_strdup(name);
}

int smpi_mpi_win_fence( int assert,  MPI_Win win){
  XBT_DEBUG("Entering fence");
  if(win->opened==0)
    win->opened=1;
  if(assert != MPI_MODE_NOPRECEDE){
    MSG_barrier_wait(win->bar);
    xbt_mutex_acquire(win->mut);
    std::vector<MPI_Request> *reqs = win->requests;
    int size = static_cast<int>(reqs->size());
    // start all requests that have been prepared by another process
    if(size>0){
        for(auto req: *reqs){
          if (req && (req->flags & PREPARED))
            smpi_mpi_start(req);
        }

        MPI_Request* treqs = &(*reqs)[0];

        smpi_mpi_waitall(size,treqs,MPI_STATUSES_IGNORE);
    }
    win->count=0;
    xbt_mutex_release(win->mut);
  }
  win->assert = assert;

  MSG_barrier_wait(win->bar);
  XBT_DEBUG("Leaving fence ");

  return MPI_SUCCESS;
}

int smpi_mpi_put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
  if(win->opened==0)//check that post/start has been done
    return MPI_ERR_WIN;
  //get receiver pointer
  MPI_Win recv_win = win->connected_wins[target_rank];

  void* recv_addr = static_cast<void*> ( static_cast<char*>(recv_win->base) + target_disp * recv_win->disp_unit);
  XBT_DEBUG("Entering MPI_Put to %d", target_rank);

  if(target_rank != smpi_comm_rank(win->comm)){
    //prepare send_request
    MPI_Request sreq = smpi_rma_send_init(origin_addr, origin_count, origin_datatype, smpi_process_index(),
        smpi_group_index(smpi_comm_group(win->comm),target_rank), SMPI_RMA_TAG+1, win->comm, MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = smpi_rma_recv_init(recv_addr, target_count, target_datatype, smpi_process_index(),
        smpi_group_index(smpi_comm_group(win->comm),target_rank), SMPI_RMA_TAG+1, recv_win->comm, MPI_OP_NULL);

    //push request to receiver's win
    xbt_mutex_acquire(recv_win->mut);
    recv_win->requests->push_back(rreq);
    xbt_mutex_release(recv_win->mut);
    //start send
    smpi_mpi_start(sreq);

    //push request to sender's win
    xbt_mutex_acquire(win->mut);
    win->requests->push_back(sreq);
    xbt_mutex_release(win->mut);
  }else{
    smpi_datatype_copy(origin_addr, origin_count, origin_datatype, recv_addr, target_count, target_datatype);
  }

  return MPI_SUCCESS;
}

int smpi_mpi_get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
  if(win->opened==0)//check that post/start has been done
    return MPI_ERR_WIN;
  //get sender pointer
  MPI_Win send_win = win->connected_wins[target_rank];

  void* send_addr = static_cast<void*>(static_cast<char*>(send_win->base) + target_disp * send_win->disp_unit);
  XBT_DEBUG("Entering MPI_Get from %d", target_rank);

  if(target_rank != smpi_comm_rank(win->comm)){
    //prepare send_request
    MPI_Request sreq = smpi_rma_send_init(send_addr, target_count, target_datatype,
        smpi_group_index(smpi_comm_group(win->comm),target_rank), smpi_process_index(), SMPI_RMA_TAG+2, send_win->comm,
        MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = smpi_rma_recv_init(origin_addr, origin_count, origin_datatype,
        smpi_group_index(smpi_comm_group(win->comm),target_rank), smpi_process_index(), SMPI_RMA_TAG+2, win->comm,
        MPI_OP_NULL);

    //start the send, with another process than us as sender. 
    smpi_mpi_start(sreq);
    //push request to receiver's win
    xbt_mutex_acquire(send_win->mut);
    send_win->requests->push_back(sreq);
    xbt_mutex_release(send_win->mut);

    //start recv
    smpi_mpi_start(rreq);
    //push request to sender's win
    xbt_mutex_acquire(win->mut);
    win->requests->push_back(rreq);
    xbt_mutex_release(win->mut);
  }else{
    smpi_datatype_copy(send_addr, target_count, target_datatype, origin_addr, origin_count, origin_datatype);
  }

  return MPI_SUCCESS;
}


int smpi_mpi_accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  if(win->opened==0)//check that post/start has been done
    return MPI_ERR_WIN;
  //FIXME: local version 
  //get receiver pointer
  MPI_Win recv_win = win->connected_wins[target_rank];

  void* recv_addr = static_cast<void*>(static_cast<char*>(recv_win->base) + target_disp * recv_win->disp_unit);
  XBT_DEBUG("Entering MPI_Accumulate to %d", target_rank);
    //As the tag will be used for ordering of the operations, add count to it
    //prepare send_request
    MPI_Request sreq = smpi_rma_send_init(origin_addr, origin_count, origin_datatype,
        smpi_process_index(), smpi_group_index(smpi_comm_group(win->comm),target_rank), SMPI_RMA_TAG+3+win->count, win->comm, op);

    //prepare receiver request
    MPI_Request rreq = smpi_rma_recv_init(recv_addr, target_count, target_datatype,
        smpi_process_index(), smpi_group_index(smpi_comm_group(win->comm),target_rank), SMPI_RMA_TAG+3+win->count, recv_win->comm, op);

    win->count++;
    //push request to receiver's win
    xbt_mutex_acquire(recv_win->mut);
    recv_win->requests->push_back(rreq);
    xbt_mutex_release(recv_win->mut);
    //start send
    smpi_mpi_start(sreq);

    //push request to sender's win
    xbt_mutex_acquire(win->mut);
    win->requests->push_back(sreq);
    xbt_mutex_release(win->mut);

  return MPI_SUCCESS;
}

int smpi_mpi_win_start(MPI_Group group, int assert, MPI_Win win){
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
  int i=0,j=0;
  int size = smpi_group_size(group);
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int src=smpi_group_index(group,j);
    if(src!=smpi_process_index()&& src!=MPI_UNDEFINED){
      reqs[i]=smpi_irecv_init(nullptr, 0, MPI_CHAR, src,SMPI_RMA_TAG+4, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  smpi_mpi_startall(size, reqs);
  smpi_mpi_waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    smpi_mpi_request_free(&reqs[i]);
  }
  xbt_free(reqs);
  win->opened++; //we're open for business !
  win->group=group;
  smpi_group_use(group);
  return MPI_SUCCESS;
}

int smpi_mpi_win_post(MPI_Group group, int assert, MPI_Win win){
  //let's make a synchronous send here
  int i=0,j=0;
  int size = smpi_group_size(group);
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int dst=smpi_group_index(group,j);
    if(dst!=smpi_process_index() && dst!=MPI_UNDEFINED){
      reqs[i]=smpi_mpi_send_init(nullptr, 0, MPI_CHAR, dst, SMPI_RMA_TAG+4, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;

  smpi_mpi_startall(size, reqs);
  smpi_mpi_waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    smpi_mpi_request_free(&reqs[i]);
  }
  xbt_free(reqs);
  win->opened++; //we're open for business !
  win->group=group;
  smpi_group_use(group);
  return MPI_SUCCESS;
}

int smpi_mpi_win_complete(MPI_Win win){
  if(win->opened==0)
    xbt_die("Complete called on already opened MPI_Win");

  XBT_DEBUG("Entering MPI_Win_Complete");
  int i=0,j=0;
  int size = smpi_group_size(win->group);
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int dst=smpi_group_index(win->group,j);
    if(dst!=smpi_process_index() && dst!=MPI_UNDEFINED){
      reqs[i]=smpi_mpi_send_init(nullptr, 0, MPI_CHAR, dst, SMPI_RMA_TAG+5, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  XBT_DEBUG("Win_complete - Sending sync messages to %d processes", size);
  smpi_mpi_startall(size, reqs);
  smpi_mpi_waitall(size, reqs, MPI_STATUSES_IGNORE);

  for(i=0;i<size;i++){
    smpi_mpi_request_free(&reqs[i]);
  }
  xbt_free(reqs);

  //now we can finish RMA calls
  xbt_mutex_acquire(win->mut);
  std::vector<MPI_Request> *reqqs = win->requests;
  size = static_cast<int>(reqqs->size());

  XBT_DEBUG("Win_complete - Finishing %d RMA calls", size);
  // start all requests that have been prepared by another process
  for (auto req: *reqqs){
    if (req && (req->flags & PREPARED))
      smpi_mpi_start(req);
  }

  MPI_Request* treqs = &(*reqqs)[0];
  smpi_mpi_waitall(size,treqs,MPI_STATUSES_IGNORE);
  reqqs->clear();
  xbt_mutex_release(win->mut);

  smpi_group_unuse(win->group);
  win->opened--; //we're closed for business !
  return MPI_SUCCESS;
}

int smpi_mpi_win_wait(MPI_Win win){
  //naive, blocking implementation.
  XBT_DEBUG("Entering MPI_Win_Wait");
  int i=0,j=0;
  int size = smpi_group_size(win->group);
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int src=smpi_group_index(win->group,j);
    if(src!=smpi_process_index() && src!=MPI_UNDEFINED){
      reqs[i]=smpi_irecv_init(nullptr, 0, MPI_CHAR, src,SMPI_RMA_TAG+5, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  XBT_DEBUG("Win_wait - Receiving sync messages from %d processes", size);
  smpi_mpi_startall(size, reqs);
  smpi_mpi_waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    smpi_mpi_request_free(&reqs[i]);
  }
  xbt_free(reqs);
  xbt_mutex_acquire(win->mut);
  std::vector<MPI_Request> *reqqs = win->requests;
  size = static_cast<int>(reqqs->size());

  XBT_DEBUG("Win_wait - Finishing %d RMA calls", size);

  // start all requests that have been prepared by another process
  for(auto req: *reqqs){
    if (req && (req->flags & PREPARED))
      smpi_mpi_start(req);
  }

  MPI_Request* treqs = &(*reqqs)[0];
  smpi_mpi_waitall(size,treqs,MPI_STATUSES_IGNORE);
  reqqs->clear();
  xbt_mutex_release(win->mut);

  smpi_group_unuse(win->group);
  win->opened--; //we're opened for business !
  return MPI_SUCCESS;
}
