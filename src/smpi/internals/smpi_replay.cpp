/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_group.hpp"
#include "smpi_process.hpp"
#include "smpi_request.hpp"
#include "xbt/replay.hpp"

#include <unordered_map>
#include <vector>

#define KEY_SIZE (sizeof(int) * 2 + 1)

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

int communicator_size = 0;
static int active_processes = 0;
std::unordered_map<int,std::vector<MPI_Request>*> reqq;

MPI_Datatype MPI_DEFAULT_TYPE;
MPI_Datatype MPI_CURRENT_TYPE;

static int sendbuffer_size=0;
char* sendbuffer=nullptr;
static int recvbuffer_size=0;
char* recvbuffer=nullptr;

static void log_timed_action (const char *const *action, double clock){
  if (XBT_LOG_ISENABLED(smpi_replay, xbt_log_priority_verbose)){
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, smpi_process()->simulated_elapsed()-clock);
    xbt_free(name);
  }
}

static std::vector<MPI_Request>* get_reqq_self()
{
  return reqq.at(smpi_process()->index());
}

static void set_reqq_self(std::vector<MPI_Request> *mpi_request)
{
   reqq.insert({smpi_process()->index(), mpi_request});
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


//TODO: this logic should be moved inside the datatype class, to support all predefined types and get rid of is_replayable.
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

const char* encode_datatype(MPI_Datatype datatype)
{
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
  // default - not implemented.
  // do not warn here as we pass in this function even for other trace formats
  return "-1";
}

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
    MPI_DEFAULT_TYPE = MPI_DOUBLE; // default MPE datatype
  else
    MPI_DEFAULT_TYPE = MPI_BYTE; // default TAU datatype

  /* start a simulated timer */
  smpi_process()->simulated_start();
  /*initialize the number of active processes */
  active_processes = smpi_process_count();

  set_reqq_self(new std::vector<MPI_Request>);
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
  CHECK_ACTION_PARAMS(action, 1, 0)
  double clock = smpi_process()->simulated_elapsed();
  double flops= parse_double(action[2]);
  int rank = smpi_process()->index();

  TRACE_smpi_computing_in(rank, flops);
  smpi_execute_flops(flops);
  TRACE_smpi_computing_out(rank);

  log_timed_action (action, clock);
}

static void action_send(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 1)
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process()->simulated_elapsed();

  if(action[4])
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int dst_traced = MPI_COMM_WORLD->group()->rank(to);

  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::Pt2PtTIData("send", dst_traced, size, encode_datatype(MPI_CURRENT_TYPE)));
  if (not TRACE_smpi_view_internals())
    TRACE_smpi_send(rank, rank, dst_traced, 0, size*MPI_CURRENT_TYPE->size());

  Request::send(nullptr, size, MPI_CURRENT_TYPE, to , 0, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);

  log_timed_action(action, clock);
}

static void action_Isend(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 1)
  int to = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process()->simulated_elapsed();

  if(action[4])
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int dst_traced = MPI_COMM_WORLD->group()->rank(to);
  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::Pt2PtTIData("Isend", dst_traced, size, encode_datatype(MPI_CURRENT_TYPE)));
  if (not TRACE_smpi_view_internals())
    TRACE_smpi_send(rank, rank, dst_traced, 0, size*MPI_CURRENT_TYPE->size());

  MPI_Request request = Request::isend(nullptr, size, MPI_CURRENT_TYPE, to, 0, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);

  get_reqq_self()->push_back(request);

  log_timed_action (action, clock);
}

static void action_recv(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 1)
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process()->simulated_elapsed();
  MPI_Status status;

  if(action[4])
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int src_traced = MPI_COMM_WORLD->group()->rank(from);

  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::Pt2PtTIData("recv", src_traced, size, encode_datatype(MPI_CURRENT_TYPE)));

  //unknown size from the receiver point of view
  if (size <= 0.0) {
    Request::probe(from, 0, MPI_COMM_WORLD, &status);
    size=status.count;
  }

  Request::recv(nullptr, size, MPI_CURRENT_TYPE, from, 0, MPI_COMM_WORLD, &status);

  TRACE_smpi_comm_out(rank);
  if (not TRACE_smpi_view_internals()) {
    TRACE_smpi_recv(src_traced, rank, 0);
  }

  log_timed_action (action, clock);
}

