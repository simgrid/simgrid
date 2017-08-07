/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_group.hpp"
#include "smpi_process.hpp"
#include "smpi_request.hpp"
#include "xbt/replay.hpp"

#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>

#include "src/simix/ActorImpl.hpp"

#define KEY_SIZE (sizeof(int) * 2 + 1)

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

int communicator_size = 0;
static int active_processes = 0;

#define REQ_KEY_SIZE 61
std::vector<std::unordered_map<std::string, MPI_Request>> reqd;

MPI_Datatype MPI_DEFAULT_TYPE;
MPI_Datatype MPI_CURRENT_TYPE;

static int sendbuffer_size=0;
char* sendbuffer=nullptr;
static int recvbuffer_size=0;
char* recvbuffer=nullptr;


/******************************************************************************
 * This code manages the request dictionaries which are used by the
 * assynchronous comunication actions (Isend, Irecv, wait, test).
 *****************************************************************************/

static std::string build_request_key(int src, int dst, int tag){
  std::stringstream key_stream;
  key_stream << src << dst << tag;
  std::string key = key_stream.str();
  return key;
}

static std::string key_from_request(MPI_Request request)
{
  std::string key = build_request_key(
      request->src(), request->dst(), request->tag());
  return key;
}

static void register_request(MPI_Request request)
{
  std::string key = key_from_request(request);
  reqd[smpi_process_index()].insert({key, request});
}

static void register_request_with_key(MPI_Request request, std::string key)
{
  reqd[smpi_process_index()].insert({key, request});
}

static MPI_Request get_request_with_key(std::string key){
  MPI_Request request = nullptr;
  try{
    request = reqd[smpi_process_index()].at(key);
  }
  catch(std::out_of_range){
    request = nullptr;
  }
  return request;
}

static MPI_Request get_request(int src, int dst, int tag)
{
  std::string key = build_request_key(src, dst, tag);
  MPI_Request request = get_request_with_key(key);
  return request;
}

static void remove_request_with_key(std::string key){
  reqd[smpi_process_index()].erase(key);
}

static void remove_request(MPI_Request request){
  std::string key = key_from_request(request);
  remove_request_with_key(key);
}


/******************* End of the request storage code. ************************/


static void log_timed_action (const char *const *action, double clock){
  if (XBT_LOG_ISENABLED(smpi_replay, xbt_log_priority_verbose)){
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, smpi_process()->simulated_elapsed() - clock);
    xbt_free(name);
  }
}

//allocate a single buffer for all sends, growing it if needed
void* smpi_get_tmp_sendbuffer(int size)
{
  if (not smpi_process()->replaying())
    return xbt_malloc(size);
  if (sendbuffer_size<size){
    sendbuffer=static_cast<char*>(xbt_realloc(sendbuffer,size));
    sendbuffer_size=size;
  }
  return sendbuffer;
}

//allocate a single buffer for all recv
void* smpi_get_tmp_recvbuffer(int size){
  if (not smpi_process()->replaying())
    return xbt_malloc(size);
  if (recvbuffer_size<size){
    recvbuffer=static_cast<char*>(xbt_realloc(recvbuffer,size));
    recvbuffer_size=size;
  }
  return recvbuffer;
}

void smpi_free_tmp_buffer(void* buf){
  if (not smpi_process()->replaying())
    xbt_free(buf);
}

/* Helper function */
static double parse_double(const char *string)
{
  char *endptr;
  double value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROWF(unknown_error, 0, "%s is not a double", string);
  return value;
}

static MPI_Datatype decode_datatype(const char *const action)
{
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
      break;
  }
   return MPI_CURRENT_TYPE;
}

const char* encode_datatype(MPI_Datatype datatype, int* known)
{
  //default type for output is set to MPI_BYTE
  // MPI_DEFAULT_TYPE is not set for output, use directly MPI_BYTE
  if(known!=nullptr)
    *known=1;
  if (datatype==MPI_BYTE)
      return "";
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
  if(known!=nullptr)
    *known=0;
  // default - not implemented.
  // do not warn here as we pass in this function even for other trace formats
  return "-1";
}


