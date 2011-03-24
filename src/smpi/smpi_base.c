/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/time.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_base, smpi,
                                "Logging specific to SMPI (base)");
XBT_LOG_EXTERNAL_CATEGORY(smpi_base);
XBT_LOG_EXTERNAL_CATEGORY(smpi_bench);
XBT_LOG_EXTERNAL_CATEGORY(smpi_kernel);
XBT_LOG_EXTERNAL_CATEGORY(smpi_mpi);
XBT_LOG_EXTERNAL_CATEGORY(smpi_mpi_dt);
XBT_LOG_EXTERNAL_CATEGORY(smpi_coll);
XBT_LOG_EXTERNAL_CATEGORY(smpi_receiver);
XBT_LOG_EXTERNAL_CATEGORY(smpi_sender);
XBT_LOG_EXTERNAL_CATEGORY(smpi_util);

static int match_recv(void* a, void* b) {
   MPI_Request ref = (MPI_Request)a;
   MPI_Request req = (MPI_Request)b;

   xbt_assert(ref, "Cannot match recv against null reference");
   xbt_assert(req, "Cannot match recv against null request");
   return req->comm == ref->comm
          && (ref->src == MPI_ANY_SOURCE || req->src == ref->src)
          && (ref->tag == MPI_ANY_TAG || req->tag == ref->tag);
}

static int match_send(void* a, void* b) {
   MPI_Request ref = (MPI_Request)a;
   MPI_Request req = (MPI_Request)b;

   xbt_assert(ref, "Cannot match send against null reference");
   xbt_assert(req, "Cannot match send against null request");
   return req->comm == ref->comm
          && (req->src == MPI_ANY_SOURCE || req->src == ref->src)
          && (req->tag == MPI_ANY_TAG || req->tag == ref->tag);
}

static MPI_Request build_request(void *buf, int count,
                                 MPI_Datatype datatype, int src, int dst,
                                 int tag, MPI_Comm comm, unsigned flags)
{
  MPI_Request request;

  request = xbt_new(s_smpi_mpi_request_t, 1);
  request->buf = buf;
  request->size = smpi_datatype_size(datatype) * count;
  request->src = src;
  request->dst = dst;
  request->tag = tag;
  request->comm = comm;
  request->action = NULL;
  request->flags = flags;
#ifdef HAVE_TRACING
  request->send = 0;
  request->recv = 0;
#endif
  return request;
}

/* MPI Low level calls */
MPI_Request smpi_mpi_send_init(void *buf, int count, MPI_Datatype datatype,
                               int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                    comm, PERSISTENT | SEND);

  return request;
}

MPI_Request smpi_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                               int src, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, src, smpi_comm_rank(comm), tag,
                    comm, PERSISTENT | RECV);

  return request;
}

void smpi_mpi_start(MPI_Request request)
{
  smx_rdv_t mailbox;

  xbt_assert(!request->action,
              "Cannot (re)start a non-finished communication");
  if(request->flags & RECV) {
    print_request("New recv", request);
    mailbox = smpi_process_mailbox();
    request->action = SIMIX_req_comm_irecv(mailbox, request->buf, &request->size, &match_recv, request);
  } else {
    print_request("New send", request);
    mailbox = smpi_process_remote_mailbox(request->dst);
    request->action = SIMIX_req_comm_isend(mailbox, request->size, -1.0,
                                           request->buf, request->size, &match_send, request, 0);
#ifdef HAVE_TRACING
    SIMIX_req_set_category (request->action, TRACE_internal_smpi_get_category());
#endif
  }
}

void smpi_mpi_startall(int count, MPI_Request * requests)
{
  int i;

  for(i = 0; i < count; i++) {
    smpi_mpi_start(requests[i]);
  }
}

void smpi_mpi_request_free(MPI_Request * request)
{
  xbt_free(*request);
  *request = MPI_REQUEST_NULL;
}

MPI_Request smpi_isend_init(void *buf, int count, MPI_Datatype datatype,
                            int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                    comm, NON_PERSISTENT | SEND);

  return request;
}

MPI_Request smpi_mpi_isend(void *buf, int count, MPI_Datatype datatype,
                           int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
      smpi_isend_init(buf, count, datatype, dst, tag, comm);

  smpi_mpi_start(request);
  return request;
}

