#include <stdio.h>
#include <sys/time.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"
#include "xbt/xbt_portability.h"
#include "smpi.h"

int MPI_Init(int *argc, char ***argv) {
  smpi_mpi_init();
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int MPI_Finalize() {
  smpi_bench_end();
  smpi_mpi_finalize();
  return MPI_SUCCESS;
}

// right now this just exits the current node, should send abort signal to all
// hosts in the communicator;
int MPI_Abort(MPI_Comm comm, int errorcode) {
  smpi_exit(errorcode);
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  int retval = MPI_SUCCESS;
  smpi_bench_end();
  if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else if (NULL == size) {
    retval = MPI_ERR_ARG;
  } else {
    *size = comm->size;
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  int retval = MPI_SUCCESS;
  smpi_bench_end();
  if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else if (NULL == rank) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_comm_rank(comm, SIMIX_host_self());
  }
  smpi_bench_begin();
  return retval;
}

/*
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) {
  int retval = MPI_SUCCESS;
  m_host_t host = SIMIX_host_self();
  int rank = smpi_comm_rank(comm, host);
  smpi_mpi_comm_split_table_node_t *split_table; 
  split_table = xbt_malloc(sizeof(smpi_mpi_comm_split_table_node_t) * comm->size);
  split_table[rank].color = color;
  split_table[rank].key   = key;
  split_table[rank].host  = host;
  smpi_mpi_alltoall(
  return retval;
}
*/

int MPI_Type_size(MPI_Datatype datatype, size_t *size) {
  int retval = MPI_SUCCESS;
  smpi_bench_end();
  if (NULL == datatype) {
    retval = MPI_ERR_TYPE;
  } else if (NULL == size) {
    retval = MPI_ERR_ARG;
  } else {
    *size = datatype->size;
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  int retval = MPI_SUCCESS;
  smpi_bench_end();
  if (NULL == request) {
    retval = MPI_ERR_REQUEST;
  } else if (NULL == status) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_wait(*request, status);
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Waitall(int count, MPI_Request *requests, MPI_Status *statuses) {
  int retval = MPI_SUCCESS;
  smpi_bench_end();
  if (NULL == requests) {
    retval = MPI_ERR_REQUEST;
  } else if (NULL == statuses) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_wait_all(count, requests, statuses);
  }
  smpi_bench_begin();
  return MPI_ERR_INTERN;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request *request) {
  int retval = MPI_SUCCESS;
  int dst;
  smpi_mpi_request_t *recvreq;
  smpi_bench_end();
  dst = smpi_comm_rank(comm, SIMIX_host_self());
  retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, &recvreq);
  if (NULL != recvreq) {
    smpi_irecv(recvreq);
    if (NULL != request) {
      *request = recvreq;
    }
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status *status) {
  int retval = MPI_SUCCESS;
  int dst;
  smpi_mpi_request_t *recvreq;
  smpi_bench_end();
  dst = smpi_comm_rank(comm, SIMIX_host_self());
  retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, &recvreq);
  if (NULL != recvreq) {
    smpi_irecv(recvreq);
    smpi_wait(recvreq, status);
    xbt_free(recvreq);
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request *request) {
  int retval = MPI_SUCCESS;
  int src;
  smpi_mpi_request_t *sendreq;
  smpi_bench_end();
  src = smpi_comm_rank(comm, SIMIX_host_self());
  retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, &sendreq);
  if (NULL != sendreq) {
    smpi_isend(sendreq);
    if (NULL != request) {
      *request = sendreq;
    }
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  int retval = MPI_SUCCESS;
  int src;
  smpi_mpi_request_t *sendreq;
  smpi_bench_end();
  src = smpi_comm_rank(comm, SIMIX_host_self());
  retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, &sendreq);
  if (NULL != sendreq) {
    smpi_isend(sendreq);
    smpi_wait(sendreq, MPI_STATUS_IGNORE);
  }
  smpi_bench_begin();
  return retval;
}

int MPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  int i;
  int rank;
  smpi_mpi_request_t *request;
  smpi_bench_end();

  rank = smpi_comm_rank(comm, SIMIX_host_self());

  if (root == rank) {
    smpi_create_request(buf, count, datatype, root, (rank + 1) % comm->size, 0, comm, &request);
    if (comm->size > 2) {
      request->fwdthrough = (rank - 1 + comm->size) % comm->size;
    }
    smpi_isend(request);
    smpi_wait(request, MPI_STATUS_IGNORE);
  } else {
    smpi_create_request(buf, count, datatype, MPI_ANY_SOURCE, rank, 0, comm, &request);
    if (NULL != request) {
      smpi_irecv(request);
      smpi_wait(request, MPI_STATUS_IGNORE);
    }
  }

  smpi_bench_begin();
  return MPI_SUCCESS;
}