//TODO should we move this macro to a public header, so it can be used
//by actions implemented outside SimGrid?
#define CHECK_ACTION_PARAMS(action, mandatory, optional) {\
    int i=0;\
    while(action[i]!=nullptr)\
     i++;\
    if(i<mandatory+2)                                           \
    THROWF(arg_error, 0, "%s replay failed.\n" \
          "%d items were given on the line. First two should be process_id and action.  " \
          "This action needs after them %d mandatory arguments, and accepts %d optional ones. \n" \
          "Please contact the Simgrid team if support is needed", __FUNCTION__, i, mandatory, optional);\
  }


namespace simgrid {
namespace smpi {


static void action_init(const char *const *action)
{
  XBT_DEBUG("Initialize the counters");
  CHECK_ACTION_PARAMS(action, 0, 1)
  if(action[2])
    MPI_DEFAULT_TYPE=MPI_DOUBLE; // default MPE dataype
  else MPI_DEFAULT_TYPE= MPI_BYTE; // default TAU datatype

  /* start a simulated timer */
  smpi_process()->simulated_start();
  /*initialize the number of active processes */
  active_processes = smpi_process_count();
  if(reqd.empty()){
    for(int i = 0; i< active_processes; i++){ 
      std::unordered_map<std::string, MPI_Request> req_map;
      reqd.push_back(req_map);
    }
  }
}

static void action_finalize(const char *const *action)
{
  /* Nothing to do */
}

static void action_comm_size(const char *const *action)
{
  communicator_size = parse_double(action[2]);
  log_timed_action (action, smpi_process()->simulated_elapsed());
}

static void action_comm_split(const char *const *action)
{
  log_timed_action (action, smpi_process()->simulated_elapsed());
}

static void action_comm_dup(const char *const *action)
{
  log_timed_action (action, smpi_process()->simulated_elapsed());
}

static void action_compute(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 1, 1)
  double clock = smpi_process()->simulated_elapsed();
  double flops= parse_double(action[2]);
  double time_comp = action[3] ? parse_double(action[3]) : 0;
  
  int rank = smpi_process()->index();
  
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type=TRACING_COMPUTING;
  if(action[3]){
   extra->comp_size = time_comp; 
  }else{
    extra->comp_size = flops;
  }
  TRACE_smpi_computing_in(rank, extra);
  if(action[3]){
    smpi_execute_flops(time_comp);
  }else{
    smpi_execute_flops(flops);
  }
  TRACE_smpi_computing_out(rank);
  log_timed_action (action, clock);
}

static void action_send(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 3, 1);
  int to = atoi(action[2]);
  int tag = atoi(action[3]);
  double size=parse_double(action[4]);
  double clock = smpi_process()->simulated_elapsed();

  if(action[5]) {
    MPI_CURRENT_TYPE=decode_datatype(action[5]);
  } else {
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;
  }

  int rank = smpi_process()->index();

  int dst_traced = MPI_COMM_WORLD->group()->rank(to);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SEND;
  extra->send_size = size;
  extra->src = rank;
  extra->dst = dst_traced;
  extra->tag = tag;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
  if (not TRACE_smpi_view_internals())
    TRACE_smpi_send(rank, rank, dst_traced, tag, size*MPI_CURRENT_TYPE->size());

  Request::send(nullptr, size, MPI_CURRENT_TYPE, to , tag, MPI_COMM_WORLD);

  log_timed_action (action, clock);

  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
}

static void action_Isend(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 3, 1);
  int to = atoi(action[2]);
  int tag = atoi(action[3]);
  double size=parse_double(action[4]);
  double clock = smpi_process()->simulated_elapsed();

  if(action[5]) 
    MPI_CURRENT_TYPE=decode_datatype(action[5]);
  else 
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int dst_traced = MPI_COMM_WORLD->group()->rank(to);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ISEND;
  extra->send_size = size;
  extra->src = rank;
  extra->dst = dst_traced;
  extra->tag = tag;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
  if (not TRACE_smpi_view_internals())
    TRACE_smpi_send(rank, rank, dst_traced, tag, size*MPI_CURRENT_TYPE->size());

  MPI_Request request = Request::isend(nullptr, size, MPI_CURRENT_TYPE, to, tag, MPI_COMM_WORLD);

  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);

  register_request(request);

  log_timed_action (action, clock);
}

