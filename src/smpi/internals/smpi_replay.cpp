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

#include <boost/algorithm/string/join.hpp>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

using simgrid::s4u::Actor;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay,smpi,"Trace Replay with SMPI");

static int active_processes  = 0;
static std::unordered_map<int, std::vector<MPI_Request>*> reqq;

static MPI_Datatype MPI_DEFAULT_TYPE;

#define CHECK_ACTION_PARAMS(action, mandatory, optional)                                                               \
  {                                                                                                                    \
    if (action.size() < static_cast<unsigned long>(mandatory + 2))                                                     \
      THROWF(arg_error, 0, "%s replay failed.\n"                                                                       \
                           "%lu items were given on the line. First two should be process_id and action.  "            \
                           "This action needs after them %lu mandatory arguments, and accepts %lu optional ones. \n"   \
                           "Please contact the Simgrid team if support is needed",                                     \
             __FUNCTION__, action.size(), static_cast<unsigned long>(mandatory),                                       \
             static_cast<unsigned long>(optional));                                                                    \
  }

static void log_timed_action(simgrid::xbt::ReplayAction& action, double clock)
{
  if (XBT_LOG_ISENABLED(smpi_replay, xbt_log_priority_verbose)){
    std::string s = boost::algorithm::join(action, " ");
    XBT_VERB("%s %f", s.c_str(), smpi_process()->simulated_elapsed() - clock);
  }
}

static std::vector<MPI_Request>* get_reqq_self()
{
  return reqq.at(Actor::self()->getPid());
}

static void set_reqq_self(std::vector<MPI_Request> *mpi_request)
{
   reqq.insert({Actor::self()->getPid(), mpi_request});
}

/* Helper function */
static double parse_double(std::string string)
{
  return xbt_str_parse_double(string.c_str(), "%s is not a double");
}

namespace simgrid {
namespace smpi {

namespace Replay {
class ActionArgParser {
public:
  virtual void parse(simgrid::xbt::ReplayAction& action) { CHECK_ACTION_PARAMS(action, 0, 0) }
};

class SendRecvParser : public ActionArgParser {
public:
  /* communication partner; if we send, this is the receiver and vice versa */
  int partner;
  double size;
  MPI_Datatype datatype1 = MPI_DEFAULT_TYPE;

  void parse(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 2, 1)
    partner = std::stoi(action[2]);
    size    = parse_double(action[3]);
    if (action.size() > 4)
      datatype1 = simgrid::smpi::Datatype::decode(action[4]);
  }
};

class ComputeParser : public ActionArgParser {
public:
  /* communication partner; if we send, this is the receiver and vice versa */
  double flops;

  void parse(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 1, 0)
    flops = parse_double(action[2]);
  }
};

class CollCommParser : public ActionArgParser {
public:
  double size;
  double comm_size;
  double comp_size;
  int send_size;
  int recv_size;
  int root = 0;
  MPI_Datatype datatype1 = MPI_DEFAULT_TYPE;
  MPI_Datatype datatype2 = MPI_DEFAULT_TYPE;
};

class BcastArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 1, 2)
    size = parse_double(action[2]);
    root = (action.size() > 3) ? std::stoi(action[3]) : 0;
    if (action.size() > 4)
      datatype1 = simgrid::smpi::Datatype::decode(action[4]);
  }
};

class ReduceArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 2, 2)
    comm_size = parse_double(action[2]);
    comp_size = parse_double(action[3]);
    root      = (action.size() > 4) ? std::stoi(action[4]) : 0;
    if (action.size() > 5)
      datatype1 = simgrid::smpi::Datatype::decode(action[5]);
  }
};

class AllReduceArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 2, 1)
    comm_size = parse_double(action[2]);
    comp_size = parse_double(action[3]);
    if (action.size() > 4)
      datatype1 = simgrid::smpi::Datatype::decode(action[4]);
  }
};

class AllToAllArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 2, 1)
    comm_size = MPI_COMM_WORLD->size();
    send_size = parse_double(action[2]);
    recv_size = parse_double(action[3]);

    if (action.size() > 4)
      datatype1 = simgrid::smpi::Datatype::decode(action[4]);
    if (action.size() > 5)
      datatype2 = simgrid::smpi::Datatype::decode(action[5]);
  }
};

class GatherParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action) override
  {
    /* The structure of the gather action for the rank 0 (total 4 processes) is the following:
          0 gather 68 68 0 0 0
        where:
          1) 68 is the sendcounts
          2) 68 is the recvcounts
          3) 0 is the root node
          4) 0 is the send datatype id, see simgrid::smpi::Datatype::decode()
          5) 0 is the recv datatype id, see simgrid::smpi::Datatype::decode()
    */
    CHECK_ACTION_PARAMS(action, 2, 3)
    comm_size = MPI_COMM_WORLD->size();
    send_size = parse_double(action[2]);
    recv_size = parse_double(action[3]);
    root      = (action.size() > 4) ? std::stoi(action[4]) : 0;

    if (action.size() > 5)
      datatype1 = simgrid::smpi::Datatype::decode(action[5]);
    if (action.size() > 6)
      datatype2 = simgrid::smpi::Datatype::decode(action[6]);
  }
};

template <class T> class ReplayAction {
protected:
  const std::string name;
  T args;

  int my_proc_id;

public:
  explicit ReplayAction(std::string name) : name(name), my_proc_id(simgrid::s4u::Actor::self()->getPid()) {}

  virtual void execute(simgrid::xbt::ReplayAction& action)
  {
    // Needs to be re-initialized for every action, hence here
    double start_time = smpi_process()->simulated_elapsed();
    args.parse(action);
    kernel(action);
    if (name != "Init")
      log_timed_action(action, start_time);
  }

  virtual void kernel(simgrid::xbt::ReplayAction& action) = 0;

  void* send_buffer(int size)
  {
    return smpi_get_tmp_sendbuffer(size);
  }

  void* recv_buffer(int size)
  {
    return smpi_get_tmp_recvbuffer(size);
  }
};

class WaitAction : public ReplayAction<ActionArgParser> {
public:
  WaitAction() : ReplayAction("Wait") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    std::string s = boost::algorithm::join(action, " ");
    xbt_assert(get_reqq_self()->size(), "action wait not preceded by any irecv or isend: %s", s.c_str());
    MPI_Request request = get_reqq_self()->back();
    get_reqq_self()->pop_back();

    if (request == nullptr) {
      /* Assume that the trace is well formed, meaning the comm might have been caught by a MPI_test. Then just
       * return.*/
      return;
    }

    int rank = request->comm() != MPI_COMM_NULL ? request->comm()->rank() : -1;

    // Must be taken before Request::wait() since the request may be set to
    // MPI_REQUEST_NULL by Request::wait!
    int src                  = request->comm()->group()->rank(request->src());
    int dst                  = request->comm()->group()->rank(request->dst());
    bool is_wait_for_receive = (request->flags() & RECV);
    // TODO: Here we take the rank while we normally take the process id (look for my_proc_id)
    TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::NoOpTIData("wait"));

    MPI_Status status;
    Request::wait(&request, &status);

    TRACE_smpi_comm_out(rank);
    if (is_wait_for_receive)
      TRACE_smpi_recv(src, dst, 0);
  }
};

class SendAction : public ReplayAction<SendRecvParser> {
public:
  SendAction() = delete;
  SendAction(std::string name) : ReplayAction(name) {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    int dst_traced = MPI_COMM_WORLD->group()->actor(args.partner)->getPid();

    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__, new simgrid::instr::Pt2PtTIData(name, args.partner, args.size,
                                                                                 Datatype::encode(args.datatype1)));
    if (not TRACE_smpi_view_internals())
      TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, 0, args.size * args.datatype1->size());

    if (name == "send") {
      Request::send(nullptr, args.size, args.datatype1, args.partner, 0, MPI_COMM_WORLD);
    } else if (name == "Isend") {
      MPI_Request request = Request::isend(nullptr, args.size, args.datatype1, args.partner, 0, MPI_COMM_WORLD);
      get_reqq_self()->push_back(request);
    } else {
      xbt_die("Don't know this action, %s", name.c_str());
    }

    TRACE_smpi_comm_out(my_proc_id);
  }
};

class RecvAction : public ReplayAction<SendRecvParser> {
public:
  RecvAction() = delete;
  explicit RecvAction(std::string name) : ReplayAction(name) {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    int src_traced = MPI_COMM_WORLD->group()->actor(args.partner)->getPid();

    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__, new simgrid::instr::Pt2PtTIData(name, args.partner, args.size,
                                                                                 Datatype::encode(args.datatype1)));