int MPI_Barrier(MPI_Comm comm) {
  smpi_bench_end();
  smpi_barrier(comm);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

// FIXME: instead of everyone sending in order, might be a good idea to send to next...
int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int i;
  int rank;
  smpi_mpi_request_t **sendreqs, **recvreqs;
  smpi_bench_end();

  rank = smpi_comm_rank(comm, SIMIX_host_self());

  sendreqs = xbt_malloc(sizeof(smpi_mpi_request_t*) * comm->size);
  recvreqs = xbt_malloc(sizeof(smpi_mpi_request_t*) * comm->size);

  for (i = 0; i < comm->size; i++) {
    if (rank == i) {
      sendreqs[i] = NULL;
      recvreqs[i] = NULL;
      memcpy(recvbuf + recvtype->size * recvcount * i, sendbuf + sendtype->size * sendcount * i, recvtype->size * recvcount);
    } else {
      smpi_create_request(sendbuf + sendtype->size * sendcount * i, sendcount, sendtype, rank, i, 0, comm, sendreqs + i);
      smpi_isend(sendreqs[i]);
      smpi_create_request(recvbuf + recvtype->size * recvcount * i, recvcount, recvtype, i, rank, 0, comm, recvreqs + i);
      smpi_irecv(recvreqs[i]);
    }
  } 

  smpi_wait_all_nostatus(comm->size, sendreqs);
  smpi_wait_all_nostatus(comm->size, recvreqs);

  xbt_free(sendreqs);
  xbt_free(recvreqs);

  smpi_bench_begin();
  return MPI_SUCCESS;
}

// FIXME: mpi routines shouldn't call mpi routines, complexity belongs at a lower level
// also, there's probably a really clever way to overlap everything for this one...
int MPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  MPI_Bcast(recvbuf, count, datatype, 0, comm);
  return MPI_SUCCESS;
}

// FIXME: check if behavior defined when send/recv bufs are same...
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  int i, j;
  int rank;
  smpi_mpi_request_t **requests;
  void **scratchbuf;
  smpi_bench_end();

  rank = smpi_comm_rank(comm, SIMIX_host_self());

  if (root == rank) {
    requests = xbt_malloc(sizeof(smpi_mpi_request_t*) * comm->size);
    scratchbuf = xbt_malloc(sizeof(void*) * comm->size);
    memcpy(recvbuf, sendbuf, datatype->size * count);
    for (i = 0; i < comm->size; i++) {
      if (rank == i) {
        requests[i] = NULL;
        scratchbuf[i] = NULL;
      } else {
        scratchbuf[i] = xbt_malloc(datatype->size * count);
        smpi_create_request(scratchbuf[i], count, datatype, MPI_ANY_SOURCE, rank, 0, comm, requests + i);
        smpi_irecv(requests[i]);
      }
    }
    smpi_wait_all_nostatus(comm->size, requests); // FIXME: use wait_any for slight performance gain
    for (i = 0; i < comm->size; i++) {
      if (rank != i) {
        for (j = 0; j < count; j++) {
          op->func(scratchbuf[i] + datatype->size * j, recvbuf + datatype->size * j, recvbuf + datatype->size * j);
        }
        xbt_free(requests[i]);
        xbt_free(scratchbuf[i]);
      }
    }
    xbt_free(requests);
    xbt_free(scratchbuf);
  } else {
    requests = xbt_malloc(sizeof(smpi_mpi_request_t*));
    smpi_create_request(sendbuf, count, datatype, rank, root, 0, comm, requests);
    smpi_isend(*requests);
    smpi_wait(*requests, MPI_STATUS_IGNORE);
    xbt_free(*requests);
    xbt_free(requests);
  }

  smpi_bench_begin();
  return MPI_SUCCESS;
}