static void action_recv(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 3, 1);
  int from = atoi(action[2]);
  int tag = atoi(action[3]);
  double size=parse_double(action[4]);
  double clock = smpi_process()->simulated_elapsed();
  MPI_Status status;

  if(action[5])
    MPI_CURRENT_TYPE = decode_datatype(action[5]);
  else
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int src_traced = MPI_COMM_WORLD->group()->rank(from);

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_RECV;
  extra->send_size = size;
  extra->src = src_traced;
  extra->dst = rank;
  extra->tag = tag;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

  //unknown size from the receiver point of view
  if(size<=0.0){
    Request::probe(from, tag, MPI_COMM_WORLD, &status);
    size=status.count;
  }

  Request::recv(nullptr, size, MPI_CURRENT_TYPE, from, tag, MPI_COMM_WORLD, &status);

  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  if (not TRACE_smpi_view_internals()) {
    TRACE_smpi_recv(rank, src_traced, rank, tag);
  }

  log_timed_action (action, clock);
}

static void action_Irecv(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 3, 1);
  int from = atoi(action[2]);
  int tag = atoi(action[3]);
  double size=parse_double(action[4]);
  double clock = smpi_process()->simulated_elapsed();

  if(action[5])
    MPI_CURRENT_TYPE=decode_datatype(action[5]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int src_traced = MPI_COMM_WORLD->group()->rank(from);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_IRECV;
  extra->send_size = size;
  extra->src = src_traced;
  extra->dst = rank;
  extra->tag = tag;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);
  MPI_Status status;
  //unknow size from the receiver pov
  if(size<=0.0){
      Request::probe(from, tag, MPI_COMM_WORLD, &status);
      size=status.count;
  }

  MPI_Request request = Request::irecv(nullptr, size, MPI_CURRENT_TYPE, from, tag, MPI_COMM_WORLD);

  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  register_request(request);

  log_timed_action (action, clock);
}

static void action_test(const char *const *action){
  CHECK_ACTION_PARAMS(action, 3, 0);
  int src = atoi(action[2]);
  int dst = atoi(action[3]);
  int tag = atoi(action[4]);
  double clock = smpi_process()->simulated_elapsed();
  MPI_Request request;
  MPI_Status status;

  std::string key = build_request_key(src, dst, tag);
  request = get_request_with_key(key);
  remove_request_with_key(key);

  //if request is null here, this may mean that a previous test has succeeded 
  //Different times in traced application and replayed version may lead to this 
  //In this case, ignore the extra calls.
  if(request!=nullptr){
    int rank = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type=TRACING_TEST;
    TRACE_smpi_testing_in(rank, extra);

    int flag = Request::test(&request, &status);

    XBT_DEBUG("MPI_Test result: %d", flag);

    register_request_with_key(request, key);
  
    TRACE_smpi_testing_out(rank);
  }
  log_timed_action (action, clock);
}

static void action_wait(const char *const *action){
  CHECK_ACTION_PARAMS(action, 3, 0);
  int src = atoi(action[2]);
  int dst = atoi(action[3]);
  int tag = atoi(action[4]);
  double clock = smpi_process()->simulated_elapsed();
  std::string key = build_request_key(src, dst, tag);
  MPI_Request request = get_request_with_key(key);
  MPI_Status status;

  xbt_assert(!reqd[smpi_process_index()].empty(),
    "action wait not preceded by any irecv or isend: %s",
    xbt_str_join_array(action," "));
  remove_request_with_key(key);
  
  if (request==nullptr){
    /* Assume that the trace is well formed, meaning the comm might have been caught by a MPI_test. Then just return.*/
    return;
  }

  int rank = request->comm() != MPI_COMM_NULL ? request->comm()->rank() : -1;

  MPI_Group group = request->comm()->group();
  int src_traced = group->rank(request->src());
  int dst_traced = group->rank(request->dst());
  int is_wait_for_receive = (request->flags() & RECV);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAIT;
  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);

  Request::wait(&request, &status);

  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  if (is_wait_for_receive)
    TRACE_smpi_recv(rank, src_traced, dst_traced, 0);
  log_timed_action (action, clock);
}

