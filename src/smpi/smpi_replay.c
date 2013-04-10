/* Copyright (c) 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "private.h"
#include <stdio.h>
#include <xbt.h>
#include <xbt/replay.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

int communicator_size = 0;
static int active_processes = 0;
xbt_dynar_t *reqq = NULL;

MPI_Datatype MPI_DEFAULT_TYPE, MPI_CURRENT_TYPE;

static void log_timed_action (const char *const *action, double clock){
  if (XBT_LOG_ISENABLED(smpi_replay, xbt_log_priority_verbose)){
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, smpi_process_simulated_elapsed()-clock);
    free(name);
  }
}

typedef struct {
  xbt_dynar_t isends; /* of MPI_Request */
  xbt_dynar_t irecvs; /* of MPI_Request */
} s_smpi_replay_globals_t, *smpi_replay_globals_t;


/* Helper function */
static double parse_double(const char *string)
{
  double value;
  char *endptr;
  value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROWF(unknown_error, 0, "%s is not a double", string);
  return value;
}

static MPI_Datatype decode_datatype(const char *const action)
{
// Declared datatypes,
 
  switch(atoi(action))
  {
    case 0:
      MPI_CURRENT_TYPE=MPI_DOUBLE;
      break;
    case 1:
      MPI_CURRENT_TYPE=MPI_INT;
      break;
    case 2:
      MPI_CURRENT_TYPE=MPI_CHAR;
      break;
    case 3:
      MPI_CURRENT_TYPE=MPI_SHORT;
      break;
    case 4:
      MPI_CURRENT_TYPE=MPI_LONG;
      break;
    case 5:
      MPI_CURRENT_TYPE=MPI_FLOAT;
      break;
    case 6:
      MPI_CURRENT_TYPE=MPI_BYTE;
      break;
    default:
      MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;
  
  }
   return MPI_CURRENT_TYPE;
}

static void action_init(const char *const *action)
{
  int i;
  XBT_DEBUG("Initialize the counters");
  smpi_replay_globals_t globals =  xbt_new(s_smpi_replay_globals_t, 1);
  globals->isends = xbt_dynar_new(sizeof(MPI_Request),NULL);
  globals->irecvs = xbt_dynar_new(sizeof(MPI_Request),NULL);

  if(action[2]) MPI_DEFAULT_TYPE= MPI_DOUBLE; // default MPE dataype 
  else MPI_DEFAULT_TYPE= MPI_BYTE; // default TAU datatype
  
  smpi_process_set_user_data((void*) globals);

  /* start a simulated timer */
  smpi_process_simulated_start();
  /*initialize the number of active processes */
  active_processes = smpi_process_count();

  if (!reqq) {
    reqq=xbt_new0(xbt_dynar_t,active_processes);
  
    for(i=0;i<active_processes;i++){
      reqq[i]=xbt_dynar_new(sizeof(MPI_Request),NULL);
    }
  }
}

static void action_finalize(const char *const *action)
{
  smpi_replay_globals_t globals =
      (smpi_replay_globals_t) smpi_process_get_user_data();
  if (globals){
    XBT_DEBUG("There are %lu isends and %lu irecvs in the dynars",
         xbt_dynar_length(globals->isends),xbt_dynar_length(globals->irecvs));
    xbt_dynar_free_container(&(globals->isends));
    xbt_dynar_free_container(&(globals->irecvs));
  }
  free(globals);
}

static void action_comm_size(const char *const *action)
{
  double clock = smpi_process_simulated_elapsed();

  communicator_size = parse_double(action[2]);
  log_timed_action (action, clock);
}

static void action_comm_split(const char *const *action)
{
  double clock = smpi_process_simulated_elapsed();

  log_timed_action (action, clock);
}

static void action_comm_dup(const char *const *action)
{
  double clock = smpi_process_simulated_elapsed();

  log_timed_action (action, clock);
}

static void action_compute(const char *const *action)
{
  double clock = smpi_process_simulated_elapsed();
  smpi_execute_flops(parse_double(action[2]));

  log_timed_action (action, clock);
}

static void action_send(const char *const *action)
{
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();

  if(action[4]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  } else {
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;
  }
    
#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank);
  int dst_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), to);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__);
  TRACE_smpi_send(rank, rank, dst_traced);
#endif

  smpi_mpi_send(NULL, size, MPI_CURRENT_TYPE, to , 0, MPI_COMM_WORLD);

  log_timed_action (action, clock);

  #ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif

}

