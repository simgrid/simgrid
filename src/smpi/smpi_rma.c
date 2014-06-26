
/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_rma, smpi, "Logging specific to SMPI (RMA operations)");

#define RMA_TAG -1234

xbt_bar_t creation_bar = NULL;

typedef struct s_smpi_mpi_win{
  void* base;
  MPI_Aint size;
  int disp_unit;
  MPI_Comm comm;
  //MPI_Info info
  int assert;
  xbt_dynar_t requests;
  xbt_bar_t bar;
  MPI_Win* connected_wins;
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
  //win->info = info;
  win->comm = comm;
  win->requests = xbt_dynar_new(sizeof(MPI_Request), NULL);
  win->connected_wins = xbt_malloc0(comm_size*sizeof(MPI_Win));
  win->connected_wins[rank] = win;
  
  if(rank==0){
    win->bar=xbt_barrier_init(comm_size);
  }
  
  mpi_coll_allgather_fun(&(win->connected_wins[rank]),
                     sizeof(MPI_Win),
                     MPI_BYTE,
                     win->connected_wins,
                     sizeof(MPI_Win),
                     MPI_BYTE,
                     comm);
                     
  mpi_coll_bcast_fun( &(win->bar),
                     sizeof(xbt_bar_t),
                     MPI_BYTE,
                     0,
                     comm);
                     
  mpi_coll_barrier_fun(comm);
  
  return win;
}

int smpi_mpi_win_free( MPI_Win* win){

  //As per the standard, perform a barrier to ensure every async comm is finished
  xbt_barrier_wait((*win)->bar);
  xbt_dynar_free(&(*win)->requests);
  xbt_free((*win)->connected_wins);
  xbt_free(*win);
  win = MPI_WIN_NULL;
  return MPI_SUCCESS;
}


int smpi_mpi_win_fence( int assert,  MPI_Win win){

  XBT_DEBUG("Entering fence");

  if(assert != MPI_MODE_NOPRECEDE){
    xbt_barrier_wait(win->bar);

    xbt_dynar_t reqs = win->requests;
    int size = xbt_dynar_length(reqs);
    unsigned int cpt=0;
    MPI_Request req;
    // start all requests that have been prepared by another process
    xbt_dynar_foreach(reqs, cpt, req){
      if (req->flags & PREPARED) smpi_mpi_start(req);
    }

    MPI_Request* treqs = xbt_dynar_to_array(reqs);
    smpi_mpi_waitall(size,treqs,MPI_STATUSES_IGNORE);
    xbt_free(treqs);
    win->requests=xbt_dynar_new(sizeof(MPI_Request), NULL);

  }
  win->assert = assert;
  
  xbt_barrier_wait(win->bar);
  XBT_DEBUG("Leaving fence ");

  return MPI_SUCCESS;
}

int smpi_mpi_put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
  //get receiver pointer
  MPI_Win recv_win = win->connected_wins[target_rank];

  void* recv_addr = (void*) ( ((char*)recv_win->base) + target_disp * recv_win->disp_unit);
  smpi_datatype_use(origin_datatype);
  smpi_datatype_use(target_datatype);
  XBT_DEBUG("Entering MPI_Put to %d", target_rank);

  if(target_rank != smpi_comm_rank(win->comm)){
    //prepare send_request
    MPI_Request sreq = smpi_rma_send_init(origin_addr, origin_count, origin_datatype,
        smpi_process_index(), smpi_group_index(smpi_comm_group(win->comm),target_rank), RMA_TAG+1, win->comm, MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = smpi_rma_recv_init(recv_addr, target_count, target_datatype,
        smpi_process_index(), smpi_group_index(smpi_comm_group(win->comm),target_rank), RMA_TAG+1, recv_win->comm, MPI_OP_NULL);

    //push request to receiver's win
    xbt_dynar_push_as(recv_win->requests, MPI_Request, rreq);

    //start send
    smpi_mpi_start(sreq);

    //push request to sender's win
    xbt_dynar_push_as(win->requests, MPI_Request, sreq);
  }else{
    smpi_datatype_copy(origin_addr, origin_count, origin_datatype,
                       recv_addr, target_count, target_datatype);
  }

  return MPI_SUCCESS;
}

int smpi_mpi_get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
  //get sender pointer
  MPI_Win send_win = win->connected_wins[target_rank];

  void* send_addr = (void*)( ((char*)send_win->base) + target_disp * send_win->disp_unit);
  smpi_datatype_use(origin_datatype);
  smpi_datatype_use(target_datatype);
  XBT_DEBUG("Entering MPI_Get from %d", target_rank);

  if(target_rank != smpi_comm_rank(win->comm)){
    //prepare send_request
    MPI_Request sreq = smpi_rma_send_init(send_addr, target_count, target_datatype,
        smpi_group_index(smpi_comm_group(win->comm),target_rank), smpi_process_index(), RMA_TAG+2, send_win->comm, MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = smpi_rma_recv_init(origin_addr, origin_count, origin_datatype,
        smpi_group_index(smpi_comm_group(win->comm),target_rank), smpi_process_index(), RMA_TAG+2, win->comm, MPI_OP_NULL);
        
    //start the send, with another process than us as sender. 
    smpi_mpi_start(sreq);
    
    //push request to receiver's win
    xbt_dynar_push_as(send_win->requests, MPI_Request, sreq);

    //start recv
    smpi_mpi_start(rreq);

    //push request to sender's win
    xbt_dynar_push_as(win->requests, MPI_Request, rreq);
  }else{
    smpi_datatype_copy(send_addr, target_count, target_datatype,
                       origin_addr, origin_count, origin_datatype);
  }

  return MPI_SUCCESS;
}


int smpi_mpi_accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  //FIXME: local version 
  //get receiver pointer
  MPI_Win recv_win = win->connected_wins[target_rank];

  void* recv_addr = (void*)( ((char*)recv_win->base) + target_disp * recv_win->disp_unit);
  XBT_DEBUG("Entering MPI_Accumulate to %d", target_rank);

  smpi_datatype_use(origin_datatype);
  smpi_datatype_use(target_datatype);


    //prepare send_request
    MPI_Request sreq = smpi_rma_send_init(origin_addr, origin_count, origin_datatype,
        smpi_process_index(), smpi_group_index(smpi_comm_group(win->comm),target_rank), RMA_TAG+3, win->comm, op);

    //prepare receiver request
    MPI_Request rreq = smpi_rma_recv_init(recv_addr, target_count, target_datatype,
        smpi_process_index(), smpi_group_index(smpi_comm_group(win->comm),target_rank), RMA_TAG+3, recv_win->comm, op);
    //push request to receiver's win
    xbt_dynar_push_as(recv_win->requests, MPI_Request, rreq);
    //start send
    smpi_mpi_start(sreq);
    
    //push request to sender's win
    xbt_dynar_push_as(win->requests, MPI_Request, sreq);
  


  return MPI_SUCCESS;
}

