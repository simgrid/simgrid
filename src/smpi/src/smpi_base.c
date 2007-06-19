#include <stdio.h>
#include <sys/time.h>
#include "msg/msg.h"
#include "simix/simix.h"
#include "xbt/sysdep.h"
#include "xbt/xbt_portability.h"
#include "smpi.h"

smpi_mpi_request_t **smpi_pending_send_requests      = NULL;
smpi_mpi_request_t **smpi_last_pending_send_requests = NULL;

smpi_mpi_request_t **smpi_pending_recv_requests      = NULL;
smpi_mpi_request_t **smpi_last_pending_recv_requests = NULL;

smpi_received_t **smpi_received                      = NULL;
smpi_received_t **smpi_last_received                 = NULL;

smx_process_t *smpi_sender_processes                 = NULL;
smx_process_t *smpi_receiver_processes               = NULL;

int smpi_running_hosts = 0;

smpi_mpi_communicator_t smpi_mpi_comm_world;

smpi_mpi_status_t smpi_mpi_status_ignore;

smpi_mpi_datatype_t smpi_mpi_byte;
smpi_mpi_datatype_t smpi_mpi_int;
smpi_mpi_datatype_t smpi_mpi_double;

smpi_mpi_op_t smpi_mpi_land;
smpi_mpi_op_t smpi_mpi_sum;

static xbt_os_timer_t smpi_timer;
static int smpi_benchmarking;
static double smpi_reference;

void smpi_mpi_land_func(void *x, void *y, void *z) {
  *(int *)z = *(int *)x && *(int *)y;
}

void smpi_mpi_sum_func(void *x, void *y, void *z) {
  *(int *)z = *(int *)x + *(int *)y;
}

void smpi_mpi_init() {
  int i;
  int size, rank;
  smx_host_t *hosts;
  smx_host_t host;
  double duration;
  m_task_t mtask;

  // will eventually need mutex
  smpi_running_hosts++;

  // initialize some local variables
  size  = SIMIX_host_get_number();
  host  = SIMIX_host_self();
  hosts = SIMIX_host_get_table();
  for(i = 0; i < size && host != hosts[i]; i++);
  rank  = i;

  // node 0 sets the globals
  if (0 == rank) {

    // global communicator
    smpi_mpi_comm_world.id           = 0;
    smpi_mpi_comm_world.size         = size;
    smpi_mpi_comm_world.barrier      = 0;
    smpi_mpi_comm_world.hosts        = hosts;
    smpi_mpi_comm_world.processes    = xbt_malloc(sizeof(m_process_t) * size);
    smpi_mpi_comm_world.processes[0] = SIMIX_process_self();

    // mpi datatypes
    smpi_mpi_byte.size               = (size_t)1;
    smpi_mpi_int.size                = sizeof(int);
    smpi_mpi_double.size             = sizeof(double);

    // mpi operations
    smpi_mpi_land.func               = &smpi_mpi_land_func;
    smpi_mpi_sum.func                = &smpi_mpi_sum_func;

    // smpi globals
    smpi_pending_send_requests       = xbt_malloc(sizeof(smpi_mpi_request_t*) * size);
    smpi_last_pending_send_requests  = xbt_malloc(sizeof(smpi_mpi_request_t*) * size);
    smpi_pending_recv_requests       = xbt_malloc(sizeof(smpi_mpi_request_t*) * size);
    smpi_last_pending_recv_requests  = xbt_malloc(sizeof(smpi_mpi_request_t*) * size);
    smpi_received                    = xbt_malloc(sizeof(smpi_received_t*) * size);
    smpi_last_received               = xbt_malloc(sizeof(smpi_received_t*) * size);
    smpi_sender_processes            = xbt_malloc(sizeof(m_process_t) * size);
    smpi_receiver_processes          = xbt_malloc(sizeof(m_process_t) * size);
    for(i = 0; i < size; i++) {
      smpi_pending_send_requests[i]      = NULL;
      smpi_last_pending_send_requests[i] = NULL;
      smpi_pending_recv_requests[i]      = NULL;
      smpi_last_pending_recv_requests[i] = NULL;
      smpi_received[i]                   = NULL;
      smpi_last_received[i]              = NULL;
    }
    smpi_timer                      = xbt_os_timer_new();
    smpi_reference                  = DEFAULT_POWER;
    smpi_benchmarking               = 0;

    // tell send/recv nodes to begin
    for(i = 0; i < size; i++) {
      mtask = MSG_task_create("READY", 0, 0, NULL);
      MSG_task_put(mtask, hosts[i], SEND_SYNC_PORT);
      mtask = (m_task_t)0;
      MSG_task_get_from_host(&mtask, SEND_SYNC_PORT, hosts[i]);
      MSG_task_destroy(mtask);
      mtask = MSG_task_create("READY", 0, 0, NULL);
      MSG_task_put(mtask, hosts[i], RECV_SYNC_PORT);
      mtask = (m_task_t)0;
      MSG_task_get_from_host(&mtask, RECV_SYNC_PORT, hosts[i]);
      MSG_task_destroy(mtask);
    }

    // now everyone else
    for(i = 1; i < size; i++) {
      mtask = MSG_task_create("READY", 0, 0, NULL);
      MSG_task_put(mtask, hosts[i], MPI_PORT);
    }

  } else {
    // everyone needs to wait for node 0 to finish
    mtask = (m_task_t)0;
    MSG_task_get(&mtask, MPI_PORT);
    MSG_task_destroy(mtask);
    smpi_mpi_comm_world.processes[rank] = SIMIX_process_self();
  }

  // now that mpi_comm_world_processes is set, it's safe to set a barrier
  smpi_barrier(&smpi_mpi_comm_world);
}