static void action_Isend(const char *const *action)
{
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;

  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  smpi_replay_globals_t globals =
     (smpi_replay_globals_t) smpi_process_get_user_data();
#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank);
  int dst_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), to);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__);
  TRACE_smpi_send(rank, rank, dst_traced);
#endif

  request = smpi_mpi_isend(NULL, size, MPI_CURRENT_TYPE, to, 0,MPI_COMM_WORLD);
  
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  request->send = 1;
  TRACE_smpi_computing_in(rank);
#endif

  xbt_dynar_push(globals->isends,&request);
  xbt_dynar_push(reqq[smpi_comm_rank(MPI_COMM_WORLD)],&request);

  log_timed_action (action, clock);
}

static void action_recv(const char *const *action) {
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Status status;

  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;
  
#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  int src_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), from);
  TRACE_smpi_computing_out(rank);

  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__);
#endif

  smpi_mpi_recv(NULL, size, MPI_CURRENT_TYPE, from, 0, MPI_COMM_WORLD, &status);

#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  TRACE_smpi_recv(rank, src_traced, rank);
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
}

static void action_Irecv(const char *const *action)
{
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;

  smpi_replay_globals_t globals =
     (smpi_replay_globals_t) smpi_process_get_user_data();
  
  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  int src_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), from);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__);
#endif

  request = smpi_mpi_irecv(NULL, size, MPI_CURRENT_TYPE, from, 0, MPI_COMM_WORLD);
  
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  request->recv = 1;
#endif
  xbt_dynar_push(globals->irecvs,&request);
  xbt_dynar_push(reqq[smpi_comm_rank(MPI_COMM_WORLD)],&request);

  log_timed_action (action, clock);
}

static void action_wait(const char *const *action){
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;
  MPI_Status status;
  smpi_replay_globals_t globals =
      (smpi_replay_globals_t) smpi_process_get_user_data();

  xbt_assert(xbt_dynar_length(globals->irecvs),
      "action wait not preceded by any irecv: %s",
      xbt_str_join_array(action," "));
  request = xbt_dynar_pop_as(globals->irecvs,MPI_Request);
#ifdef HAVE_TRACING
  int rank = request && request->comm != MPI_COMM_NULL
      ? smpi_comm_rank(request->comm)
      : -1;
  TRACE_smpi_computing_out(rank);

  MPI_Group group = smpi_comm_group(request->comm);
  int src_traced = smpi_group_rank(group, request->src);
  int dst_traced = smpi_group_rank(group, request->dst);
  int is_wait_for_receive = request->recv;
  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__);
#endif
  smpi_mpi_wait(&request, &status);
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  if (is_wait_for_receive) {
    TRACE_smpi_recv(rank, src_traced, dst_traced);
  }
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
}

static void action_waitall(const char *const *action){
  double clock = smpi_process_simulated_elapsed();
  int count_requests=0;
  unsigned int i=0;

  count_requests=xbt_dynar_length(reqq[smpi_comm_rank(MPI_COMM_WORLD)]);

  if (count_requests>0) {
    MPI_Request requests[count_requests];
    MPI_Status status[count_requests];
  
    /*  The reqq is an array of dynars. Its index corresponds to the rank.
     Thus each rank saves its own requests to the array request. */
    xbt_dynar_foreach(reqq[smpi_comm_rank(MPI_COMM_WORLD)],i,requests[i]); 
    
  #ifdef HAVE_TRACING
   //save information from requests
 
   xbt_dynar_t srcs = xbt_dynar_new(sizeof(int), NULL);
   xbt_dynar_t dsts = xbt_dynar_new(sizeof(int), NULL);
   xbt_dynar_t recvs = xbt_dynar_new(sizeof(int), NULL);
   for (i = 0; i < count_requests; i++) {
    if(requests[i]){
      int *asrc = xbt_new(int, 1);
      int *adst = xbt_new(int, 1);
      int *arecv = xbt_new(int, 1);
      *asrc = requests[i]->src;
      *adst = requests[i]->dst;
      *arecv = requests[i]->recv;
      xbt_dynar_insert_at(srcs, i, asrc);
      xbt_dynar_insert_at(dsts, i, adst);
      xbt_dynar_insert_at(recvs, i, arecv);
      xbt_free(asrc);
      xbt_free(adst);
      xbt_free(arecv);
    }else {
      int *t = xbt_new(int, 1);
      xbt_dynar_insert_at(srcs, i, t);
      xbt_dynar_insert_at(dsts, i, t);
      xbt_dynar_insert_at(recvs, i, t);
      xbt_free(t);
    }
   }
   int rank_traced = smpi_process_index();
   TRACE_smpi_computing_out(rank_traced);

   TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__);
  #endif

    smpi_mpi_waitall(count_requests, requests, status);

  #ifdef HAVE_TRACING
   for (i = 0; i < count_requests; i++) {
    int src_traced, dst_traced, is_wait_for_receive;
    xbt_dynar_get_cpy(srcs, i, &src_traced);
    xbt_dynar_get_cpy(dsts, i, &dst_traced);
    xbt_dynar_get_cpy(recvs, i, &is_wait_for_receive);
    if (is_wait_for_receive) {
      TRACE_smpi_recv(rank_traced, src_traced, dst_traced);
    }
   }
   TRACE_smpi_ptp_out(rank_traced, -1, -1, __FUNCTION__);
   //clean-up of dynars
   xbt_dynar_free(&srcs);
   xbt_dynar_free(&dsts);
   xbt_dynar_free(&recvs);
   TRACE_smpi_computing_in(rank_traced);
  #endif
   
   xbt_dynar_reset(reqq[smpi_comm_rank(MPI_COMM_WORLD)]);
  }
  log_timed_action (action, clock);
}