    MPI_Status status;
    // unknown size from the receiver point of view
    if (args.size <= 0.0) {
      Request::probe(args.partner, 0, MPI_COMM_WORLD, &status);
      args.size = status.count;
    }

    if (name == "recv") {
      Request::recv(nullptr, args.size, args.datatype1, args.partner, 0, MPI_COMM_WORLD, &status);
    } else if (name == "Irecv") {
      MPI_Request request = Request::irecv(nullptr, args.size, args.datatype1, args.partner, 0, MPI_COMM_WORLD);
      get_reqq_self()->push_back(request);
    }

    TRACE_smpi_comm_out(my_proc_id);
    // TODO: Check why this was only activated in the "recv" case and not in the "Irecv" case
    if (name == "recv" && not TRACE_smpi_view_internals()) {
      TRACE_smpi_recv(src_traced, my_proc_id, 0);
    }
  }
};

class ComputeAction : public ReplayAction<ComputeParser> {
public:
  ComputeAction() : ReplayAction("compute") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_computing_in(my_proc_id, args.flops);
    smpi_execute_flops(args.flops);
    TRACE_smpi_computing_out(my_proc_id);
  }
};

class TestAction : public ReplayAction<ActionArgParser> {
public:
  TestAction() : ReplayAction("Test") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    MPI_Request request = get_reqq_self()->back();
    get_reqq_self()->pop_back();
    // if request is null here, this may mean that a previous test has succeeded
    // Different times in traced application and replayed version may lead to this
    // In this case, ignore the extra calls.
    if (request != nullptr) {
      TRACE_smpi_testing_in(my_proc_id);

      MPI_Status status;
      int flag = Request::test(&request, &status);

      XBT_DEBUG("MPI_Test result: %d", flag);
      /* push back request in vector to be caught by a subsequent wait. if the test did succeed, the request is now
       * nullptr.*/
      get_reqq_self()->push_back(request);

      TRACE_smpi_testing_out(my_proc_id);
    }
  }
};

class InitAction : public ReplayAction<ActionArgParser> {
public:
  InitAction() : ReplayAction("Init") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    CHECK_ACTION_PARAMS(action, 0, 1)
    MPI_DEFAULT_TYPE = (action.size() > 2) ? MPI_DOUBLE // default MPE datatype
                                           : MPI_BYTE;  // default TAU datatype

    /* start a simulated timer */
    smpi_process()->simulated_start();
    /*initialize the number of active processes */
    active_processes = smpi_process_count();

    set_reqq_self(new std::vector<MPI_Request>);
  }
};

class CommunicatorAction : public ReplayAction<ActionArgParser> {
public:
  CommunicatorAction() : ReplayAction("Comm") {}
  void kernel(simgrid::xbt::ReplayAction& action) override { /* nothing to do */}
};

class WaitAllAction : public ReplayAction<ActionArgParser> {
public:
  WaitAllAction() : ReplayAction("waitAll") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    const unsigned int count_requests = get_reqq_self()->size();

    if (count_requests > 0) {
      TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                         new simgrid::instr::Pt2PtTIData("waitAll", -1, count_requests, ""));
      std::vector<std::pair</*sender*/int,/*recv*/int>> sender_receiver;
      for (const auto& req : (*get_reqq_self())) {
        if (req && (req->flags() & RECV)) {
          sender_receiver.push_back({req->src(), req->dst()});
        }
      }
      MPI_Status status[count_requests];
      Request::waitall(count_requests, &(*get_reqq_self())[0], status);

      for (auto& pair : sender_receiver) {
        TRACE_smpi_recv(pair.first, pair.second, 0);
      }
      TRACE_smpi_comm_out(my_proc_id);
    }
  }
};

class BarrierAction : public ReplayAction<ActionArgParser> {
public:
  BarrierAction() : ReplayAction("barrier") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__, new simgrid::instr::NoOpTIData("barrier"));
    Colls::barrier(MPI_COMM_WORLD);
    TRACE_smpi_comm_out(my_proc_id);
  }
};

class BcastAction : public ReplayAction<BcastArgParser> {
public:
  BcastAction() : ReplayAction("bcast") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_comm_in(my_proc_id, "action_bcast",
                       new simgrid::instr::CollTIData("bcast", MPI_COMM_WORLD->group()->actor(args.root)->getPid(),
                                                      -1.0, args.size, -1, Datatype::encode(args.datatype1), ""));