MPI_Request smpi_irecv_init(void *buf, int count, MPI_Datatype datatype,
                            int src, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, src, smpi_comm_rank(comm), tag,
                    comm, NON_PERSISTENT | RECV);

  return request;
}

MPI_Request smpi_mpi_irecv(void *buf, int count, MPI_Datatype datatype,
                           int src, int tag, MPI_Comm comm)
{
  MPI_Request request =
      smpi_irecv_init(buf, count, datatype, src, tag, comm);

  smpi_mpi_start(request);
  return request;
}

void smpi_mpi_recv(void *buf, int count, MPI_Datatype datatype, int src,
                   int tag, MPI_Comm comm, MPI_Status * status)
{
  MPI_Request request;

  request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
  smpi_mpi_wait(&request, status);
}

void smpi_mpi_send(void *buf, int count, MPI_Datatype datatype, int dst,
                   int tag, MPI_Comm comm)
{
  MPI_Request request;

  request = smpi_mpi_isend(buf, count, datatype, dst, tag, comm);
  smpi_mpi_wait(&request, MPI_STATUS_IGNORE);
}

void smpi_mpi_sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       int dst, int sendtag, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int src, int recvtag,
                       MPI_Comm comm, MPI_Status * status)
{
  MPI_Request requests[2];
  MPI_Status stats[2];

  requests[0] =
      smpi_isend_init(sendbuf, sendcount, sendtype, dst, sendtag, comm);
  requests[1] =
      smpi_irecv_init(recvbuf, recvcount, recvtype, src, recvtag, comm);
  smpi_mpi_startall(2, requests);
  smpi_mpi_waitall(2, requests, stats);
  if(status != MPI_STATUS_IGNORE) {
    // Copy receive status
    memcpy(status, &stats[1], sizeof(MPI_Status));
  }
}

int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype)
{
  return status->count / smpi_datatype_size(datatype);
}

static void finish_wait(MPI_Request * request, MPI_Status * status)
{
  MPI_Request req = *request;

  if(status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = req->src;
    status->MPI_TAG = req->tag;
    status->MPI_ERROR = MPI_SUCCESS;
    status->count = req->size;
  }
  print_request("Finishing", req);
  if(req->flags & NON_PERSISTENT) {
    smpi_mpi_request_free(request);
  } else {
    req->action = NULL;
  }
}

int smpi_mpi_test(MPI_Request * request, MPI_Status * status)
{
   int flag = SIMIX_req_comm_test((*request)->action);

   if(flag) {
      smpi_mpi_wait(request, status);
   }
   return flag;
}

int smpi_mpi_testany(int count, MPI_Request requests[], int *index,
                     MPI_Status * status)
{
  xbt_dynar_t comms;
  int i, flag, size;
  int* map;

  *index = MPI_UNDEFINED;
  flag = 0;
  if(count > 0) {
    comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
    map = xbt_new(int, count);
    size = 0;
    for(i = 0; i < count; i++) {
      if(requests[i]->action) {
         xbt_dynar_push(comms, &requests[i]->action);
         map[size] = i;
         size++;
      }
    }
    if(size > 0) {
      *index = SIMIX_req_comm_testany(comms);
      *index = map[*index];
      if(*index != MPI_UNDEFINED) {
        smpi_mpi_wait(&requests[*index], status);
        flag = 1;
      }
    }
    xbt_free(map);
    xbt_dynar_free(&comms);
  }
  return flag;
}

void smpi_mpi_wait(MPI_Request * request, MPI_Status * status)
{
  print_request("Waiting", *request);
  SIMIX_req_comm_wait((*request)->action, -1.0);
  finish_wait(request, status);
}

