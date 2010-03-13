#include "private.h"
#include "xbt/time.h"

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

void smpi_process_init(int* argc, char*** argv) {
  int index;
  smpi_process_data_t data;
  smx_process_t proc;

  proc = SIMIX_process_self();
  index = atoi((*argv)[1]);
  data = smpi_process_remote_data(index);
  SIMIX_process_set_data(proc, data);
  if (*argc > 2) {
    free((*argv)[1]);
    memmove(&(*argv)[1], &(*argv)[2], sizeof(char *) * (*argc - 2));
    (*argv)[(*argc) - 1] = NULL;
  }
  (*argc)--;
  DEBUG2("<%d> New process in the game: %p", index, proc);
}

void smpi_process_destroy(void) {
  int index = smpi_process_index();

  DEBUG1("<%d> Process left the game", index);
}

/* MPI Low level calls */
MPI_Request smpi_mpi_isend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  MPI_Request request;

  request = xbt_new(s_smpi_mpi_request_t, 1);
  request->comm = comm;
  request->src = smpi_comm_rank(comm);
  request->dst = dst;
  request->tag = tag;
  request->size = smpi_datatype_size(datatype) * count;
  request->complete = 0;
  smpi_process_post_send(comm, request);
  request->pair = SIMIX_network_isend(request->rdv, request->size, -1.0, buf, request->size, request);
  return request;
}

MPI_Request smpi_mpi_irecv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm) {
  MPI_Request request;

  request = xbt_new(s_smpi_mpi_request_t, 1);
  request->comm = comm;
  request->src = src;
  request->dst = smpi_comm_rank(comm);
  request->tag = tag;
  request->size = smpi_datatype_size(datatype) * count;
  request->complete = 0;
  smpi_process_post_recv(request);
  request->pair = SIMIX_network_irecv(request->rdv, buf, &request->size);
  return request;
}

void smpi_mpi_recv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status) {
  MPI_Request request;

  request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
  smpi_mpi_wait(&request, status);
}

void smpi_mpi_send(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm) {
  MPI_Request request;

  request = smpi_mpi_isend(buf, count, datatype, src, tag, comm);
  smpi_mpi_wait(&request, MPI_STATUS_IGNORE);
}

void smpi_mpi_sendrecv(void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status) {
  MPI_Request requests[2];
  MPI_Status stats[2];

  requests[0] = smpi_mpi_isend(sendbuf, sendcount, sendtype, dst, sendtag, comm);
  requests[1] = smpi_mpi_irecv(recvbuf, recvcount, recvtype, src, recvtag, comm);
  smpi_mpi_waitall(2, requests, stats);
  if(status != MPI_STATUS_IGNORE) {
    // Copy receive status
    memcpy(status, &stats[1], sizeof(MPI_Status));
  }
}

static void finish_wait(MPI_Request* request, MPI_Status* status) {
  MPI_Request data = (MPI_Request)SIMIX_communication_get_data((*request)->pair);

  xbt_assert0(data != MPI_REQUEST_NULL, "Erroneous situation");
  if(status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = (*request)->src;
    status->MPI_TAG = (*request)->tag;
    status->MPI_ERROR = MPI_SUCCESS;
  }
  DEBUG3("finishing wait for %p [data = %p, complete = %d]", *request, data, data->complete);
  // data == *request if sender is first to finish its wait
  // data != *request if receiver is first to finish its wait
  if(data->complete == 0) {
    // first arrives here
    data->complete = 1;
    if(data != *request) {
      // receveiver cleans its part
      xbt_free(*request);
    }
  } else {
    // second arrives here
    if(data != *request) {
      // receiver cleans everything
      xbt_free(data);
    }
    xbt_free(*request);
  }
  *request = MPI_REQUEST_NULL;
}

int smpi_mpi_test(MPI_Request* request, MPI_Status* status) {
  MPI_Request data = (MPI_Request)SIMIX_communication_get_data((*request)->pair);
  int flag = data && data->complete == 1;

  if(flag) {
    SIMIX_communication_destroy((*request)->pair);
    finish_wait(request, status);
  }
  return flag;
}