static void action_Irecv(const char *const *action)
{
  CHECK_ACTION_PARAMS(action, 2, 1)
  int from = atoi(action[2]);
  double size=parse_double(action[3]);
  double clock = smpi_process()->simulated_elapsed();

  if(action[4])
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  int rank = smpi_process()->index();
  int src_traced = MPI_COMM_WORLD->group()->rank(from);
  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::Pt2PtTIData("Irecv", src_traced, size, encode_datatype(MPI_CURRENT_TYPE)));
  MPI_Status status;
  //unknow size from the receiver pov
  if (size <= 0.0) {
    Request::probe(from, 0, MPI_COMM_WORLD, &status);
    size = status.count;
  }

  MPI_Request request = Request::irecv(nullptr, size, MPI_CURRENT_TYPE, from, 0, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
  get_reqq_self()->push_back(request);

  log_timed_action (action, clock);
}

static void action_test(const char* const* action)
{
  CHECK_ACTION_PARAMS(action, 0, 0)
  double clock = smpi_process()->simulated_elapsed();
  MPI_Status status;

  MPI_Request request = get_reqq_self()->back();
  get_reqq_self()->pop_back();
  //if request is null here, this may mean that a previous test has succeeded
  //Different times in traced application and replayed version may lead to this
  //In this case, ignore the extra calls.
  if(request!=nullptr){
    int rank = smpi_process()->index();
    TRACE_smpi_testing_in(rank);

    int flag = Request::test(&request, &status);

    XBT_DEBUG("MPI_Test result: %d", flag);
    /* push back request in vector to be caught by a subsequent wait. if the test did succeed, the request is now nullptr.*/
    get_reqq_self()->push_back(request);

    TRACE_smpi_testing_out(rank);
  }
  log_timed_action (action, clock);
}

static void action_wait(const char *const *action){
  CHECK_ACTION_PARAMS(action, 0, 0)
  double clock = smpi_process()->simulated_elapsed();
  MPI_Status status;

  xbt_assert(get_reqq_self()->size(), "action wait not preceded by any irecv or isend: %s",
      xbt_str_join_array(action," "));
  MPI_Request request = get_reqq_self()->back();
  get_reqq_self()->pop_back();

  if (request==nullptr){
    /* Assume that the trace is well formed, meaning the comm might have been caught by a MPI_test. Then just return.*/
    return;
  }

  int rank = request->comm() != MPI_COMM_NULL ? request->comm()->rank() : -1;

  MPI_Group group = request->comm()->group();
  int src_traced = group->rank(request->src());
  int dst_traced = group->rank(request->dst());
  int is_wait_for_receive = (request->flags() & RECV);
  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::NoOpTIData("wait"));

  Request::wait(&request, &status);

  TRACE_smpi_comm_out(rank);
  if (is_wait_for_receive)
    TRACE_smpi_recv(src_traced, dst_traced, 0);
  log_timed_action (action, clock);
}

static void action_waitall(const char *const *action){
  CHECK_ACTION_PARAMS(action, 0, 0)
  double clock = smpi_process()->simulated_elapsed();
  const unsigned int count_requests = get_reqq_self()->size();

  if (count_requests>0) {
    MPI_Status status[count_requests];

   int rank_traced = smpi_process()->index();
   TRACE_smpi_comm_in(rank_traced, __FUNCTION__, new simgrid::instr::Pt2PtTIData("waitAll", -1, count_requests, ""));
   int recvs_snd[count_requests];
   int recvs_rcv[count_requests];
   for (unsigned int i = 0; i < count_requests; i++) {
     const auto& req = (*get_reqq_self())[i];
     if (req && (req->flags () & RECV)){
       recvs_snd[i]=req->src();
       recvs_rcv[i]=req->dst();
     }else
       recvs_snd[i]=-100;
   }
   Request::waitall(count_requests, &(*get_reqq_self())[0], status);

   for (unsigned i = 0; i < count_requests; i++) {
     if (recvs_snd[i]!=-100)
       TRACE_smpi_recv(recvs_snd[i], recvs_rcv[i],0);
   }
   TRACE_smpi_comm_out(rank_traced);
  }
  log_timed_action (action, clock);
}