    Colls::bcast(send_buffer(args.size * args.datatype1->size()), args.size, args.datatype1, args.root, MPI_COMM_WORLD);

    TRACE_smpi_comm_out(my_proc_id);
  }
};

class ReduceAction : public ReplayAction<ReduceArgParser> {
public:
  ReduceAction() : ReplayAction("reduce") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_comm_in(my_proc_id, "action_reduce",
                       new simgrid::instr::CollTIData("reduce", MPI_COMM_WORLD->group()->actor(args.root)->getPid(), args.comp_size,
                                                      args.comm_size, -1, Datatype::encode(args.datatype1), ""));

    Colls::reduce(send_buffer(args.comm_size * args.datatype1->size()),
        recv_buffer(args.comm_size * args.datatype1->size()), args.comm_size, args.datatype1, MPI_OP_NULL, args.root, MPI_COMM_WORLD);
    smpi_execute_flops(args.comp_size);

    TRACE_smpi_comm_out(my_proc_id);
  }
};

class AllReduceAction : public ReplayAction<AllReduceArgParser> {
public:
  AllReduceAction() : ReplayAction("allReduce") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_comm_in(my_proc_id, "action_allReduce", new simgrid::instr::CollTIData("allReduce", -1, args.comp_size, args.comm_size, -1,
                                                                                Datatype::encode(args.datatype1), ""));

    Colls::allreduce(send_buffer(args.comm_size * args.datatype1->size()),
        recv_buffer(args.comm_size * args.datatype1->size()), args.comm_size, args.datatype1, MPI_OP_NULL, MPI_COMM_WORLD);
    smpi_execute_flops(args.comp_size);

    TRACE_smpi_comm_out(my_proc_id);
  }
};

class AllToAllAction : public ReplayAction<AllToAllArgParser> {
public:
  AllToAllAction() : ReplayAction("allToAll") {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_comm_in(my_proc_id, "action_allToAll",
                     new simgrid::instr::CollTIData("allToAll", -1, -1.0, args.send_size, args.recv_size,
                                                    Datatype::encode(args.datatype1),
                                                    Datatype::encode(args.datatype2)));

    Colls::alltoall(send_buffer(args.send_size*args.comm_size* args.datatype1->size()), 
      args.send_size, args.datatype1, recv_buffer(args.recv_size * args.comm_size * args.datatype2->size()),
      args.recv_size, args.datatype2, MPI_COMM_WORLD);

    TRACE_smpi_comm_out(my_proc_id);
  }
};

class GatherAction : public ReplayAction<GatherArgParser> {
public:
  GatherAction(std::string name) : ReplayAction(name) {}
  void kernel(simgrid::xbt::ReplayAction& action) override
  {
    TRACE_smpi_comm_in(my_proc_id, name, new simgrid::instr::CollTIData(name, (name == "gather") ? args.root : -1, -1.0, args.send_size, args.recv_size,
                                                                          Datatype::encode(args.datatype1), Datatype::encode(args.datatype2)));

    if (name == "gather")
      Colls::gather(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                 (rank == args.root) ? recv_buffer(args.recv_size * args.comm_size * args.datatype2->size()) : nullptr, args.recv_size, args.datatype2, args.root, MPI_COMM_WORLD);
    else
      Colls::allgather(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                       recv_buffer(args.recv_size * args.datatype2->size()), args.recv_size, args.datatype2, MPI_COMM_WORLD);

    TRACE_smpi_comm_out(my_proc_id);
  }
};
} // Replay Namespace

