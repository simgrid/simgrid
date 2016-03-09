/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include <stdio.h>
#include <xbt.h>
#include <xbt/replay.h>

#define KEY_SIZE (sizeof(int) * 2 + 1)

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

int communicator_size = 0;
static int active_processes = 0;
xbt_dict_t reqq = NULL;

MPI_Datatype MPI_DEFAULT_TYPE;
MPI_Datatype MPI_CURRENT_TYPE;

static int sendbuffer_size=0;
char* sendbuffer=NULL;
static int recvbuffer_size=0;
char* recvbuffer=NULL;

static void log_timed_action (const char *const *action, double clock){
  if (XBT_LOG_ISENABLED(smpi_replay, xbt_log_priority_verbose)){
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, smpi_process_simulated_elapsed()-clock);
    free(name);
  }
}

static xbt_dynar_t get_reqq_self()
{
   char * key = bprintf("%d", smpi_process_index());
   xbt_dynar_t dynar_mpi_request = (xbt_dynar_t) xbt_dict_get(reqq, key);
   free(key);
 
   return dynar_mpi_request;
}

static void set_reqq_self(xbt_dynar_t mpi_request)
{
   char * key = bprintf("%d", smpi_process_index());
   xbt_dict_set(reqq, key, mpi_request, free);
   free(key);
}

//allocate a single buffer for all sends, growing it if needed
void* smpi_get_tmp_sendbuffer(int size)
{
  if (!smpi_process_get_replaying())
    return xbt_malloc(size);
  if (sendbuffer_size<size){
    sendbuffer=static_cast<char*>(xbt_realloc(sendbuffer,size));
    sendbuffer_size=size;
  }
  return sendbuffer;
}

//allocate a single buffer for all recv
void* smpi_get_tmp_recvbuffer(int size){
  if (!smpi_process_get_replaying())
  return xbt_malloc(size);
  if (recvbuffer_size<size){
    recvbuffer=static_cast<char*>(xbt_realloc(recvbuffer,size));
    recvbuffer_size=size;
  }
  return recvbuffer;
}

void smpi_free_tmp_buffer(void* buf){
  if (!smpi_process_get_replaying())
    xbt_free(buf);
}

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
  switch(atoi(action)) {
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


const char* encode_datatype(MPI_Datatype datatype, int* known)
{
  //default type for output is set to MPI_BYTE
  // MPI_DEFAULT_TYPE is not set for output, use directly MPI_BYTE
  if(known)*known=1;
  if (datatype==MPI_BYTE){
      return "";
  }
  if(datatype==MPI_DOUBLE)
      return "0";
  if(datatype==MPI_INT)
      return "1";
  if(datatype==MPI_CHAR)
      return "2";
  if(datatype==MPI_SHORT)
      return "3";
  if(datatype==MPI_LONG)
    return "4";
  if(datatype==MPI_FLOAT)
      return "5";
  //tell that the datatype is not handled by replay, and that its size should be measured and replayed as size*MPI_BYTE
  if(known)*known=0;
  // default - not implemented.
  // do not warn here as we pass in this function even for other trace formats
  return "-1";
}

#define CHECK_ACTION_PARAMS(action, mandatory, optional) {\
    int i=0;\
    while(action[i]!=NULL)\
     i++;\
    if(i<mandatory+2)                                           \
    THROWF(arg_error, 0, "%s replay failed.\n" \
          "%d items were given on the line. First two should be process_id and action.  " \
          "This action needs after them %d mandatory arguments, and accepts %d optional ones. \n" \
          "Please contact the Simgrid team if support is needed", __FUNCTION__, i, mandatory, optional);\
  }

static void action_init(const char *const *action)
{
  XBT_DEBUG("Initialize the counters");
  CHECK_ACTION_PARAMS(action, 0, 1);
  if(action[2]) MPI_DEFAULT_TYPE= MPI_DOUBLE; // default MPE dataype 
  else MPI_DEFAULT_TYPE= MPI_BYTE; // default TAU datatype

  /* start a simulated timer */
  smpi_process_simulated_start();
  /*initialize the number of active processes */
  active_processes = smpi_process_count();

  if (!reqq) {
    reqq = xbt_dict_new();
  }

  set_reqq_self(xbt_dynar_new(sizeof(MPI_Request),&xbt_free_ref));
}