static void action_barrier(const char *const *action){
  double clock = smpi_process()->simulated_elapsed();
  int rank = smpi_process()->index();
  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::NoOpTIData("barrier"));

  Colls::barrier(MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
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
  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::CollTIData("bcast", MPI_COMM_WORLD->group()->index(root), -1.0, size, -1,
                                                    encode_datatype(MPI_CURRENT_TYPE), ""));

  void *sendbuf = smpi_get_tmp_sendbuffer(size* MPI_CURRENT_TYPE->size());

  Colls::bcast(sendbuf, size, MPI_CURRENT_TYPE, root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
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
  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::CollTIData("reduce", MPI_COMM_WORLD->group()->index(root), comp_size,
                                                    comm_size, -1, encode_datatype(MPI_CURRENT_TYPE), ""));

  void *recvbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  void *sendbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  Colls::reduce(sendbuf, recvbuf, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, root, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_comm_out(rank);
  log_timed_action (action, clock);
}

static void action_allReduce(const char *const *action) {
  CHECK_ACTION_PARAMS(action, 2, 1)
  double comm_size = parse_double(action[2]);
  double comp_size = parse_double(action[3]);

  if(action[4])
    MPI_CURRENT_TYPE=decode_datatype(action[4]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  double clock = smpi_process()->simulated_elapsed();
  int rank = smpi_process()->index();
  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::CollTIData("allReduce", -1, comp_size, comm_size, -1,
                                                                        encode_datatype(MPI_CURRENT_TYPE), ""));

  void *recvbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  void *sendbuf = smpi_get_tmp_sendbuffer(comm_size* MPI_CURRENT_TYPE->size());
  Colls::allreduce(sendbuf, recvbuf, comm_size, MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_comm_out(rank);
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
  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::CollTIData("allToAll", -1, -1.0, send_size, recv_size,
                                                                        encode_datatype(MPI_CURRENT_TYPE),
                                                                        encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::alltoall(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
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

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::CollTIData("gather", root, -1.0, send_size, recv_size,
                                                                        encode_datatype(MPI_CURRENT_TYPE),
                                                                        encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::gather(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(smpi_process()->index());
  log_timed_action (action, clock);
}

static void action_scatter(const char* const* action)
{
  /* The structure of the scatter action for the rank 0 (total 4 processes) is the following:
        0 gather 68 68 0 0 0
      where:
        1) 68 is the sendcounts
        2) 68 is the recvcounts
        3) 0 is the root node
        4) 0 is the send datatype id, see decode_datatype()
        5) 0 is the recv datatype id, see decode_datatype()
  */
  CHECK_ACTION_PARAMS(action, 2, 3)
  double clock                   = smpi_process()->simulated_elapsed();
  int comm_size                  = MPI_COMM_WORLD->size();
  int send_size                  = parse_double(action[2]);
  int recv_size                  = parse_double(action[3]);
  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;
  if (action[4] && action[5]) {
    MPI_CURRENT_TYPE  = decode_datatype(action[5]);
    MPI_CURRENT_TYPE2 = decode_datatype(action[6]);
  } else {
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;
  }
  void* send = smpi_get_tmp_sendbuffer(send_size * MPI_CURRENT_TYPE->size());
  void* recv = nullptr;
  int root   = 0;
  if (action[4])
    root   = atoi(action[4]);
  int rank = MPI_COMM_WORLD->rank();

  if (rank == root)
    recv = smpi_get_tmp_recvbuffer(recv_size * comm_size * MPI_CURRENT_TYPE2->size());

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::CollTIData("gather", root, -1.0, send_size, recv_size,
                                                                        encode_datatype(MPI_CURRENT_TYPE),
                                                                        encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::scatter(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(smpi_process()->index());
  log_timed_action(action, clock);
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

  std::vector<int>* trace_recvcounts = new std::vector<int>;
  for (int i = 0; i < comm_size; i++) // copy data to avoid bad free
    trace_recvcounts->push_back(recvcounts[i]);

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::VarCollTIData(
                                             "gatherV", root, send_size, nullptr, -1, trace_recvcounts,
                                             encode_datatype(MPI_CURRENT_TYPE), encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::gatherv(send, send_size, MPI_CURRENT_TYPE, recv, recvcounts, disps, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(smpi_process()->index());
  log_timed_action (action, clock);
}

static void action_scatterv(const char* const* action)
{
  /* The structure of the scatterv action for the rank 0 (total 4 processes) is the following:
       0 gather 68 10 10 10 68 0 0 0
     where:
       1) 68 10 10 10 is the sendcounts
       2) 68 is the recvcount
       3) 0 is the root node
       4) 0 is the send datatype id, see decode_datatype()
       5) 0 is the recv datatype id, see decode_datatype()
  */
  double clock  = smpi_process()->simulated_elapsed();
  int comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size + 1, 2)
  int recv_size = parse_double(action[2 + comm_size]);
  int disps[comm_size];
  int sendcounts[comm_size];
  int send_sum = 0;

  MPI_Datatype MPI_CURRENT_TYPE2 = MPI_DEFAULT_TYPE;
  if (action[4 + comm_size] && action[5 + comm_size]) {
    MPI_CURRENT_TYPE  = decode_datatype(action[4 + comm_size]);
    MPI_CURRENT_TYPE2 = decode_datatype(action[5 + comm_size]);
  } else
    MPI_CURRENT_TYPE = MPI_DEFAULT_TYPE;

  void* send = nullptr;
  void* recv = smpi_get_tmp_recvbuffer(recv_size * MPI_CURRENT_TYPE->size());
  for (int i = 0; i < comm_size; i++) {
    sendcounts[i] = atoi(action[i + 2]);
    send_sum += sendcounts[i];
    disps[i] = 0;
  }

  int root = atoi(action[3 + comm_size]);
  int rank = MPI_COMM_WORLD->rank();

  if (rank == root)
    send = smpi_get_tmp_sendbuffer(send_sum * MPI_CURRENT_TYPE2->size());

  std::vector<int>* trace_sendcounts = new std::vector<int>;
  for (int i = 0; i < comm_size; i++) // copy data to avoid bad free
    trace_sendcounts->push_back(sendcounts[i]);

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::VarCollTIData(
                                             "gatherV", root, -1, trace_sendcounts, recv_size, nullptr,
                                             encode_datatype(MPI_CURRENT_TYPE), encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::scatterv(send, sendcounts, disps, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(smpi_process()->index());
  log_timed_action(action, clock);
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
  std::vector<int>* trace_recvcounts = new std::vector<int>;
  if(action[3+comm_size])
    MPI_CURRENT_TYPE=decode_datatype(action[3+comm_size]);
  else
    MPI_CURRENT_TYPE= MPI_DEFAULT_TYPE;

  for(int i=0;i<comm_size;i++) {
    recvcounts[i] = atoi(action[i+2]);
    trace_recvcounts->push_back(recvcounts[i]);
    size+=recvcounts[i];
  }

  TRACE_smpi_comm_in(rank, __FUNCTION__,
                     new simgrid::instr::VarCollTIData("reduceScatter", -1, 0, nullptr, -1, trace_recvcounts,
                                                       std::to_string(comp_size), /* ugly hack to print comp_size */
                                                       encode_datatype(MPI_CURRENT_TYPE)));

  void *sendbuf = smpi_get_tmp_sendbuffer(size* MPI_CURRENT_TYPE->size());
  void *recvbuf = smpi_get_tmp_recvbuffer(size* MPI_CURRENT_TYPE->size());

  Colls::reduce_scatter(sendbuf, recvbuf, recvcounts, MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_comm_out(rank);
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

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::CollTIData("allGather", -1, -1.0, sendcount, recvcount,
                                                                        encode_datatype(MPI_CURRENT_TYPE),
                                                                        encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::allgather(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcount, MPI_CURRENT_TYPE2, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
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

  std::vector<int>* trace_recvcounts = new std::vector<int>;
  for (int i = 0; i < comm_size; i++) // copy data to avoid bad free
    trace_recvcounts->push_back(recvcounts[i]);

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::VarCollTIData(
                                             "allGatherV", -1, sendcount, nullptr, -1, trace_recvcounts,
                                             encode_datatype(MPI_CURRENT_TYPE), encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::allgatherv(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcounts, disps, MPI_CURRENT_TYPE2,
                          MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
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
  int send_size = 0;
  int recv_size = 0;
  int sendcounts[comm_size];
  std::vector<int>* trace_sendcounts = new std::vector<int>;
  int recvcounts[comm_size];
  std::vector<int>* trace_recvcounts = new std::vector<int>;
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

  int rank       = smpi_process()->index();
  void *sendbuf = smpi_get_tmp_sendbuffer(send_buf_size* MPI_CURRENT_TYPE->size());
  void *recvbuf  = smpi_get_tmp_recvbuffer(recv_buf_size* MPI_CURRENT_TYPE2->size());

  for(int i=0;i<comm_size;i++) {
    sendcounts[i] = atoi(action[i+3]);
    trace_sendcounts->push_back(sendcounts[i]);
    send_size += sendcounts[i];
    recvcounts[i] = atoi(action[i+4+comm_size]);
    trace_recvcounts->push_back(recvcounts[i]);
    recv_size += recvcounts[i];
    senddisps[i] = 0;
    recvdisps[i] = 0;
  }

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::VarCollTIData(
                                             "allToAllV", -1, send_size, trace_sendcounts, recv_size, trace_recvcounts,
                                             encode_datatype(MPI_CURRENT_TYPE), encode_datatype(MPI_CURRENT_TYPE2)));

  Colls::alltoallv(sendbuf, sendcounts, senddisps, MPI_CURRENT_TYPE,recvbuf, recvcounts, recvdisps,
                         MPI_CURRENT_TYPE, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(rank);
  log_timed_action (action, clock);
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
  TRACE_smpi_comm_in(rank, "smpi_replay_run_init", new simgrid::instr::NoOpTIData("init"));
  TRACE_smpi_comm_out(rank);
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
  xbt_replay_action_register("scatter", simgrid::smpi::action_scatter);
  xbt_replay_action_register("gatherV",    simgrid::smpi::action_gatherv);
  xbt_replay_action_register("scatterV", simgrid::smpi::action_scatterv);
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
  XBT_DEBUG("There are %zu elements in reqq[*]", get_reqq_self()->size());
  if (not get_reqq_self()->empty()) {
    unsigned int count_requests=get_reqq_self()->size();
    MPI_Request requests[count_requests];
    MPI_Status status[count_requests];
    unsigned int i=0;

    for (auto const& req : *get_reqq_self()) {
      requests[i] = req;
      i++;
    }
    simgrid::smpi::Request::waitall(count_requests, requests, status);
  }
  delete get_reqq_self();
  active_processes--;

  if(active_processes==0){
    /* Last process alive speaking: end the simulated timer */
    XBT_INFO("Simulation time %f", smpi_process()->simulated_elapsed());
    xbt_free(sendbuffer);
    xbt_free(recvbuffer);
  }

  TRACE_smpi_comm_in(smpi_process()->index(), "smpi_replay_run_finalize", new simgrid::instr::NoOpTIData("finalize"));

  smpi_process()->finalize();

  TRACE_smpi_comm_out(smpi_process()->index());
  TRACE_smpi_finalize(smpi_process()->index());
}

/** @brief chain a replay initialization and a replay start */
void smpi_replay_run(int* argc, char*** argv)
{
  smpi_replay_init(argc, argv);
  smpi_replay_main(argc, argv);
}