static void action_scatter(simgrid::xbt::ReplayAction& action)
{
  /* The structure of the scatter action for the rank 0 (total 4 processes) is the following:
        0 gather 68 68 0 0 0
      where:
        1) 68 is the sendcounts
        2) 68 is the recvcounts
        3) 0 is the root node
        4) 0 is the send datatype id, see simgrid::smpi::Datatype::decode()
        5) 0 is the recv datatype id, see simgrid::smpi::Datatype::decode()
  */
  CHECK_ACTION_PARAMS(action, 2, 3)
  double clock                   = smpi_process()->simulated_elapsed();
  unsigned long comm_size        = MPI_COMM_WORLD->size();
  int send_size                  = parse_double(action[2]);
  int recv_size                  = parse_double(action[3]);
  MPI_Datatype MPI_CURRENT_TYPE{(action.size() > 6) ? simgrid::smpi::Datatype::decode(action[5]) : MPI_DEFAULT_TYPE};
  MPI_Datatype MPI_CURRENT_TYPE2{(action.size() > 6) ? simgrid::smpi::Datatype::decode(action[6]) : MPI_DEFAULT_TYPE};

  void* send = smpi_get_tmp_sendbuffer(send_size * MPI_CURRENT_TYPE->size());
  void* recv = nullptr;
  int root   = (action.size() > 4) ? std::stoi(action[4]) : 0;
  int rank = MPI_COMM_WORLD->rank();

  if (rank == root)
    recv = smpi_get_tmp_recvbuffer(recv_size * comm_size * MPI_CURRENT_TYPE2->size());

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::CollTIData("gather", root, -1.0, send_size, recv_size,
                                                                        Datatype::encode(MPI_CURRENT_TYPE),
                                                                        Datatype::encode(MPI_CURRENT_TYPE2)));

  Colls::scatter(send, send_size, MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(Actor::self()->getPid());
  log_timed_action(action, clock);
}

static void action_gatherv(simgrid::xbt::ReplayAction& action)
{
  /* The structure of the gatherv action for the rank 0 (total 4 processes) is the following:
       0 gather 68 68 10 10 10 0 0 0
     where:
       1) 68 is the sendcount
       2) 68 10 10 10 is the recvcounts
       3) 0 is the root node
       4) 0 is the send datatype id, see simgrid::smpi::Datatype::decode()
       5) 0 is the recv datatype id, see simgrid::smpi::Datatype::decode()
  */
  double clock = smpi_process()->simulated_elapsed();
  unsigned long comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size+1, 2)
  int send_size = parse_double(action[2]);
  std::vector<int> disps(comm_size, 0);
  std::shared_ptr<std::vector<int>> recvcounts(new std::vector<int>(comm_size));

  MPI_Datatype MPI_CURRENT_TYPE =
      (action.size() > 5 + comm_size) ? simgrid::smpi::Datatype::decode(action[4 + comm_size]) : MPI_DEFAULT_TYPE;
  MPI_Datatype MPI_CURRENT_TYPE2{
      (action.size() > 5 + comm_size) ? simgrid::smpi::Datatype::decode(action[5 + comm_size]) : MPI_DEFAULT_TYPE};

  void *send = smpi_get_tmp_sendbuffer(send_size* MPI_CURRENT_TYPE->size());
  void *recv = nullptr;
  for (unsigned int i = 0; i < comm_size; i++) {
    (*recvcounts)[i] = std::stoi(action[i + 3]);
  }
  int recv_sum = std::accumulate(recvcounts->begin(), recvcounts->end(), 0);

  int root = (action.size() > 3 + comm_size) ? std::stoi(action[3 + comm_size]) : 0;
  int rank = MPI_COMM_WORLD->rank();

  if(rank==root)
    recv = smpi_get_tmp_recvbuffer(recv_sum* MPI_CURRENT_TYPE2->size());

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::VarCollTIData(
                                             "gatherV", root, send_size, nullptr, -1, recvcounts,
                                             Datatype::encode(MPI_CURRENT_TYPE), Datatype::encode(MPI_CURRENT_TYPE2)));

  Colls::gatherv(send, send_size, MPI_CURRENT_TYPE, recv, recvcounts->data(), disps.data(), MPI_CURRENT_TYPE2, root,
                 MPI_COMM_WORLD);

  TRACE_smpi_comm_out(Actor::self()->getPid());
  log_timed_action (action, clock);
}