void smpi_mpi_finalize() {
  int i;
  smpi_running_hosts--;
  if (0 <= smpi_running_hosts) {
    for(i = 0; i < smpi_mpi_comm_world.size; i++) {
      if(SIMIX_process_is_suspended(smpi_sender_processes[i])) {
        SIMIX_process_resume(smpi_sender_processes[i]);
      }
      if(SIMIX_process_is_suspended(smpi_receiver_processes[i])) {
        SIMIX_process_resume(smpi_receiver_processes[i]);
      }
    }
  } else {
    xbt_free(smpi_mpi_comm_world.processes);
    xbt_free(smpi_pending_send_requests);
    xbt_free(smpi_last_pending_send_requests);
    xbt_free(smpi_pending_recv_requests);
    xbt_free(smpi_last_pending_recv_requests);
    xbt_free(smpi_received);
    xbt_free(smpi_last_received);
    xbt_free(smpi_sender_processes);
    xbt_free(smpi_receiver_processes);
    xbt_os_timer_free(smpi_timer);
  }
}

void smpi_complete(smpi_mpi_request_t *request) {
  smpi_waitlist_node_t *current, *next;
  request->completed = 1;
  request->next      = NULL;
  current = request->waitlist;
  while(NULL != current) {
    if(SIMIX_process_is_suspended(current->process)) {
      SIMIX_process_resume(current->process);
    }
    next = current->next;
    xbt_free(current);
    current = next;
  }
  request->waitlist  = NULL;
}

int smpi_host_rank_self() {
  return smpi_comm_rank(&smpi_mpi_comm_world, SIMIX_host_self());
}

void smpi_isend(smpi_mpi_request_t *sendreq) {
  int rank = smpi_host_rank_self();
  if (NULL == smpi_last_pending_send_requests[rank]) {
    smpi_pending_send_requests[rank] = sendreq;
  } else {
    smpi_last_pending_send_requests[rank]->next = sendreq;
  }
  smpi_last_pending_send_requests[rank] = sendreq;
  if (SIMIX_process_is_suspended(smpi_sender_processes[rank])) {
    SIMIX_process_resume(smpi_sender_processes[rank]);
  }
}