static void action_waitall(const char *const *action){
  CHECK_ACTION_PARAMS(action, 0, 0);
  double clock = smpi_process()->simulated_elapsed();
  unsigned int count_requests = reqd[smpi_process_index()].size();
  unsigned int i = 0;

  if (count_requests > 0) {
    MPI_Request requests[count_requests];
    MPI_Status status[count_requests];
    int my_id = smpi_process_index();
    for(auto it = reqd[my_id].begin(); it != reqd[my_id].end(); it++){
      requests[i] = it->second;
      i++;
    } 
   
    int rank_traced = smpi_process_index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_WAITALL;
    extra->send_size=count_requests;
    TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

   
    Request::waitall(count_requests, requests, status);

    for(auto it = reqd[my_id].begin(); it != reqd[my_id].end(); it++){
      MPI_Request req = it->second;
      if(req->flags() & RECV){
        TRACE_smpi_recv(rank_traced, req->src(), req->dst(), req->tag());
      }
    }

    TRACE_smpi_ptp_out(rank_traced, -1, -1, __FUNCTION__);
   
    reqd[smpi_process_index()].clear();

  }

  log_timed_action (action, clock);
}

static void action_barrier(const char *const *action){
  double clock = smpi_process()->simulated_elapsed();
  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_BARRIER;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  Colls::barrier(MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_bcast(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 1, 2)
  double size = parse_double(action[2]);
  double clock = smpi_process()->simulated_elapsed();
  int root=0;
  /* Initialize MPI_CURRENT_TYPE in order to decrease the number of the checks */
  MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  if(action[3]) {
    root= atoi(action[3]);
    if(action[4])
      MPI_CURRENT_TYPE=decode_datatype(action[4]);
  }

  int rank = smpi_process()->index();
  int root_traced = MPI_COMM_WORLD->group()->index(root);

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_BCAST;
  extra->send_size = size;
  extra->root = root_traced;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);
  void *sendbuf = smpi_get_tmp_sendbuffer(size* MPI_CURRENT_TYPE->size());

  Colls::bcast(sendbuf, size, MPI_CURRENT_TYPE, root, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_reduce(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 2)
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);
  double clock = smpi_process()->simulated_elapsed();
  int root=0;
  MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  if(action[4]) {
    root= atoi(action[4]);
    if(action[5])
      MPI_CURRENT_TYPE=decode_datatype(action[5]);
  }

  int rank = smpi_process()->index();
  int root_traced = MPI_COMM_WORLD->group()->rank(root);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE;
  extra->send_size = comm_size;
  extra->comp_size = comp_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->root = root_traced;

  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__,extra);

  void *recvbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  void *sendbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  Colls::reduce(sendbuf, recvbuf, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, root, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allReduce(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 1);
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);

  if(action[4])
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  double clock = smpi_process()->simulated_elapsed();
  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLREDUCE;
  extra->send_size = comm_size;
  extra->comp_size = comp_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  void *recvbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  void *sendbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  Colls::allreduce(sendbuf, recvbuf, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allToAll(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 2) //two mandatory (send and recv volumes) and two optional (corresponding datatypes)
  double clock = smpi_process()->simulated_elapsed();
  int comm_size = MPI_COMM_WORLD->size();
  int send_size = parse_double(action[2]);
  int recv_size = parse_double(action[3]);
  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;

  if(action[4] && action[5]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
    MPI_CURRENT_TYPE2=decode_datatype(action[5]);
  }
  else
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;

  void *send = smpi_get_tmp_sendbuffer(send_size*comm_size* MPI_CURRENT_TYPE->size());
  void *recv = smpi_get_tmp_recvbuffer(recv_size*comm_size* MPI_CURRENT_TYPE2->size());

  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLTOALL;
  extra->send_size = send_size;
  extra->recv_size = recv_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, nullptr);

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  Colls::alltoall(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, MPI_COMM_WORLD);

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
  CHECK_ACTION_PARAMS(action, 2, 3)
  double clock = smpi_process()->simulated_elapsed();
  int comm_size = MPI_COMM_WORLD->size();
  int send_size = parse_double(action[2]);
  int recv_size = parse_double(action[3]);
  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;
  if(action[4] && action[5]) {
    MPI_CURRENT_TYPE=decode_datatype(action[5]);
    MPI_CURRENT_TYPE2=decode_datatype(action[6]);
  } else {
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;
  }
  void *send = smpi_get_tmp_sendbuffer(send_size* MPI_CURRENT_TYPE->size());
  void *recv = nullptr;
  int root=0;
  if(action[4])
    root=atoi(action[4]);
  int rank = MPI_COMM_WORLD->rank();

  if(rank==root)
    recv = smpi_get_tmp_recvbuffer(recv_size*comm_size* MPI_CURRENT_TYPE2->size());

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_GATHER;
  extra->send_size = send_size;
  extra->recv_size = recv_size;
  extra->root = root;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, nullptr);

  TRACE_smpi_collective_in(smpi_process()->index(), root, __FUNCTION__, extra);

  Colls::gather(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(smpi_process()->index(), -1, __FUNCTION__);
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
  double clock = smpi_process()->simulated_elapsed();
  int comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size+1, 2)
  int send_size = parse_double(action[2]);
  int disps[comm_size];
  int recvcounts[comm_size];
  int recv_sum=0;

  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;
  if(action[4+comm_size] && action[5+comm_size]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4+comm_size]);
    MPI_CURRENT_TYPE2=decode_datatype(action[5+comm_size]);
  } else
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;

  void *send = smpi_get_tmp_sendbuffer(send_size* MPI_CURRENT_TYPE->size());
  void *recv = nullptr;
  for(int i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+3]);
    recv_sum=recv_sum+recvcounts[i];
    disps[i]=0;
  }

  int root=atoi(action[3+comm_size]);
  int rank = MPI_COMM_WORLD->rank();

  if(rank==root)
    recv = smpi_get_tmp_recvbuffer(recv_sum* MPI_CURRENT_TYPE2->size());

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_GATHERV;
  extra->send_size = send_size;
  extra->recvcounts= xbt_new(int,comm_size);
  for(int i=0; i< comm_size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i];
  extra->root = root;
  extra->num_processes = comm_size;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, nullptr);

  TRACE_smpi_collective_in(smpi_process()->index(), root, __FUNCTION__, extra);

  Colls::gatherv(send, send_size, MPI_CURRENT_TYPE, recv, recvcounts, disps, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(smpi_process()->index(), -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_reducescatter(const char *const *action) {
 /* The structure of the reducescatter action for the rank 0 (total 4 processes) is the following:
      0 reduceScatter 275427 275427 275427 204020 11346849 0
    where:
      1) The first four values after the name of the action declare the recvcounts array
      2) The value 11346849 is the amount of instructions
      3) The last value corresponds to the datatype, see decode_datatype().
*/
  double clock = smpi_process()->simulated_elapsed();
  int comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size+1, 1)
  int comp_size = parse_double(action[2+comm_size]);
  int recvcounts[comm_size];
  int rank = smpi_process()->index();
  int size = 0;
  if(action[3+comm_size])
    MPI_CURRENT_TYPE=decode_datatype(action[3+comm_size]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  for(int i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+2]);
    size+=recvcounts[i];
  }

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE_SCATTER;
  extra->send_size = 0;
  extra->recvcounts= xbt_new(int, comm_size);
  for(int i=0; i< comm_size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i];
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->comp_size = comp_size;
  extra->num_processes = comm_size;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  void *sendbuf = smpi_get_tmp_sendbuffer(size* MPI_CURRENT_TYPE->size());
  void *recvbuf = smpi_get_tmp_recvbuffer(size* MPI_CURRENT_TYPE->size());

  Colls::reduce_scatter(sendbuf, recvbuf, recvcounts, MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allgather(const char *const *action) {
  /* The structure of the allgather action for the rank 0 (total 4 processes) is the following:
        0 allGather 275427 275427
    where:
        1) 275427 is the sendcount
        2) 275427 is the recvcount
        3) No more values mean that the datatype for sent and receive buffer is the default one, see decode_datatype().
  */
  double clock = smpi_process()->simulated_elapsed();

  CHECK_ACTION_PARAMS(action, 2, 2)
  int sendcount=atoi(action[2]);
  int recvcount=atoi(action[3]);

  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;

  if(action[4] && action[5]) {
    MPI_CURRENT_TYPE = decode_datatype(action[4]);
    MPI_CURRENT_TYPE2 = decode_datatype(action[5]);
  } else
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;

  void *sendbuf = smpi_get_tmp_sendbuffer(sendcount* MPI_CURRENT_TYPE->size());
  void *recvbuf = smpi_get_tmp_recvbuffer(recvcount* MPI_CURRENT_TYPE2->size());

  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLGATHER;
  extra->send_size = sendcount;
  extra->recv_size= recvcount;
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, nullptr);
  extra->num_processes = MPI_COMM_WORLD->size();

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  Colls::allgather(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcount, MPI_CURRENT_TYPE2, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allgatherv(const char *const *action) {
  /* The structure of the allgatherv action for the rank 0 (total 4 processes) is the following:
        0 allGatherV 275427 275427 275427 275427 204020
     where:
        1) 275427 is the sendcount
        2) The next four elements declare the recvcounts array
        3) No more values mean that the datatype for sent and receive buffer is the default one, see decode_datatype().
  */
  double clock = smpi_process()->simulated_elapsed();

  int comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size+1, 2)
  int sendcount=atoi(action[2]);
  int recvcounts[comm_size];
  int disps[comm_size];
  int recv_sum=0;
  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;

  if(action[3+comm_size] && action[4+comm_size]) {
    MPI_CURRENT_TYPE = decode_datatype(action[3+comm_size]);
    MPI_CURRENT_TYPE2 = decode_datatype(action[4+comm_size]);
  } else
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;

  void *sendbuf = smpi_get_tmp_sendbuffer(sendcount* MPI_CURRENT_TYPE->size());

  for(int i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+3]);
    recv_sum=recv_sum+recvcounts[i];
    disps[i] = 0;
  }
  void *recvbuf = smpi_get_tmp_recvbuffer(recv_sum* MPI_CURRENT_TYPE2->size());

  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLGATHERV;
  extra->send_size = sendcount;
  extra->recvcounts= xbt_new(int, comm_size);
  for(int i=0; i< comm_size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i];
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, nullptr);
  extra->num_processes = comm_size;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  Colls::allgatherv(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcounts, disps, MPI_CURRENT_TYPE2,
                          MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}