static void action_finalize(const char *const *action)
{
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
  CHECK_ACTION_PARAMS(action, 1, 0);
  double clock = smpi_process_simulated_elapsed();
  double flops= parse_double(action[2]);
  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type=TRACING_COMPUTING;
  extra->comp_size=flops;
  TRACE_smpi_computing_in(rank, extra);

  smpi_execute_flops(flops);

  TRACE_smpi_computing_out(rank);
  log_timed_action (action, clock);
}

static void action_send(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 1);
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();

  if(action[4]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  } else {
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;
  }

  int rank = smpi_process_index();

  int dst_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), to);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SEND;
  extra->send_size = size;
  extra->src = rank;
  extra->dst = dst_traced;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
  if (!TRACE_smpi_view_internals()) {
    TRACE_smpi_send(rank, rank, dst_traced, size*smpi_datatype_size(MPI_CURRENT_TYPE));
  }

  smpi_mpi_send(NULL, size, MPI_CURRENT_TYPE, to , 0, MPI_COMM_WORLD);

  log_timed_action (action, clock);

  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
}

static void action_Isend(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 1);
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;

  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process_index();
  int dst_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), to);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ISEND;
  extra->send_size = size;
  extra->src = rank;
  extra->dst = dst_traced;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
  if (!TRACE_smpi_view_internals()) {
    TRACE_smpi_send(rank, rank, dst_traced, size*smpi_datatype_size(MPI_CURRENT_TYPE));
  }

  request = smpi_mpi_isend(NULL, size, MPI_CURRENT_TYPE, to, 0,MPI_COMM_WORLD);

  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  request->send = 1;

  xbt_dynar_push(get_reqq_self(),&request);

  log_timed_action (action, clock);
}

static void action_recv(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 1);
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Status status;

  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process_index();
  int src_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), from);

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_RECV;
  extra->send_size = size;
  extra->src = src_traced;
  extra->dst = rank;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

  //unknow size from the receiver pov
  if(size==-1){
      smpi_mpi_probe(from, 0, MPI_COMM_WORLD, &status);
      size=status.count;
  }

  smpi_mpi_recv(NULL, size, MPI_CURRENT_TYPE, from, 0, MPI_COMM_WORLD, &status);

  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  if (!TRACE_smpi_view_internals()) {
    TRACE_smpi_recv(rank, src_traced, rank);
  }

  log_timed_action (action, clock);
}

static void action_Irecv(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 1);
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;

  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process_index();
  int src_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), from);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_IRECV;
  extra->send_size = size;
  extra->src = src_traced;
  extra->dst = rank;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);
  MPI_Status status;
  //unknow size from the receiver pov
  if(size==-1){
      smpi_mpi_probe(from, 0, MPI_COMM_WORLD, &status);
      size=status.count;
  }

  request = smpi_mpi_irecv(NULL, size, MPI_CURRENT_TYPE, from, 0, MPI_COMM_WORLD);

  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  request->recv = 1;
  xbt_dynar_push(get_reqq_self(),&request);

  log_timed_action (action, clock);
}

static void action_test(const char *const *action){
  CHECK_ACTION_PARAMS(action, 0, 0);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;
  MPI_Status status;
  int flag = true;

  request = xbt_dynar_pop_as(get_reqq_self(),MPI_Request);
  //if request is null here, this may mean that a previous test has succeeded 
  //Different times in traced application and replayed version may lead to this 
  //In this case, ignore the extra calls.
  if(request){
    int rank = smpi_process_index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type=TRACING_TEST;
    TRACE_smpi_testing_in(rank, extra);

    flag = smpi_mpi_test(&request, &status);

    XBT_DEBUG("MPI_Test result: %d", flag);
    /* push back request in dynar to be caught by a subsequent wait. if the test did succeed, the request is now NULL.*/
    xbt_dynar_push_as(get_reqq_self(),MPI_Request, request);

    TRACE_smpi_testing_out(rank);
  }
  log_timed_action (action, clock);
}