static void action_barrier(const char *const *action){
  double clock = smpi_process_simulated_elapsed();
#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  smpi_mpi_barrier(MPI_COMM_WORLD);
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
}


static void action_bcast(const char *const *action)
{
  double size = parse_double(action[2]);
  double clock = smpi_process_simulated_elapsed();
  int root=0;
  /*
   * Initialize MPI_CURRENT_TYPE in order to decrease
   * the number of the checks
   * */
  MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;  

  if(action[3]) {
    root= atoi(action[3]);
    if(action[4]) {
      MPI_CURRENT_TYPE=decode_datatype(action[4]);   
    }
  }
  
#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), 0);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif

  smpi_mpi_bcast(NULL, size, MPI_CURRENT_TYPE, root, MPI_COMM_WORLD);
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
}

static void action_reduce(const char *const *action)
{
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  int root=0;
  MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;
  
  if(action[4]) {
      root= atoi(action[4]);
      if(action[5]) {
	MPI_CURRENT_TYPE=decode_datatype(action[5]);
      }
  }

#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), 0);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
   smpi_mpi_reduce(NULL, NULL, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, root, MPI_COMM_WORLD);
   smpi_execute_flops(comp_size);
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
}

static void action_allReduce(const char *const *action) {
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
  
  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;
  
  double clock = smpi_process_simulated_elapsed();
#ifdef HAVE_TRACING
  int rank = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  smpi_mpi_reduce(NULL, NULL, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, 0, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);
  smpi_mpi_bcast(NULL, comm_size, MPI_CURRENT_TYPE, 0, MPI_COMM_WORLD);
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
}

static void action_allToAll(const char *const *action) {
  double clock = smpi_process_simulated_elapsed();
  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  int send_size = parse_double(action[2]);
  int recv_size = parse_double(action[3]);
  void *send = xbt_new0(int, send_size*comm_size);  
  void *recv = xbt_new0(int, send_size*comm_size);
  
  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

#ifdef HAVE_TRACING
  int rank = smpi_process_index();
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif

  if (send_size < 200 && comm_size > 12) {
    smpi_coll_tuned_alltoall_bruck(send, send_size, MPI_CURRENT_TYPE,
                                   recv, recv_size, MPI_CURRENT_TYPE,
                                   MPI_COMM_WORLD);
  } else if (send_size < 3000) {  
    smpi_coll_tuned_alltoall_basic_linear(send, send_size, MPI_CURRENT_TYPE,
                                          recv, recv_size, MPI_CURRENT_TYPE,
                                          MPI_COMM_WORLD);
  } else {
    smpi_coll_tuned_alltoall_pairwise(send, send_size, MPI_CURRENT_TYPE,
                                      recv, recv_size, MPI_CURRENT_TYPE,
                                      MPI_COMM_WORLD);
  }

#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif

  log_timed_action (action, clock);
  xbt_free(send);
  xbt_free(recv);
}