static void action_allToAllv(const char *const *action) {
  /* The structure of the allToAllV action for the rank 0 (total 4 processes) is the following:
        0 allToAllV 100 1 7 10 12 100 1 70 10 5
     where:
        1) 100 is the size of the send buffer *sizeof(int),
        2) 1 7 10 12 is the sendcounts array
        3) 100*sizeof(int) is the size of the receiver buffer
        4)  1 70 10 5 is the recvcounts array
  */
  double clock = smpi_process()->simulated_elapsed();

  int comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, 2*comm_size+2, 2)
  int sendcounts[comm_size];
  int recvcounts[comm_size];
  int senddisps[comm_size];
  int recvdisps[comm_size];

  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;

  int send_buf_size=parse_double(action[2]);
  int recv_buf_size=parse_double(action[3+comm_size]);
  if(action[4+2*comm_size] && action[5+2*comm_size]) {
    MPI_CURRENT_TYPE=decode_datatype(action[4+2*comm_size]);
    MPI_CURRENT_TYPE2=decode_datatype(action[5+2*comm_size]);
  }
  else
    MPI_CURRENT_TYPE=MPI_DEFAULT_TYPE;

  void *sendbuf = smpi_get_tmp_sendbuffer(send_buf_size* MPI_CURRENT_TYPE->size());
  void *recvbuf  = smpi_get_tmp_recvbuffer(recv_buf_size* MPI_CURRENT_TYPE2->size());

  for(int i=0;i<comm_size;i++) {
    sendcounts[i] = atoi(action[i+3]);
    recvcounts[i] = atoi(action[i+4+comm_size]);
    senddisps[i] = 0;
    recvdisps[i] = 0;
  }

  int rank = smpi_process()->index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLTOALLV;
  extra->recvcounts= xbt_new(int, comm_size);
  extra->sendcounts= xbt_new(int, comm_size);
  extra->num_processes = comm_size;

  for(int i=0; i< comm_size; i++){//copy data to avoid bad free
    extra->send_size += sendcounts[i];
    extra->sendcounts[i] = sendcounts[i];
    extra->recv_size += recvcounts[i];
    extra->recvcounts[i] = recvcounts[i];
  }
  extra->datatype1 = encode_datatype(MPI_CURRENT_TYPE, nullptr);
  extra->datatype2 = encode_datatype(MPI_CURRENT_TYPE2, nullptr);

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  Colls::alltoallv(sendbuf, sendcounts, senddisps, MPI_CURRENT_TYPE,recvbuf, recvcounts, recvdisps,
                         MPI_CURRENT_TYPE, MPI_COMM_WORLD);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  log_timed_action (action, clock);
}