int smpi_mpi_waitany(int count, MPI_Request requests[],
                     MPI_Status * status)
{
  xbt_dynar_t comms;
  int i, size, index;
  int *map;

  index = MPI_UNDEFINED;
  if(count > 0) {
    // Wait for a request to complete
    comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
    map = xbt_new(int, count);
    size = 0;
    XBT_DEBUG("Wait for one of");
    for(i = 0; i < count; i++) {
      if(requests[i] != MPI_REQUEST_NULL) {
        print_request("   ", requests[i]);
        xbt_dynar_push(comms, &requests[i]->action);
        map[size] = i;
        size++;
      }
    }
    if(size > 0) {
      index = SIMIX_req_comm_waitany(comms);
      index = map[index];
      finish_wait(&requests[index], status);
    }
    xbt_free(map);
    xbt_dynar_free(&comms);
  }
  return index;
}

void smpi_mpi_waitall(int count, MPI_Request requests[],
                      MPI_Status status[])
{
  int index, c;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUS_IGNORE ? MPI_STATUS_IGNORE : &stat;

  for(c = 0; c < count; c++) {
    if(MC_IS_ENABLED) {
      smpi_mpi_wait(&requests[c], pstat);
      index = c;
    } else {
      index = smpi_mpi_waitany(count, requests, pstat);
      if(index == MPI_UNDEFINED) {
        break;
      }
    }
    if(status != MPI_STATUS_IGNORE) {
      memcpy(&status[index], pstat, sizeof(*pstat));
    }
  }
}

int smpi_mpi_waitsome(int incount, MPI_Request requests[], int *indices,
                      MPI_Status status[])
{
  int i, count, index;

  count = 0;
  for(i = 0; i < incount; i++) {
     if(smpi_mpi_testany(incount, requests, &index, status)) {
       indices[count] = index;
       count++;
     }
  }
  return count;
}

void smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm)
{
  // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
  nary_tree_bcast(buf, count, datatype, root, comm, 4);
}

void smpi_mpi_barrier(MPI_Comm comm)
{
  // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
  nary_tree_barrier(comm, 4);
}