static void action_scatterv(simgrid::xbt::ReplayAction& action)
{
  /* The structure of the scatterv action for the rank 0 (total 4 processes) is the following:
       0 gather 68 10 10 10 68 0 0 0
     where:
       1) 68 10 10 10 is the sendcounts
       2) 68 is the recvcount
       3) 0 is the root node
       4) 0 is the send datatype id, see simgrid::smpi::Datatype::decode()
       5) 0 is the recv datatype id, see simgrid::smpi::Datatype::decode()
  */
  double clock  = smpi_process()->simulated_elapsed();
  unsigned long comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size + 1, 2)
  int recv_size = parse_double(action[2 + comm_size]);
  std::vector<int> disps(comm_size, 0);
  std::shared_ptr<std::vector<int>> sendcounts(new std::vector<int>(comm_size));

  MPI_Datatype MPI_CURRENT_TYPE =
      (action.size() > 5 + comm_size) ? simgrid::smpi::Datatype::decode(action[4 + comm_size]) : MPI_DEFAULT_TYPE;
  MPI_Datatype MPI_CURRENT_TYPE2{
      (action.size() > 5 + comm_size) ? simgrid::smpi::Datatype::decode(action[5 + comm_size]) : MPI_DEFAULT_TYPE};

  void* send = nullptr;
  void* recv = smpi_get_tmp_recvbuffer(recv_size * MPI_CURRENT_TYPE->size());
  for (unsigned int i = 0; i < comm_size; i++) {
    (*sendcounts)[i] = std::stoi(action[i + 2]);
  }
  int send_sum = std::accumulate(sendcounts->begin(), sendcounts->end(), 0);

  int root = (action.size() > 3 + comm_size) ? std::stoi(action[3 + comm_size]) : 0;
  int rank = MPI_COMM_WORLD->rank();

  if (rank == root)
    send = smpi_get_tmp_sendbuffer(send_sum * MPI_CURRENT_TYPE2->size());

  TRACE_smpi_comm_in(rank, __FUNCTION__, new simgrid::instr::VarCollTIData("gatherV", root, -1, sendcounts, recv_size,
                                                                           nullptr, Datatype::encode(MPI_CURRENT_TYPE),
                                                                           Datatype::encode(MPI_CURRENT_TYPE2)));

  Colls::scatterv(send, sendcounts->data(), disps.data(), MPI_CURRENT_TYPE, recv, recv_size, MPI_CURRENT_TYPE2, root,
                  MPI_COMM_WORLD);

  TRACE_smpi_comm_out(Actor::self()->getPid());
  log_timed_action(action, clock);
}

static void action_reducescatter(simgrid::xbt::ReplayAction& action)
{
  /* The structure of the reducescatter action for the rank 0 (total 4 processes) is the following:
       0 reduceScatter 275427 275427 275427 204020 11346849 0
     where:
       1) The first four values after the name of the action declare the recvcounts array
       2) The value 11346849 is the amount of instructions
       3) The last value corresponds to the datatype, see simgrid::smpi::Datatype::decode().
 */
  double clock = smpi_process()->simulated_elapsed();
  unsigned long comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size+1, 1)
  int comp_size = parse_double(action[2+comm_size]);
  int my_proc_id                     = Actor::self()->getPid();
  std::shared_ptr<std::vector<int>> recvcounts(new std::vector<int>);
  MPI_Datatype MPI_CURRENT_TYPE =
      (action.size() > 3 + comm_size) ? simgrid::smpi::Datatype::decode(action[3 + comm_size]) : MPI_DEFAULT_TYPE;

  for (unsigned int i = 0; i < comm_size; i++) {
    recvcounts->push_back(std::stoi(action[i + 2]));
  }
  int size{std::accumulate(recvcounts->begin(), recvcounts->end(), 0)};

  TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                     new simgrid::instr::VarCollTIData("reduceScatter", -1, 0, nullptr, -1, recvcounts,
                                                       std::to_string(comp_size), /* ugly hack to print comp_size */
                                                       Datatype::encode(MPI_CURRENT_TYPE)));

  void *sendbuf = smpi_get_tmp_sendbuffer(size* MPI_CURRENT_TYPE->size());
  void *recvbuf = smpi_get_tmp_recvbuffer(size* MPI_CURRENT_TYPE->size());

  Colls::reduce_scatter(sendbuf, recvbuf, recvcounts->data(), MPI_CURRENT_TYPE, MPI_OP_NULL, MPI_COMM_WORLD);
  smpi_execute_flops(comp_size);

  TRACE_smpi_comm_out(my_proc_id);
  log_timed_action (action, clock);
}