/*In previous versions, we called smpi_send_process_data (implemented on
 * smpi_base.c). The problem is that that function needs to have at least one
 * process in the destination host. This is not always true when using a
 * centralized LB approach. This alternate implementation is more flexible,
 * since we don't need to have processes in the receiving host.*/
void smpi_replay_send_process_data(double data_size, sg_host_t host)
{
  smx_activity_t action;
  sg_host_t host_list[2];
  double comp_amount[2];
  double comm_amount[4];
  
  TRACE_smpi_send_process_data_in(smpi_process_index());

  host_list[0] = sg_host_self();
  host_list[1] = host;
  comp_amount[0] = 0;
  comp_amount[1] = 0;
  comm_amount[0] = data_size;
  comm_amount[1] = 0;
  comm_amount[2] = 0;
  comm_amount[3] = 0;
  action = simcall_execution_parallel_start("data_migration", 2, host_list, 
	    comp_amount, comm_amount, -1.0, 0);
  simcall_execution_wait(action);

  TRACE_smpi_send_process_data_out(smpi_process_index());
}


/* Based on MSG_process_migrate [src/msg/msg_process.c] */
void smpi_replay_process_migrate(smx_actor_t process, sg_host_t new_host,
    unsigned long size)
{
 
  /* The data migration can't be done in this function, because it
   * needs to be done in parallel. As this function needs to be called in a
   * critical region (e.g. protected by a mutex), these (synchronous) data
   * migrations would become sequential.*/


  //Update the traces to reflect the migration.
  sg_host_t host = sg_host_self();
  TRACE_smpi_process_change_host(smpi_process_index(), host, new_host, size);
  
  // Now, we change the host to which this rank is mapped.
  process->iface()->migrate(new_host);
}

}} // namespace simgrid::smpi