void smpi_match_requests(int rank) {
  smpi_mpi_request_t *frequest, *prequest, *crequest;
  smpi_received_t *freceived, *preceived, *creceived;
  size_t dsize;
  short int match;
  frequest  = smpi_pending_recv_requests[rank];
  prequest  = NULL;
  crequest  = frequest;
  while(NULL != crequest) {
    freceived = smpi_received[rank];
    preceived = NULL;
    creceived = freceived;
    match     = 0;
    while(NULL != creceived && !match) {
      if(crequest->comm->id == creceived->commid && 
        (MPI_ANY_SOURCE == crequest->src || crequest->src == creceived->src) && 
        crequest->tag == creceived->tag) {

        // we have a match!
        match = 1;

        // pull the request from the queue
        if(NULL == prequest) {
          frequest = crequest->next;
          smpi_pending_recv_requests[rank] = frequest;
        } else {
          prequest->next = crequest->next;
        }
        if(crequest == smpi_last_pending_recv_requests[rank]) {
          smpi_last_pending_recv_requests[rank] = prequest;
        }

        // pull the received data from the queue
        if(NULL == preceived) {
          freceived = creceived->next;
          smpi_received[rank] = freceived;
        } else {
          preceived->next = creceived->next;
        }
        if(creceived == smpi_last_received[rank]) {
          smpi_last_received[rank] = preceived;
        }

        // for when request->src is any source
        crequest->src = creceived->src;

        // calculate data size
        dsize = crequest->count * crequest->datatype->size;

        // copy data to buffer
        memcpy(crequest->buf, creceived->data, dsize);

        // fwd through
        crequest->fwdthrough = creceived->fwdthrough;

        // get rid of received data node, no longer needed
        xbt_free(creceived->data);
        xbt_free(creceived);

        if (crequest->fwdthrough == rank) {
          smpi_complete(crequest);
        } else {
          crequest->src = rank;
          crequest->dst = (rank + 1) % crequest->comm->size;
          smpi_isend(crequest);
        }

      } else {
        preceived = creceived;
        creceived = creceived->next;
      }
    }
    prequest = crequest;
    crequest = crequest->next;
  }
}

void smpi_bench_begin() {
  xbt_assert0(!smpi_benchmarking, "Already benchmarking");
  smpi_benchmarking = 1;
  xbt_os_timer_start(smpi_timer);
  return;
}

void smpi_bench_end() {
  m_task_t ctask = NULL;
  double duration;
  xbt_assert0(smpi_benchmarking, "Not benchmarking yet");
  smpi_benchmarking = 0;
  xbt_os_timer_stop(smpi_timer);
  duration = xbt_os_timer_elapsed(smpi_timer);
  ctask = MSG_task_create("computation", duration * smpi_reference, 0 , NULL);
  MSG_task_execute(ctask);
  MSG_task_destroy(ctask);
  return;
}

int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t *datatype,
  int src, int dst, int tag, smpi_mpi_communicator_t *comm, smpi_mpi_request_t **request) {
  int retval = MPI_SUCCESS;
  *request = NULL;
  if (NULL == buf && 0 < count) {
    retval = MPI_ERR_INTERN;
  } else if (0 > count) {
    retval = MPI_ERR_COUNT;
  } else if (NULL == datatype) {
    retval = MPI_ERR_TYPE;
  } else if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else if (MPI_ANY_SOURCE != src && (0 > src || comm->size <= src)) {
    retval = MPI_ERR_RANK;
  } else if (0 > dst || comm->size <= dst) {
    retval = MPI_ERR_RANK;
  } else if (0 > tag) {
    retval = MPI_ERR_TAG;
  } else {
    *request = xbt_malloc(sizeof(smpi_mpi_request_t));
    (*request)->buf        = buf;
    (*request)->count      = count;
    (*request)->datatype   = datatype;
    (*request)->src        = src;
    (*request)->dst        = dst;
    (*request)->tag        = tag;
    (*request)->comm       = comm;
    (*request)->completed  = 0;
    (*request)->fwdthrough = dst;
    (*request)->waitlist   = NULL;
    (*request)->next       = NULL;
  }
  return retval;
}