static void action_allgatherv(simgrid::xbt::ReplayAction& action)
{
  /* The structure of the allgatherv action for the rank 0 (total 4 processes) is the following:
        0 allGatherV 275427 275427 275427 275427 204020
     where:
        1) 275427 is the sendcount
        2) The next four elements declare the recvcounts array
        3) No more values mean that the datatype for sent and receive buffer is the default one, see
     simgrid::smpi::Datatype::decode().
  */
  double clock = smpi_process()->simulated_elapsed();

  unsigned long comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size+1, 2)
  int sendcount = std::stoi(action[2]);
  std::shared_ptr<std::vector<int>> recvcounts(new std::vector<int>(comm_size));
  std::vector<int> disps(comm_size, 0);

  int datatype_index = 0, disp_index = 0;
  if (action.size() > 3 + 2 * comm_size) { /* datatype + disp are specified */
    datatype_index = 3 + comm_size;
    disp_index     = datatype_index + 1;
  } else if (action.size() > 3 + 2 * comm_size) { /* disps specified; datatype is not specified; use the default one */
    datatype_index = -1;
    disp_index     = 3 + comm_size;
  } else if (action.size() > 3 + comm_size) { /* only datatype, no disp specified */
    datatype_index = 3 + comm_size;
  }

  if (disp_index != 0) {
    for (unsigned int i = 0; i < comm_size; i++)
      disps[i]          = std::stoi(action[disp_index + i]);
  }

  MPI_Datatype MPI_CURRENT_TYPE{(datatype_index > 0) ? simgrid::smpi::Datatype::decode(action[datatype_index])
                                                     : MPI_DEFAULT_TYPE};
  MPI_Datatype MPI_CURRENT_TYPE2{(datatype_index > 0) ? simgrid::smpi::Datatype::decode(action[datatype_index])
                                                      : MPI_DEFAULT_TYPE};

  void *sendbuf = smpi_get_tmp_sendbuffer(sendcount* MPI_CURRENT_TYPE->size());

  for (unsigned int i = 0; i < comm_size; i++) {
    (*recvcounts)[i] = std::stoi(action[i + 3]);
  }
  int recv_sum  = std::accumulate(recvcounts->begin(), recvcounts->end(), 0);
  void *recvbuf = smpi_get_tmp_recvbuffer(recv_sum* MPI_CURRENT_TYPE2->size());

  int my_proc_id = Actor::self()->getPid();

  TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                     new simgrid::instr::VarCollTIData("allGatherV", -1, sendcount, nullptr, -1, recvcounts,
                                                       Datatype::encode(MPI_CURRENT_TYPE),
                                                       Datatype::encode(MPI_CURRENT_TYPE2)));

  Colls::allgatherv(sendbuf, sendcount, MPI_CURRENT_TYPE, recvbuf, recvcounts->data(), disps.data(), MPI_CURRENT_TYPE2,
                    MPI_COMM_WORLD);

  TRACE_smpi_comm_out(my_proc_id);
  log_timed_action (action, clock);
}

static void action_allToAllv(simgrid::xbt::ReplayAction& action)
{
  /* The structure of the allToAllV action for the rank 0 (total 4 processes) is the following:
        0 allToAllV 100 1 7 10 12 100 1 70 10 5
     where:
        1) 100 is the size of the send buffer *sizeof(int),
        2) 1 7 10 12 is the sendcounts array
        3) 100*sizeof(int) is the size of the receiver buffer
        4)  1 70 10 5 is the recvcounts array
  */
  double clock = smpi_process()->simulated_elapsed();

  unsigned long comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, 2*comm_size+2, 2)
  std::shared_ptr<std::vector<int>> sendcounts(new std::vector<int>(comm_size));
  std::shared_ptr<std::vector<int>> recvcounts(new std::vector<int>(comm_size));
  std::vector<int> senddisps(comm_size, 0);
  std::vector<int> recvdisps(comm_size, 0);

  MPI_Datatype MPI_CURRENT_TYPE = (action.size() > 5 + 2 * comm_size)
                                      ? simgrid::smpi::Datatype::decode(action[4 + 2 * comm_size])
                                      : MPI_DEFAULT_TYPE;
  MPI_Datatype MPI_CURRENT_TYPE2{(action.size() > 5 + 2 * comm_size)
                                     ? simgrid::smpi::Datatype::decode(action[5 + 2 * comm_size])
                                     : MPI_DEFAULT_TYPE};

  int send_buf_size=parse_double(action[2]);
  int recv_buf_size=parse_double(action[3+comm_size]);
  int my_proc_id = Actor::self()->getPid();
  void *sendbuf = smpi_get_tmp_sendbuffer(send_buf_size* MPI_CURRENT_TYPE->size());
  void *recvbuf  = smpi_get_tmp_recvbuffer(recv_buf_size* MPI_CURRENT_TYPE2->size());

  for (unsigned int i = 0; i < comm_size; i++) {
    (*sendcounts)[i] = std::stoi(action[3 + i]);
    (*recvcounts)[i] = std::stoi(action[4 + comm_size + i]);
  }
  int send_size = std::accumulate(sendcounts->begin(), sendcounts->end(), 0);
  int recv_size = std::accumulate(recvcounts->begin(), recvcounts->end(), 0);

  TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                     new simgrid::instr::VarCollTIData("allToAllV", -1, send_size, sendcounts, recv_size, recvcounts,
                                                       Datatype::encode(MPI_CURRENT_TYPE),
                                                       Datatype::encode(MPI_CURRENT_TYPE2)));

  Colls::alltoallv(sendbuf, sendcounts->data(), senddisps.data(), MPI_CURRENT_TYPE, recvbuf, recvcounts->data(),
                   recvdisps.data(), MPI_CURRENT_TYPE, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(my_proc_id);
  log_timed_action (action, clock);
}

}} // namespace simgrid::smpi