/** @brief Only initialize the replay, don't do it for real */
void smpi_replay_init(int* argc, char*** argv)
{
  simgrid::smpi::Process::init(argc, argv);
  smpi_process()->mark_as_initialized();
  smpi_process()->set_replaying(true);

  int rank = smpi_process()->index();
  TRACE_smpi_init(rank);
  TRACE_smpi_computing_init(rank);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_INIT;
  TRACE_smpi_collective_in(rank, -1, "smpi_replay_run_init", extra);
  TRACE_smpi_collective_out(rank, -1, "smpi_replay_run_init");
  
  xbt_replay_action_register("init",       simgrid::smpi::action_init);
  xbt_replay_action_register("finalize",   simgrid::smpi::action_finalize);
  xbt_replay_action_register("comm_size",  simgrid::smpi::action_comm_size);
  xbt_replay_action_register("comm_split", simgrid::smpi::action_comm_split);
  xbt_replay_action_register("comm_dup",   simgrid::smpi::action_comm_dup);
  xbt_replay_action_register("send",       simgrid::smpi::action_send);
  xbt_replay_action_register("Isend",      simgrid::smpi::action_Isend);
  xbt_replay_action_register("recv",       simgrid::smpi::action_recv);
  xbt_replay_action_register("Irecv",      simgrid::smpi::action_Irecv);
  xbt_replay_action_register("test",       simgrid::smpi::action_test);
  xbt_replay_action_register("wait",       simgrid::smpi::action_wait);
  xbt_replay_action_register("waitAll",    simgrid::smpi::action_waitall);
  xbt_replay_action_register("barrier",    simgrid::smpi::action_barrier);
  xbt_replay_action_register("bcast",      simgrid::smpi::action_bcast);
  xbt_replay_action_register("reduce",     simgrid::smpi::action_reduce);
  xbt_replay_action_register("allReduce",  simgrid::smpi::action_allReduce);
  xbt_replay_action_register("allToAll",   simgrid::smpi::action_allToAll);
  xbt_replay_action_register("allToAllV",  simgrid::smpi::action_allToAllv);
  xbt_replay_action_register("gather",     simgrid::smpi::action_gather);
  xbt_replay_action_register("gatherV",    simgrid::smpi::action_gatherv);
  xbt_replay_action_register("allGather",  simgrid::smpi::action_allgather);
  xbt_replay_action_register("allGatherV", simgrid::smpi::action_allgatherv);
  xbt_replay_action_register("reduceScatter",  simgrid::smpi::action_reducescatter);
  xbt_replay_action_register("compute",    simgrid::smpi::action_compute);

  //if we have a delayed start, sleep here.
  if(*argc>2){
    double value = xbt_str_parse_double((*argv)[2], "%s is not a double");
    XBT_VERB("Delayed start for instance - Sleeping for %f flops ",value );
    smpi_execute_flops(value);
  } else {
    //UGLY: force a context switch to be sure that all MSG_processes begin initialization
    XBT_DEBUG("Force context switch by smpi_execute_flops  - Sleeping for 0.0 flops ");
    smpi_execute_flops(0.0);
  }
}