void smpi_barrier(smpi_mpi_communicator_t *comm) {
  int i;
  comm->barrier++;
  if(comm->barrier < comm->size) {
    SIMIX_process_suspend(SIMIX_process_self());
  } else {
    comm->barrier = 0;
    for(i = 0; i < comm->size; i++) {
      if (SIMIX_process_is_suspended(comm->processes[i])) {
        SIMIX_process_resume(comm->processes[i]);
      }
    }
  }
}

int smpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host) {
  int i;
  for(i = 0; i < comm->size && host != comm->hosts[i]; i++);
  if (i >= comm->size) i = -1;
  return i;
}

void smpi_irecv(smpi_mpi_request_t *recvreq) {
  int rank = smpi_host_rank_self();
  if (NULL == smpi_pending_recv_requests[rank]) {
    smpi_pending_recv_requests[rank] = recvreq;
  } else if (NULL != smpi_last_pending_recv_requests[rank]) {
    smpi_last_pending_recv_requests[rank]->next = recvreq;
  } else { // can't happen!
    fprintf(stderr, "smpi_pending_recv_requests not null while smpi_last_pending_recv_requests null!\n");
  }
  smpi_last_pending_recv_requests[rank] = recvreq;
  smpi_match_requests(rank);
  if (SIMIX_process_is_suspended(smpi_receiver_processes[rank])) {
    SIMIX_process_resume(smpi_receiver_processes[rank]);
  }
}

void smpi_wait(smpi_mpi_request_t *request, smpi_mpi_status_t *status) {
  smpi_waitlist_node_t *waitnode, *current;
  if (NULL != request) {
    if (!request->completed) {
      waitnode = xbt_malloc(sizeof(smpi_waitlist_node_t));
      waitnode->process = SIMIX_process_self();
      waitnode->next    = NULL;
      if (NULL == request->waitlist) {
        request->waitlist = waitnode;
      } else {
        for(current = request->waitlist; NULL != current->next; current = current->next);
        current->next = waitnode;
      }
      SIMIX_process_suspend(waitnode->process);
    }
    if (NULL != status && MPI_STATUS_IGNORE != status) {
      status->MPI_SOURCE = request->src;
    }
  }
}

void smpi_wait_all(int count, smpi_mpi_request_t **requests, smpi_mpi_status_t *statuses) {
  int i;
  for (i = 0; i < count; i++) {
    smpi_wait(requests[i], &statuses[i]);
  }
}

void smpi_wait_all_nostatus(int count, smpi_mpi_request_t **requests) {
  int i;
  for (i = 0; i < count; i++) {
    smpi_wait(requests[i], MPI_STATUS_IGNORE);
  }
}