static void action_allToAllv(const char *const *action) {
  /*
 The structure of the allToAllV action for the rank 0 (total 4 processes) 
 is the following:   
  0 allToAllV 100 1 7 10 12 5 10 20 45 100 1 70 10 5 1 5 77 90

  where: 
  1) 100 is the size of the send buffer *sizeof(int),
  2) 1 7 10 12 is the sendcounts array
  3) 5 10 20 45 is the sdispls array
  4) 100*sizeof(int) is the size of the receiver buffer
  5)  1 70 10 5 is the recvcounts array
  6) 1 5 77 90 is the rdispls array
    
   */
  
  
  double clock = smpi_process_simulated_elapsed();
  
  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  int send_buf_size=0,recv_buf_size=0,i=0;
  int *sendcounts = xbt_new0(int, comm_size);  
  int *recvcounts = xbt_new0(int, comm_size);  
  int *senddisps = xbt_new0(int, comm_size);  
  int *recvdisps = xbt_new0(int, comm_size);  
  
  send_buf_size=parse_double(action[2]);
  recv_buf_size=parse_double(action[3+2*comm_size]);

  int *sendbuf = xbt_new0(int, send_buf_size);  
  int *recvbuf = xbt_new0(int, recv_buf_size);  

  if(action[4+4*comm_size]) MPI_CURRENT_TYPE=decode_datatype(action[4+4*comm_size]);    
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  for(i=0;i<comm_size;i++) {
    sendcounts[i] = atoi(action[i+3]);
    senddisps[i] = atoi(action[i+3+comm_size]);
    recvcounts[i] = atoi(action[i+4+2*comm_size]);
    recvdisps[i] = atoi(action[i+4+3*comm_size]);
  }
  

#ifdef HAVE_TRACING
  int rank = MPI_COMM_WORLD != MPI_COMM_NULL ? smpi_process_index() : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
    smpi_coll_basic_alltoallv(sendbuf, sendcounts, senddisps, 	MPI_CURRENT_TYPE,
                               recvbuf, recvcounts, recvdisps, MPI_CURRENT_TYPE,
                               MPI_COMM_WORLD);
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
   
  log_timed_action (action, clock);
  xbt_free(sendbuf);
  xbt_free(recvbuf);
  xbt_free(sendcounts);
  xbt_free(recvcounts);
  xbt_free(senddisps);
  xbt_free(recvdisps);

  
}

void smpi_replay_init(int *argc, char***argv){
  PMPI_Init(argc, argv);
  if (!smpi_process_index()){
    _xbt_replay_action_init();
    xbt_replay_action_register("init",       action_init);
    xbt_replay_action_register("finalize",   action_finalize);
    xbt_replay_action_register("comm_size",  action_comm_size);
    xbt_replay_action_register("comm_split", action_comm_split);
    xbt_replay_action_register("comm_dup",   action_comm_dup);
    xbt_replay_action_register("send",       action_send);
    xbt_replay_action_register("Isend",      action_Isend);
    xbt_replay_action_register("recv",       action_recv);
    xbt_replay_action_register("Irecv",      action_Irecv);
    xbt_replay_action_register("wait",       action_wait);
    xbt_replay_action_register("waitAll",    action_waitall);
    xbt_replay_action_register("barrier",    action_barrier);
    xbt_replay_action_register("bcast",      action_bcast);
    xbt_replay_action_register("reduce",     action_reduce);
    xbt_replay_action_register("allReduce",  action_allReduce);
    xbt_replay_action_register("allToAll",   action_allToAll);
    xbt_replay_action_register("allToAllV",  action_allToAllv);
    xbt_replay_action_register("compute",    action_compute);
  }

  xbt_replay_action_runner(*argc, *argv);
}

int smpi_replay_finalize(){
  double sim_time= 1.;
  /* One active process will stop. Decrease the counter*/
  active_processes--;
  XBT_DEBUG("There are %lu elements in reqq[*]",
            xbt_dynar_length(reqq[smpi_comm_rank(MPI_COMM_WORLD)]));
  xbt_dynar_free(&reqq[smpi_comm_rank(MPI_COMM_WORLD)]);
  if(!active_processes){
    /* Last process alive speaking */
    /* end the simulated timer */
    sim_time = smpi_process_simulated_elapsed();
    XBT_INFO("Simulation time %g", sim_time);
    _xbt_replay_action_exit();
    xbt_free(reqq);
    reqq = NULL;
  }
  return PMPI_Finalize();
}