/** @brief Only initialize the replay, don't do it for real */
void smpi_replay_init(int* argc, char*** argv)
{
  simgrid::smpi::Process::init(argc, argv);
  smpi_process()->mark_as_initialized();
  smpi_process()->set_replaying(true);

  int my_proc_id = Actor::self()->getPid();
  TRACE_smpi_init(my_proc_id);
  TRACE_smpi_computing_init(my_proc_id);
  TRACE_smpi_comm_in(my_proc_id, "smpi_replay_run_init", new simgrid::instr::NoOpTIData("init"));
  TRACE_smpi_comm_out(my_proc_id);
  xbt_replay_action_register("init", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::InitAction().execute(action); });
  xbt_replay_action_register("finalize", [](simgrid::xbt::ReplayAction& action) { /* nothing to do */ });
  xbt_replay_action_register("comm_size", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::CommunicatorAction().execute(action); });
  xbt_replay_action_register("comm_split",[](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::CommunicatorAction().execute(action); });
  xbt_replay_action_register("comm_dup",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::CommunicatorAction().execute(action); });

  xbt_replay_action_register("send",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::SendAction("send").execute(action); });
  xbt_replay_action_register("Isend", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::SendAction("Isend").execute(action); });
  xbt_replay_action_register("recv",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::RecvAction("recv").execute(action); });
  xbt_replay_action_register("Irecv", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::RecvAction("Irecv").execute(action); });
  xbt_replay_action_register("test",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::TestAction().execute(action); });
  xbt_replay_action_register("wait",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::WaitAction().execute(action); });
  xbt_replay_action_register("waitAll", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::WaitAllAction().execute(action); });
  xbt_replay_action_register("barrier", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::BarrierAction().execute(action); });
  xbt_replay_action_register("bcast",   [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::BcastAction().execute(action); });
  xbt_replay_action_register("reduce",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::ReduceAction().execute(action); });
  xbt_replay_action_register("allReduce", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::AllReduceAction().execute(action); });
  xbt_replay_action_register("allToAll", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::AllToAllAction().execute(action); });
  xbt_replay_action_register("allToAllV",  simgrid::smpi::action_allToAllv);
  xbt_replay_action_register("gather",   [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::GatherAction("gather").execute(action); });
  xbt_replay_action_register("scatter", simgrid::smpi::action_scatter);
  xbt_replay_action_register("gatherV",    simgrid::smpi::action_gatherv);
  xbt_replay_action_register("scatterV", simgrid::smpi::action_scatterv);
  xbt_replay_action_register("allGather", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::GatherAction("allGather").execute(action); });
  xbt_replay_action_register("allGatherV", simgrid::smpi::action_allgatherv);
  xbt_replay_action_register("reduceScatter",  simgrid::smpi::action_reducescatter);
  xbt_replay_action_register("compute", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::Replay::ComputeAction().execute(action); });

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
    smpi_free_replay_tmp_buffers();
  }

  TRACE_smpi_comm_in(Actor::self()->getPid(), "smpi_replay_run_finalize", new simgrid::instr::NoOpTIData("finalize"));

  smpi_process()->finalize();

  TRACE_smpi_comm_out(Actor::self()->getPid());
  TRACE_smpi_finalize(Actor::self()->getPid());
}

/** @brief chain a replay initialization and a replay start */
void smpi_replay_run(int* argc, char*** argv)
{
  smpi_replay_init(argc, argv);
  smpi_replay_main(argc, argv);
}