int smpi_mpi_testany(int count, MPI_Request requests[], int* index, MPI_Status* status) {
  MPI_Request data;
  int i, flag;

  *index = MPI_UNDEFINED;
  flag = 0;
  for(i = 0; i < count; i++) {
    if(requests[i] != MPI_REQUEST_NULL) {
      data = (MPI_Request)SIMIX_communication_get_data(requests[i]->pair);
      if(data != MPI_REQUEST_NULL && data->complete == 1) {
        SIMIX_communication_destroy(requests[i]->pair);
        finish_wait(&requests[i], status);
        *index = i;
        flag = 1;
        break;
      }
    }
  }
  return flag;
}

void smpi_mpi_wait(MPI_Request* request, MPI_Status* status) {
  MPI_Request data = (MPI_Request)SIMIX_communication_get_data((*request)->pair);

  DEBUG6("wait for request %p (%p: %p) [src = %d, dst = %d, tag = %d]",
         *request, (*request)->pair, data, (*request)->src, (*request)->dst, (*request)->tag);
  // data is null if receiver waits before sender enters the rdv
  if(data == MPI_REQUEST_NULL || data->complete == 0) {
    SIMIX_network_wait((*request)->pair, -1.0);
  } else {
    SIMIX_communication_destroy((*request)->pair);
  }
  finish_wait(request, status);
}

int smpi_mpi_waitany(int count, MPI_Request requests[], MPI_Status* status) {
  xbt_dynar_t comms;
  MPI_Request data;
  int i, size, index;
  int* map;

  index = MPI_UNDEFINED;
  if(count > 0) {
    // First check for already completed requests
    for(i = 0; i < count; i++) {
      if(requests[i] != MPI_REQUEST_NULL) {
        data = (MPI_Request)SIMIX_communication_get_data(requests[i]->pair);
        if(data != MPI_REQUEST_NULL && data->complete == 1) {
          index = i;
          SIMIX_communication_destroy(requests[index]->pair); // always succeeds (but cleans the simix layer)
          break;
        }
      }
    }
    if(index == MPI_UNDEFINED) {
      // Otherwise, wait for a request to complete
      comms = xbt_dynar_new(sizeof(smx_comm_t), NULL);
      map = xbt_new(int, count);
      size = 0;
      DEBUG0("Wait for one of");
      for(i = 0; i < count; i++) {
        if(requests[i] != MPI_REQUEST_NULL && requests[i]->complete == 0) {
         DEBUG4("   request %p [src = %d, dst = %d, tag = %d]",
                requests[i], requests[i]->src, requests[i]->dst, requests[i]->tag);
          xbt_dynar_push(comms, &requests[i]->pair);
          map[size] = i;
          size++;
        }
      }
      if(size > 0) {
        index = SIMIX_network_waitany(comms);
        index = map[index];
      }
      xbt_free(map);
      xbt_dynar_free_container(&comms);
    }
    if(index != MPI_UNDEFINED) {
      finish_wait(&requests[index], status);
    }
  }
  return index;
}

void smpi_mpi_waitall(int count, MPI_Request requests[],  MPI_Status status[]) {
  int index;
  MPI_Status stat;

  while(count > 0) {
    index = smpi_mpi_waitany(count, requests, &stat);
    if(index == MPI_UNDEFINED) {
      break;
    }
    if(status != MPI_STATUS_IGNORE) {
      memcpy(&status[index], &stat, sizeof(stat));
    }
    // Move the last request to the found position
    requests[index] = requests[count - 1];
    requests[count - 1] = MPI_REQUEST_NULL;
    count--;
  }
}