int smpi_sender(int argc, char *argv[]) {
  smx_process_t process;
  char taskname[50];
  size_t dsize;
  void *data;
  smx_host_t dhost;
  m_task_t mtask;
  int rank, fc, ft;
  smpi_mpi_request_t *sendreq;

  process = SIMIX_process_self();

  // wait for init
  mtask = (m_task_t)0;
  MSG_task_get(&mtask, SEND_SYNC_PORT);

  rank = smpi_host_rank_self();

  smpi_sender_processes[rank] = process;

  // ready!
  MSG_task_put(mtask, MSG_task_get_source(mtask), SEND_SYNC_PORT);

  while (0 < smpi_running_hosts) {
    sendreq = smpi_pending_send_requests[rank];
    if (NULL != sendreq) {

      // pull from queue if not a fwd or no more to fwd
      if (sendreq->dst == sendreq->fwdthrough) {
        smpi_pending_send_requests[rank] = sendreq->next;
        if(sendreq == smpi_last_pending_send_requests[rank]) {
          smpi_last_pending_send_requests[rank] = NULL;
        }
        ft = sendreq->dst;
      } else {
        fc = ((sendreq->fwdthrough - sendreq->dst + sendreq->comm->size) % sendreq->comm->size) / 2;
        ft = (sendreq->dst + fc) % sendreq->comm->size;
        //printf("node %d sending broadcast to node %d through node %d\n", rank, sendreq->dst, ft);
      }

      // create task to send
      sprintf(taskname, "comm:%d,src:%d,dst:%d,tag:%d,ft:%d", sendreq->comm->id, sendreq->src, sendreq->dst, sendreq->tag, ft);
      dsize = sendreq->count * sendreq->datatype->size;
      data  = xbt_malloc(dsize);
      memcpy(data, sendreq->buf, dsize);
      mtask = MSG_task_create(taskname, 0, dsize, data);

      // figure out which host to send it to
      dhost = sendreq->comm->hosts[sendreq->dst];

      // send task
      #ifdef DEBUG
        printf("host %s attempting to send to host %s\n", SIMIX_host_get_name(SIMIX_host_self()), SIMIX_host_get_name(dhost));
      #endif
      MSG_task_put(mtask, dhost, MPI_PORT);

      if (sendreq->dst == sendreq->fwdthrough) {
        smpi_complete(sendreq);
      } else {
        sendreq->dst = (sendreq->dst + fc + 1) % sendreq->comm->size;
      }

    } else {
      SIMIX_process_suspend(process);
    }
  }
  return 0;
}

int smpi_receiver(int argc, char **argv) {
  smx_process_t process;
  m_task_t mtask;
  smpi_received_t *received;
  int rank;
  smpi_mpi_request_t *recvreq;

  process = SIMIX_process_self();

  // wait for init
  mtask = (m_task_t)0;
  MSG_task_get(&mtask, RECV_SYNC_PORT);

  rank = smpi_host_rank_self();

  // potential race condition...
  smpi_receiver_processes[rank] = process;

  // ready!
  MSG_task_put(mtask, MSG_task_get_source(mtask), RECV_SYNC_PORT);

  while (0 < smpi_running_hosts) {
    recvreq = smpi_pending_recv_requests[rank];
    if (NULL != recvreq) {
      mtask = (m_task_t)0;

      #ifdef DEBUG
        printf("host %s waiting to receive from anyone, but first in queue is (%d,%d,%d).\n",
	  SIMIX_host_get_name(SIMIX_host_self()), recvreq->src, recvreq->dst, recvreq->tag);
      #endif
      MSG_task_get(&mtask, MPI_PORT);

      received = xbt_malloc(sizeof(smpi_received_t));

      sscanf(MSG_task_get_name(mtask), "comm:%d,src:%d,dst:%d,tag:%d,ft:%d",
        &received->commid, &received->src, &received->dst, &received->tag, &received->fwdthrough);
      received->data = MSG_task_get_data(mtask);
      received->next = NULL;

      if (NULL == smpi_last_received[rank]) {
        smpi_received[rank] = received;
      } else {
        smpi_last_received[rank]->next = received;
      }
      smpi_last_received[rank] = received;

      MSG_task_destroy(mtask);

      smpi_match_requests(rank);

    } else {
      SIMIX_process_suspend(process);
    }
  }
  return 0;
}

// FIXME: move into own file
int smpi_gettimeofday(struct timeval *tv, struct timezone *tz) {
  double now;
  int retval = 0;
  smpi_bench_end();
  if (NULL == tv) {
    retval = -1;
  } else {
    now = SIMIX_get_clock();
    tv->tv_sec  = now;
    tv->tv_usec = ((now - (double)tv->tv_sec) * 1000000.0);
  }
  smpi_bench_begin();
  return retval;
}

unsigned int smpi_sleep(unsigned int seconds) {
  m_task_t task = NULL;
  smpi_bench_end();
  task = MSG_task_create("sleep", seconds * DEFAULT_POWER, 0, NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  smpi_bench_begin();
  return 0;
}

void smpi_exit(int status) {
  smpi_bench_end();
  smpi_running_hosts--;
  SIMIX_process_kill(SIMIX_process_self());
  return;
}