static void action_wait(const char *const *action){
  CHECK_ACTION_PARAMS(action, 0, 0);
  double clock = smpi_process_simulated_elapsed();
  MPI_Request request;
  MPI_Status status;

  xbt_assert(xbt_dynar_length(get_reqq_self()),
      "action wait not preceded by any irecv or isend: %s",
      xbt_str_join_array(action," "));
  request = xbt_dynar_pop_as(get_reqq_self(),MPI_Request);

  if (!request){
    /* Assume that the trace is well formed, meaning the comm might have been caught by a MPI_test. Then just return.*/
    return;
  }

  int rank = request->comm != MPI_COMM_NULL ? smpi_comm_rank(request->comm) : -1;

  MPI_Group group = smpi_comm_group(request->comm);
  int src_traced = smpi_group_rank(group, request->src);
  int dst_traced = smpi_group_rank(group, request->dst);
  int is_wait_for_receive = request->recv;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAIT;
  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);

  smpi_mpi_wait(&request, &status);

  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  if (is_wait_for_receive)
    TRACE_smpi_recv(rank, src_traced, dst_traced);
  log_timed_action (action, clock);
}

static void action_waitall(const char *const *action){
  CHECK_ACTION_PARAMS(action, 0, 0);
  double clock = smpi_process_simulated_elapsed();
  int count_requests=0;
  unsigned int i=0;

  count_requests=xbt_dynar_length(get_reqq_self());

  if (count_requests>0) {
    MPI_Request requests[count_requests];
    MPI_Status status[count_requests];

    /*  The reqq is an array of dynars. Its index corresponds to the rank.
     Thus each rank saves its own requests to the array request. */
    xbt_dynar_foreach(get_reqq_self(),i,requests[i]); 

   //save information from requests
   xbt_dynar_t srcs = xbt_dynar_new(sizeof(int), NULL);
   xbt_dynar_t dsts = xbt_dynar_new(sizeof(int), NULL);
   xbt_dynar_t recvs = xbt_dynar_new(sizeof(int), NULL);
   for (i = 0; (int)i < count_requests; i++) {
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
   instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
   extra->type = TRACING_WAITALL;
   extra->send_size=count_requests;
   TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

   smpi_mpi_waitall(count_requests, requests, status);

   for (i = 0; (int)i < count_requests; i++) {
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

   //TODO xbt_dynar_free_container(get_reqq_self());
   set_reqq_self(xbt_dynar_new(sizeof(MPI_Request),&xbt_free_ref));
  }
  log_timed_action (action, clock);
}

static void action_barrier(const char *const *action){
  double clock = smpi_process_simulated_elapsed();
  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_BARRIER;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  mpi_coll_barrier_fun(MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_bcast(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 1, 2);
  double size = parse_double(action[2]);
  double clock = smpi_process_simulated_elapsed();
  int root=0;
  /* Initialize MPI_CURRENT_TYPE in order to decrease the number of the checks */
  MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;  

  if(action[3]) {
    root= atoi(action[3]);
    if(action[4]) {
      MPI_CURRENT_TYPE=decode_datatype(action[4]);   
    }
  }

  int rank = smpi_process_index();
  int root_traced = smpi_group_index(smpi_comm_group(MPI_COMM_WORLD), root);

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_BCAST;
  extra->send_size = size;
  extra->root = root_traced;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);
  void *sendbuf = smpi_get_tmp_sendbuffer(size* smpi_datatype_size(MPI_CURRENT_TYPE));

  mpi_coll_bcast_fun(sendbuf, size, MPI_CURRENT_TYPE, root, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_reduce(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 2);
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

  int rank = smpi_process_index();
  int root_traced = smpi_group_rank(smpi_comm_group(MPI_COMM_WORLD), root);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE;
  extra->send_size = comm_size;
  extra->comp_size = comp_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->root = root_traced;

  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__,extra);

  void *recvbuf = smpi_get_tmp_sendbuffer(comm_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *sendbuf = smpi_get_tmp_sendbuffer(comm_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  mpi_coll_reduce_fun(sendbuf, recvbuf, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, root, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allReduce(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 1);
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);

  if(action[4]) MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  double clock = smpi_process_simulated_elapsed();
  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLREDUCE;
  extra->send_size = comm_size;
  extra->comp_size = comp_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  void *recvbuf = smpi_get_tmp_sendbuffer(comm_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *sendbuf = smpi_get_tmp_sendbuffer(comm_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  mpi_coll_allreduce_fun(sendbuf, recvbuf, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allToAll(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 2); //two mandatory (send and recv volumes)
                                     //two optional (corresponding datatypes)
  double clock = smpi_process_simulated_elapsed();
  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  int send_size = parse_double(action[2]);
  int recv_size = parse_double(action[3]);
  MPI_Datatype MPI_CURRENT_TYPE2;

  if(action[4] && action[5]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
    MPI_CURRENT_TYPE2=decode_datatype(action[5]);
  }
  else{
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;
    MPI_CURRENT_TYPE2=MPI_DEFAULT_TYPE;
  }

  void *send = smpi_get_tmp_sendbuffer(send_size*comm_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *recv = smpi_get_tmp_recvbuffer(recv_size*comm_size* smpi_datatype_size(MPI_CURRENT_TYPE2));

  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLTOALL;
  extra->send_size = send_size;
  extra->recv_size = recv_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, NULL);

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  mpi_coll_alltoall_fun(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_gather(const char *const *action) {
  /* The structure of the gather action for the rank 0 (total 4 processes) is the following:
 0 gather 68 68 0 0 0

  where: 
  1) 68 is the sendcounts
  2) 68 is the recvcounts
  3) 0 is the root node
  4) 0 is the send datatype id, see decode_datatype()
  5) 0 is the recv datatype id, see decode_datatype()
  */
  CHECK_ACTION_PARAMS(action, 2, 3);
  double clock = smpi_process_simulated_elapsed();
  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  int send_size = parse_double(action[2]);
  int recv_size = parse_double(action[3]);
  MPI_Datatype MPI_CURRENT_TYPE2;
  if(action[4] && action[5]) {
    MPI_CURRENT_TYPE=decode_datatype(action[5]);
    MPI_CURRENT_TYPE2=decode_datatype(action[6]);
  } else {
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;
    MPI_CURRENT_TYPE2=MPI_DEFAULT_TYPE;
  }
  void *send = smpi_get_tmp_sendbuffer(send_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *recv = NULL;
  int root=0;
  if(action[4])
    root=atoi(action[4]);
  int rank = smpi_comm_rank(MPI_COMM_WORLD);

  if(rank==root)
    recv = smpi_get_tmp_recvbuffer(recv_size*comm_size* smpi_datatype_size(MPI_CURRENT_TYPE2));

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_GATHER;
  extra->send_size = send_size;
  extra->recv_size = recv_size;
  extra->root = root;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, NULL);

  TRACE_smpi_collective_in(smpi_process_index(), root, __FUNCTION__, extra);

  mpi_coll_gather_fun(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(smpi_process_index(), -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_gatherv(const char *const *action) {
  /* The structure of the gatherv action for the rank 0 (total 4 processes) is the following:
 0 gather 68 68 10 10 10 0 0 0

  where:
  1) 68 is the sendcount
  2) 68 10 10 10 is the recvcounts
  3) 0 is the root node
  4) 0 is the send datatype id, see decode_datatype()
  5) 0 is the recv datatype id, see decode_datatype()
  */

  double clock = smpi_process_simulated_elapsed();
  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  CHECK_ACTION_PARAMS(action, comm_size+1, 2);
  int send_size = parse_double(action[2]);
  int *disps = xbt_new0(int, comm_size);
  int *recvcounts = xbt_new0(int, comm_size);
  int i=0,recv_sum=0;

  MPI_Datatype MPI_CURRENT_TYPE2;
  if(action[4+comm_size] && action[5+comm_size]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4+comm_size]);
    MPI_CURRENT_TYPE2=decode_datatype(action[5+comm_size]);
  } else {
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;
    MPI_CURRENT_TYPE2=MPI_DEFAULT_TYPE;
  }
  void *send = smpi_get_tmp_sendbuffer(send_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *recv = NULL;
  for(i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+3]);
    recv_sum=recv_sum+recvcounts[i];
    disps[i] = 0;
  }

  int root=atoi(action[3+comm_size]);
  int rank = smpi_comm_rank(MPI_COMM_WORLD);;

  if(rank==root)
    recv = smpi_get_tmp_recvbuffer(recv_sum* smpi_datatype_size(MPI_CURRENT_TYPE2));

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_GATHERV;
  extra->send_size = send_size;
  extra->recvcounts= xbt_new(int,comm_size);
  for(i=0; i< comm_size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i];
  extra->root = root;
  extra->num_processes = comm_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, NULL);

  TRACE_smpi_collective_in(smpi_process_index(), root, __FUNCTION__, extra);

  smpi_mpi_gatherv(send, send_size, MPI_CURRENT_TYPE, recv, recvcounts, disps, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(smpi_process_index(), -1, __FUNCTION__);
  log_timed_action (action, clock);
  xbt_free(recvcounts);
  xbt_free(disps);
}

static void action_reducescatter(const char *const *action) {
 /* The structure of the reducescatter action for the rank 0 (total 4 processes) is the following:
0 reduceScatter 275427 275427 275427 204020 11346849 0

  where: 
  1) The first four values after the name of the action declare the recvcounts array
  2) The value 11346849 is the amount of instructions
  3) The last value corresponds to the datatype, see decode_datatype().

  We analyze a MPI_Reduce_scatter call to one MPI_Reduce and one MPI_Scatterv. */
  double clock = smpi_process_simulated_elapsed();
  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  CHECK_ACTION_PARAMS(action, comm_size+1, 1);
  int comp_size = parse_double(action[2+comm_size]);
  int *recvcounts = xbt_new0(int, comm_size);  
  int *disps = xbt_new0(int, comm_size);  
  int i=0;
  int rank = smpi_process_index();
  int size = 0;
  if(action[3+comm_size])
    MPI_CURRENT_TYPE=decode_datatype(action[3+comm_size]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  for(i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+2]);
    disps[i] = 0;
    size+=recvcounts[i];
  }

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE_SCATTER;
  extra->send_size = 0;
  extra->recvcounts= xbt_new(int, comm_size);
  for(i=0; i< comm_size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i];
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->comp_size = comp_size;
  extra->num_processes = comm_size;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  void *sendbuf = smpi_get_tmp_sendbuffer(size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *recvbuf = smpi_get_tmp_recvbuffer(size* smpi_datatype_size(MPI_CURRENT_TYPE));
   
   mpi_coll_reduce_scatter_fun(sendbuf, recvbuf, recvcounts, MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
   smpi_execute_flops(comp_size);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  xbt_free(recvcounts);
  xbt_free(disps);
  log_timed_action (action, clock);
}

static void action_allgather(const char *const *action) {
  /* The structure of the allgather action for the rank 0 (total 4 processes) is the following:
  0 allGather 275427 275427

  where: 
  1) 275427 is the sendcount
  2) 275427 is the recvcount
  3) No more values mean that the datatype for sent and receive buffer is the default one, see decode_datatype().  */
  double clock = smpi_process_simulated_elapsed();

  CHECK_ACTION_PARAMS(action, 2, 2);
  int sendcount=atoi(action[2]); 
  int recvcount=atoi(action[3]); 

  MPI_Datatype MPI_CURRENT_TYPE2;

  if(action[4] && action[5]) {
    MPI_CURRENT_TYPE = decode_datatype(action[4]);
    MPI_CURRENT_TYPE2 = decode_datatype(action[5]);
  } else {
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;
    MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;
  }
  void *sendbuf = smpi_get_tmp_sendbuffer(sendcount* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *recvbuf = smpi_get_tmp_recvbuffer(recvcount* smpi_datatype_size(MPI_CURRENT_TYPE2));

  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLGATHER;
  extra->send_size = sendcount;
  extra->recv_size= recvcount;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, NULL);
  extra->num_processes = smpi_comm_size(MPI_COMM_WORLD);

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  mpi_coll_allgather_fun(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcount, MPI_CURRENT_TYPE2, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allgatherv(const char *const *action) {
  /* The structure of the allgatherv action for the rank 0 (total 4 processes) is the following:
0 allGatherV 275427 275427 275427 275427 204020

  where: 
  1) 275427 is the sendcount
  2) The next four elements declare the recvcounts array
  3) No more values mean that the datatype for sent and receive buffer
  is the default one, see decode_datatype(). */
  double clock = smpi_process_simulated_elapsed();

  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  CHECK_ACTION_PARAMS(action, comm_size+1, 2);
  int i=0;
  int sendcount=atoi(action[2]);
  int *recvcounts = xbt_new0(int, comm_size);  
  int *disps = xbt_new0(int, comm_size);  
  int recv_sum=0;  
  MPI_Datatype MPI_CURRENT_TYPE2;

  if(action[3+comm_size] && action[4+comm_size]) {
    MPI_CURRENT_TYPE = decode_datatype(action[3+comm_size]);
    MPI_CURRENT_TYPE2 = decode_datatype(action[4+comm_size]);
  } else {
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;
    MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;    
  }
  void *sendbuf = smpi_get_tmp_sendbuffer(sendcount* smpi_datatype_size(MPI_CURRENT_TYPE));

  for(i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+3]);
    recv_sum=recv_sum+recvcounts[i];
  }
  void *recvbuf = smpi_get_tmp_recvbuffer(recv_sum* smpi_datatype_size(MPI_CURRENT_TYPE2));

  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLGATHERV;
  extra->send_size = sendcount;
  extra->recvcounts= xbt_new(int, comm_size);
  for(i=0; i< comm_size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i];
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, NULL);
  extra->num_processes = comm_size;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  mpi_coll_allgatherv_fun(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcounts, disps, MPI_CURRENT_TYPE2,
                          MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
  xbt_free(recvcounts);
  xbt_free(disps);
}

static void action_allToAllv(const char *const *action) {
  /* The structure of the allToAllV action for the rank 0 (total 4 processes) is the following:
  0 allToAllV 100 1 7 10 12 100 1 70 10 5

  where: 
  1) 100 is the size of the send buffer *sizeof(int),
  2) 1 7 10 12 is the sendcounts array
  3) 100*sizeof(int) is the size of the receiver buffer
  4)  1 70 10 5 is the recvcounts array */
  double clock = smpi_process_simulated_elapsed();

  int comm_size = smpi_comm_size(MPI_COMM_WORLD);
  CHECK_ACTION_PARAMS(action, 2*comm_size+2, 2);
  int send_buf_size=0,recv_buf_size=0,i=0;
  int *sendcounts = xbt_new0(int, comm_size);  
  int *recvcounts = xbt_new0(int, comm_size);  
  int *senddisps = xbt_new0(int, comm_size);  
  int *recvdisps = xbt_new0(int, comm_size);  

  MPI_Datatype MPI_CURRENT_TYPE2;

  send_buf_size=parse_double(action[2]);
  recv_buf_size=parse_double(action[3+comm_size]);
  if(action[4+2*comm_size] && action[5+2*comm_size]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4+2*comm_size]);
    MPI_CURRENT_TYPE2=decode_datatype(action[5+2*comm_size]);
  }
  else{
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;
    MPI_CURRENT_TYPE2=MPI_DEFAULT_TYPE;
  }

  void *sendbuf = smpi_get_tmp_sendbuffer(send_buf_size* smpi_datatype_size(MPI_CURRENT_TYPE));
  void *recvbuf  = smpi_get_tmp_recvbuffer(recv_buf_size* smpi_datatype_size(MPI_CURRENT_TYPE2));

  for(i=0;i<comm_size;i++) {
    sendcounts[i] = atoi(action[i+3]);
    recvcounts[i] = atoi(action[i+4+comm_size]);
  }

  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLTOALLV;
  extra->recvcounts= xbt_new(int, comm_size);
  extra->sendcounts= xbt_new(int, comm_size);
  extra->num_processes = comm_size;

  for(i=0; i< comm_size; i++){//copy data to avoid bad free
    extra->send_size += sendcounts[i];
    extra->sendcounts[i] = sendcounts[i];
    extra->recv_size += recvcounts[i];
    extra->recvcounts[i] = recvcounts[i];
  }
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, NULL);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, NULL);

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  mpi_coll_alltoallv_fun(sendbuf, sendcounts, senddisps, MPI_CURRENT_TYPE,recvbuf, recvcounts, recvdisps,
                         MPI_CURRENT_TYPE, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
  xbt_free(sendcounts);
  xbt_free(recvcounts);
  xbt_free(senddisps);
  xbt_free(recvdisps);
}

void smpi_replay_run(int *argc, char***argv){
  /* First initializes everything */
  smpi_process_init(argc, argv);
  smpi_process_mark_as_initialized();
  smpi_process_set_replaying(1);

  int rank = smpi_process_index();
  TRACE_smpi_init(rank);
  TRACE_smpi_computing_init(rank);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_INIT;
  char *operation =bprintf("%s_init",__FUNCTION__);
  TRACE_smpi_collective_in(rank, -1, operation, extra);
  TRACE_smpi_collective_out(rank, -1, operation);
  free(operation);

  if (!_xbt_replay_action_init()) {
    xbt_replay_action_register("init",       action_init);
    xbt_replay_action_register("finalize",   action_finalize);
    xbt_replay_action_register("comm_size",  action_comm_size);
    xbt_replay_action_register("comm_split", action_comm_split);
    xbt_replay_action_register("comm_dup",   action_comm_dup);
    xbt_replay_action_register("send",       action_send);
    xbt_replay_action_register("Isend",      action_Isend);
    xbt_replay_action_register("recv",       action_recv);
    xbt_replay_action_register("Irecv",      action_Irecv);
    xbt_replay_action_register("test",       action_test);
    xbt_replay_action_register("wait",       action_wait);
    xbt_replay_action_register("waitAll",    action_waitall);
    xbt_replay_action_register("barrier",    action_barrier);
    xbt_replay_action_register("bcast",      action_bcast);
    xbt_replay_action_register("reduce",     action_reduce);
    xbt_replay_action_register("allReduce",  action_allReduce);
    xbt_replay_action_register("allToAll",   action_allToAll);
    xbt_replay_action_register("allToAllV",  action_allToAllv);
    xbt_replay_action_register("gather",  action_gather);
    xbt_replay_action_register("gatherV",  action_gatherv);
    xbt_replay_action_register("allGather",  action_allgather);
    xbt_replay_action_register("allGatherV",  action_allgatherv);
    xbt_replay_action_register("reduceScatter",  action_reducescatter);
    xbt_replay_action_register("compute",    action_compute);
  }

  //if we have a delayed start, sleep here.
  if(*argc>2){
    char *endptr;
    double value = strtod((*argv)[2], &endptr);
    if (*endptr != '\0')
      THROWF(unknown_error, 0, "%s is not a double", (*argv)[2]);
    XBT_VERB("Delayed start for instance - Sleeping for %f flops ",value );
    smpi_execute_flops(value);
  } else {
    //UGLY: force a context switch to be sure that all MSG_processes begin initialization
    XBT_DEBUG("Force context switch by smpi_execute_flops  - Sleeping for 0.0 flops ");
    smpi_execute_flops(0.0);
  }

  /* Actually run the replay */
  xbt_replay_action_runner(*argc, *argv);

  /* and now, finalize everything */
  double sim_time= 1.;
  /* One active process will stop. Decrease the counter*/
  XBT_DEBUG("There are %lu elements in reqq[*]", xbt_dynar_length(get_reqq_self()));
  if (!xbt_dynar_is_empty(get_reqq_self())){
    int count_requests=xbt_dynar_length(get_reqq_self());
    MPI_Request requests[count_requests];
    MPI_Status status[count_requests];
    unsigned int i;

    xbt_dynar_foreach(get_reqq_self(),i,requests[i]);
    smpi_mpi_waitall(count_requests, requests, status);
    active_processes--;
  } else {
    active_processes--;
  }

  if(!active_processes){
    /* Last process alive speaking */
    /* end the simulated timer */
    sim_time = smpi_process_simulated_elapsed();
  }

  //TODO xbt_dynar_free_container(get_reqq_self()));

  if(!active_processes){
    XBT_INFO("Simulation time %f", sim_time);
    _xbt_replay_action_exit();
    xbt_free(sendbuffer);
    xbt_free(recvbuffer);
    //xbt_free(reqq);
    xbt_dict_free(&reqq); //not need, data have been freed ???
    reqq = NULL;
  }

  instr_extra_data extra_fin = xbt_new0(s_instr_extra_data_t,1);
  extra_fin->type = TRACING_FINALIZE;
  operation =bprintf("%s_finalize",__FUNCTION__);
  TRACE_smpi_collective_in(rank, -1, operation, extra_fin);

  smpi_process_finalize();

  TRACE_smpi_collective_out(rank, -1, operation);
  TRACE_smpi_finalize(smpi_process_index());
  smpi_process_destroy();
  free(operation);
}