int smpi_mpi_waitsome(int incount, MPI_Request requests[], int* indices, MPI_Status status[]) {
  MPI_Request data;
  int i, count;

  count = 0;
  for(i = 0; i < incount; i++) {
    if(requests[i] != MPI_REQUEST_NULL) {
      data = (MPI_Request)SIMIX_communication_get_data(requests[i]->pair);
      if(data != MPI_REQUEST_NULL && data->complete == 1) {
        SIMIX_communication_destroy(requests[i]->pair);
        finish_wait(&requests[i], status != MPI_STATUS_IGNORE ? &status[i] : MPI_STATUS_IGNORE);
        indices[count] = i;
        count++;
      }
    }
  }
  return count;
}

void smpi_mpi_bcast(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
  nary_tree_bcast(buf, count, datatype, root, comm, 4);
}

void smpi_mpi_barrier(MPI_Comm comm) {
  // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
  nary_tree_barrier(comm, 4);
}

void smpi_mpi_gather(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, src, index, sendsize, recvsize;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    recvsize = smpi_datatype_size(recvtype);
    // Local copy from root
    memcpy(&((char*)recvbuf)[root * recvcount * recvsize], sendbuf, sendcount * sendsize * sizeof(char));
    // Receive buffers from senders
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = smpi_mpi_irecv(&((char*)recvbuf)[src * recvcount * recvsize], recvcount, recvtype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_gatherv(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int* recvcounts, int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, src, index, sendsize;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    // Local copy from root
    memcpy(&((char*)recvbuf)[displs[root]], sendbuf, sendcount * sendsize * sizeof(char));
    // Receive buffers from senders
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = smpi_mpi_irecv(&((char*)recvbuf)[displs[src]], recvcounts[src], recvtype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_allgather(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, other, index, sendsize, recvsize;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  // Local copy from self
  memcpy(&((char*)recvbuf)[rank * recvcount * recvsize], sendbuf, sendcount * sendsize * sizeof(char));
  // Send/Recv buffers to/from others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] = smpi_mpi_isend(sendbuf, sendcount, sendtype, other, system_tag, comm);
      index++;
      requests[index] = smpi_mpi_irecv(&((char*)recvbuf)[other * recvcount * recvsize], recvcount, recvtype, other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

void smpi_mpi_allgatherv(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int* recvcounts, int* displs, MPI_Datatype recvtype, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, other, index, sendsize, recvsize;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  // Local copy from self
  memcpy(&((char*)recvbuf)[displs[rank]], sendbuf, sendcount * sendsize * sizeof(char));
  // Send buffers to others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] = smpi_mpi_isend(sendbuf, sendcount, sendtype, other, system_tag, comm);
      index++;
      requests[index] = smpi_mpi_irecv(&((char*)recvbuf)[displs[other]], recvcounts[other], recvtype, other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

void smpi_mpi_scatter(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, dst, index, sendsize, recvsize;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    recvsize = smpi_datatype_size(recvtype);
    // Local copy from root
    memcpy(recvbuf, &((char*)sendbuf)[root * sendcount * sendsize], recvcount * recvsize * sizeof(char));
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] = smpi_mpi_isend(&((char*)sendbuf)[dst * sendcount * sendsize], sendcount, sendtype, dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_scatterv(void* sendbuf, int* sendcounts, int* displs, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, dst, index, sendsize, recvsize;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm, MPI_STATUS_IGNORE);
  } else {
    sendsize = smpi_datatype_size(sendtype);
    recvsize = smpi_datatype_size(recvtype);
    // Local copy from root
    memcpy(recvbuf, &((char*)sendbuf)[displs[root]], recvcount * recvsize * sizeof(char));
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] = smpi_mpi_isend(&((char*)sendbuf)[displs[dst]], sendcounts[dst], sendtype, dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_reduce(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  int system_tag = 666;
  int rank, size, src, index, datasize;
  MPI_Request* requests;
  void** tmpbufs;
 
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
    tmpbufs = xbt_new(void*, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        tmpbufs[index] = xbt_malloc(count * datasize);
        requests[index] = smpi_mpi_irecv(tmpbufs[index], count, datatype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
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

void smpi_mpi_allreduce(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
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