void smpi_mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, src, index, sendsize, recvsize;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    recvsize = smpi_datatype_size(recvtype);
    // Local copy from root
    memcpy(&((char *) recvbuf)[root * recvcount * recvsize], sendbuf,
           sendcount * sendsize * sizeof(char));
    // Receive buffers from senders
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = smpi_irecv_init(&((char *) recvbuf)
                                          [src * recvcount * recvsize],
                                          recvcount, recvtype, src,
                                          system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, src, index, sendsize;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    // Local copy from root
    memcpy(&((char *) recvbuf)[displs[root]], sendbuf,
           sendcount * sendsize * sizeof(char));
    // Receive buffers from senders
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        requests[index] =
            smpi_irecv_init(&((char *) recvbuf)[displs[src]],
                            recvcounts[src], recvtype, src, system_tag,
                            comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_allgather(void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype,
                        MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, other, index, sendsize, recvsize;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  // Local copy from self
  memcpy(&((char *) recvbuf)[rank * recvcount * recvsize], sendbuf,
         sendcount * sendsize * sizeof(char));
  // Send/Recv buffers to/from others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] =
          smpi_isend_init(sendbuf, sendcount, sendtype, other, system_tag,
                          comm);
      index++;
      requests[index] = smpi_irecv_init(&((char *) recvbuf)
                                        [other * recvcount * recvsize],
                                        recvcount, recvtype, other,
                                        system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(2 * (size - 1), requests);
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

void smpi_mpi_allgatherv(void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs,
                         MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, other, index, sendsize, recvsize;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  // Local copy from self
  memcpy(&((char *) recvbuf)[displs[rank]], sendbuf,
         sendcount * sendsize * sizeof(char));
  // Send buffers to others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] =
          smpi_isend_init(sendbuf, sendcount, sendtype, other, system_tag,
                          comm);
      index++;
      requests[index] =
          smpi_irecv_init(&((char *) recvbuf)[displs[other]],
                          recvcounts[other], recvtype, other, system_tag,
                          comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(2 * (size - 1), requests);
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

void smpi_mpi_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                      int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, dst, index, sendsize, recvsize;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm,
                  MPI_STATUS_IGNORE);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    recvsize = smpi_datatype_size(recvtype);
    // Local copy from root
    memcpy(recvbuf, &((char *) sendbuf)[root * sendcount * sendsize],
           recvcount * recvsize * sizeof(char));
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] = smpi_isend_init(&((char *) sendbuf)
                                          [dst * sendcount * sendsize],
                                          sendcount, sendtype, dst,
                                          system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_scatterv(void *sendbuf, int *sendcounts, int *displs,
                       MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, dst, index, sendsize, recvsize;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm,
                  MPI_STATUS_IGNORE);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    recvsize = smpi_datatype_size(recvtype);
    // Local copy from root
    memcpy(recvbuf, &((char *) sendbuf)[displs[root]],
           recvcount * recvsize * sizeof(char));
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] =
            smpi_isend_init(&((char *) sendbuf)[displs[dst]],
                            sendcounts[dst], sendtype, dst, system_tag,
                            comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, src, index, datasize;
  MPI_Request *requests;
  void **tmpbufs;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, count, datatype, root, system_tag, comm);
  } else {
    datasize = smpi_datatype_size(datatype);
    // Local copy from root
    memcpy(recvbuf, sendbuf, count * datasize * sizeof(char));
    // Receive buffers from senders
    //TODO: make a MPI_barrier here ?
    requests = xbt_new(MPI_Request, size - 1);
    tmpbufs = xbt_new(void *, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        tmpbufs[index] = xbt_malloc(count * datasize);
        requests[index] =
            smpi_irecv_init(tmpbufs[index], count, datatype, src,
                            system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    for(src = 0; src < size - 1; src++) {
      index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
      if(index == MPI_UNDEFINED) {
        break;
      }
      smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
    }
    for(index = 0; index < size - 1; index++) {
      xbt_free(tmpbufs[index]);
    }
    xbt_free(tmpbufs);
    xbt_free(requests);
  }
}

void smpi_mpi_allreduce(void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  smpi_mpi_bcast(recvbuf, count, datatype, 0, comm);

/*
FIXME: buggy implementation

  int system_tag = 666;
  int rank, size, other, index, datasize;
  MPI_Request* requests;
  void** tmpbufs;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  datasize = smpi_datatype_size(datatype);
  // Local copy from self
  memcpy(recvbuf, sendbuf, count * datasize * sizeof(char));
  // Send/Recv buffers to/from others;
  //TODO: make a MPI_barrier here ?
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  tmpbufs = xbt_new(void*, size - 1);
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      tmpbufs[index / 2] = xbt_malloc(count * datasize);
      requests[index] = smpi_mpi_isend(sendbuf, count, datatype, other, system_tag, comm);
      requests[index + 1] = smpi_mpi_irecv(tmpbufs[index / 2], count, datatype, other, system_tag, comm);
      index += 2;
    }
  }
  // Wait for completion of all comms.
  for(other = 0; other < 2 * (size - 1); other++) {
    index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
    if(index == MPI_UNDEFINED) {
      break;
    }
    if((index & 1) == 1) {
      // Request is odd: it's a irecv
      smpi_op_apply(op, tmpbufs[index / 2], recvbuf, &count, &datatype);
    }
  }
  for(index = 0; index < size - 1; index++) {
    xbt_free(tmpbufs[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
*/
}

void smpi_mpi_scan(void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, other, index, datasize;
  int total;
  MPI_Request *requests;
  void **tmpbufs;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  datasize = smpi_datatype_size(datatype);
  // Local copy from self
  memcpy(recvbuf, sendbuf, count * datasize * sizeof(char));
  // Send/Recv buffers to/from others;
  total = rank + (size - (rank + 1));
  requests = xbt_new(MPI_Request, total);
  tmpbufs = xbt_new(void *, rank);
  index = 0;
  for(other = 0; other < rank; other++) {
    tmpbufs[index] = xbt_malloc(count * datasize);
    requests[index] =
        smpi_irecv_init(tmpbufs[index], count, datatype, other, system_tag,
                        comm);
    index++;
  }
  for(other = rank + 1; other < size; other++) {
    requests[index] =
        smpi_isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(size - 1, requests);
  for(other = 0; other < total; other++) {
    index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
    if(index == MPI_UNDEFINED) {
      break;
    }
    if(index < rank) {
      // #Request is below rank: it's a irecv
      smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
    }
  }
  for(index = 0; index < size - 1; index++) {
    xbt_free(tmpbufs[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
}