/** @brief actually run the replay after initialization */
void smpi_replay_main(int* argc, char*** argv)
{
  simgrid::xbt::replay_runner(*argc, *argv);

  /* and now, finalize everything */
  /* One active process will stop. Decrease the counter*/
  
  XBT_DEBUG("There are %lu elements in reqd[*]",
            reqd[smpi_process_index()].size());
  if(!reqd[smpi_process_index()].empty()){
    int count_requests=reqd[smpi_process_index()].size();

    MPI_Request requests[count_requests];
    MPI_Status status[count_requests];
    unsigned int i = 0;
    int my_id = smpi_process_index();
    for(auto it = reqd[my_id].begin(); it != reqd[my_id].end(); it++){
      requests[i] = it->second;
      i++;
    }
    simgrid::smpi::Request::waitall(count_requests, requests, status);
  }

  active_processes--;
  
  if(active_processes==0){
    /* Last process alive speaking: end the simulated timer */
    XBT_INFO("Simulation time %f", smpi_process()->simulated_elapsed());
    xbt_free(sendbuffer);
    xbt_free(recvbuffer);
  }
  
  instr_extra_data extra_fin = xbt_new0(s_instr_extra_data_t,1);
  extra_fin->type = TRACING_FINALIZE;
  TRACE_smpi_collective_in(smpi_process()->index(), -1, "smpi_replay_run_finalize", extra_fin);

  smpi_process()->finalize();

  TRACE_smpi_collective_out(smpi_process()->index(), -1, "smpi_replay_run_finalize");
  TRACE_smpi_finalize(smpi_process()->index());
}

/** @brief chain a replay initialization and a replay start */
void smpi_replay_run(int* argc, char*** argv)
{
  smpi_replay_init(argc, argv);
  smpi_replay_main(argc, argv);
}

