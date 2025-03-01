/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_replay.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_config.hpp"
#include "smpi_datatype.hpp"
#include "smpi_group.hpp"
#include "smpi_request.hpp"
#include "src/smpi/include/private.hpp"
#include "xbt/replay.hpp"
#include "xbt/str.h"

#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <tuple>
#include <unordered_map>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_replay, smpi, "Trace Replay with SMPI");
// From https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
// This is all just to make std::unordered_map work with std::tuple. If we need this in other places,
// this could go into a header file.
namespace hash_tuple {
template <typename TT> class hash {
public:
  size_t operator()(TT const& tt) const { return std::hash<TT>()(tt); }
};

template <class T> inline void hash_combine(std::size_t& seed, T const& v)
{
  seed ^= hash_tuple::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Recursive template code derived from Matthieu M.
template <class Tuple, size_t Index = std::tuple_size_v<Tuple> - 1> class HashValueImpl {
public:
  static void apply(size_t& seed, Tuple const& tuple)
  {
    HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
    hash_combine(seed, std::get<Index>(tuple));
  }
};

template <class Tuple> class HashValueImpl<Tuple, 0> {
public:
  static void apply(size_t& seed, Tuple const& tuple) { hash_combine(seed, std::get<0>(tuple)); }
};

template <typename... TT> class hash<std::tuple<TT...>> {
public:
  size_t operator()(std::tuple<TT...> const& tt) const
  {
    size_t seed = 0;
    HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
    return seed;
  }
};
}

void log_timed_action(const simgrid::xbt::ReplayAction& action, double clock)
{
  if (XBT_LOG_ISENABLED(smpi_replay, xbt_log_priority_verbose)){
    std::string s = boost::algorithm::join(action, " ");
    XBT_VERB("%s %f", s.c_str(), smpi_process()->simulated_elapsed() - clock);
  }
}

/* Helper functions */
static double parse_double(const std::string& string)
{
  return xbt_str_parse_double(string.c_str(), "not a double");
}

template <typename T> static T parse_integer(const std::string& string)
{
  double val = trunc(xbt_str_parse_double(string.c_str(), "not a double"));
  xbt_assert(static_cast<double>(std::numeric_limits<T>::min()) <= val &&
                 val <= static_cast<double>(std::numeric_limits<T>::max()),
             "out of range: %g", val);
  return static_cast<T>(val);
}

static int parse_root(const simgrid::xbt::ReplayAction& action, unsigned i)
{
  return i < action.size() ? std::stoi(action[i]) : 0;
}

static MPI_Datatype parse_datatype(const simgrid::xbt::ReplayAction& action, unsigned i)
{
  return i < action.size() ? simgrid::smpi::Datatype::decode(action[i]) : simgrid::smpi::replay::MPI_DEFAULT_TYPE;
}

namespace simgrid::smpi::replay {
MPI_Datatype MPI_DEFAULT_TYPE;

class RequestStorage {
private:
  using req_key_t     = std::tuple</*sender*/ int, /* receiver */ int, /* tag */ int>;
  using req_storage_t = std::unordered_map<req_key_t, std::list<MPI_Request>, hash_tuple::hash<std::tuple<int, int, int>>>;

  req_storage_t store;

public:
  RequestStorage() = default;
  size_t size() const { return store.size(); }

  req_storage_t& get_store() { return store; }

  void get_requests(std::vector<MPI_Request>& vec) const
  {
    for (auto const& [_, reqs] : store) {
      aid_t my_proc_id = simgrid::s4u::this_actor::get_pid();
      for (const auto& req : reqs) {
        if (req != MPI_REQUEST_NULL && (req->src() == my_proc_id || req->dst() == my_proc_id)) {
          vec.push_back(req);
          req->print_request("MM");
        }
      }
    }
  }

  MPI_Request pop(int src, int dst, int tag)
  {
    auto it = store.find(req_key_t(src, dst, tag));
    if (it == store.end())
     return MPI_REQUEST_NULL;
    MPI_Request req = it->second.front();
    it->second.pop_front();
    if(it->second.empty())
      store.erase(req_key_t(src, dst, tag));
    return req;
  }

  void add(MPI_Request req)
  {
    if (req != MPI_REQUEST_NULL){ // Can and does happen in the case of TestAction
      store[req_key_t(req->src()-1, req->dst()-1, req->tag())].push_back(req);
    }
  }

  /* Sometimes we need to re-insert MPI_REQUEST_NULL but we still need src,dst and tag */
  void addNullRequest(int src, int dst, int tag)
  {
    int src_pid = MPI_COMM_WORLD->group()->actor(src) - 1;
    int dest_pid = MPI_COMM_WORLD->group()->actor(dst) - 1;
    store[req_key_t(src_pid, dest_pid, tag)].push_back(MPI_REQUEST_NULL);
  }
};

void WaitTestParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 3, 0)
  src = std::stoi(action[2]);
  dst = std::stoi(action[3]);
  tag = std::stoi(action[4]);
}

void SendOrRecvParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 3, 1)
  partner = std::stoi(action[2]);
  tag     = std::stoi(action[3]);
  size      = parse_integer<ssize_t>(action[4]);
  datatype1 = parse_datatype(action, 5);
}

void ComputeParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 1, 0)
  flops = parse_double(action[2]);
}

void SleepParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 1, 0)
  time = parse_double(action[2]);
}

void LocationParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 2, 0)
  filename = action[2];
  line = std::stoi(action[3]);
}

void SendRecvParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 6, 0)
  sendcount = parse_integer<int>(action[2]);
  dst = std::stoi(action[3]);
  recvcount = parse_integer<int>(action[4]);
  src = std::stoi(action[5]);
  datatype1 = parse_datatype(action, 6);
  datatype2 = parse_datatype(action, 7);
}

void BcastArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 1, 2)
  size      = parse_integer<size_t>(action[2]);
  root      = parse_root(action, 3);
  datatype1 = parse_datatype(action, 4);
}

void ReduceArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 2, 2)
  comm_size = parse_integer<unsigned>(action[2]);
  comp_size = parse_double(action[3]);
  root      = parse_root(action, 4);
  datatype1 = parse_datatype(action, 5);
}

void AllReduceArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 2, 1)
  comm_size = parse_integer<unsigned>(action[2]);
  comp_size = parse_double(action[3]);
  datatype1 = parse_datatype(action, 4);
}

void AllToAllArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 2, 1)
  comm_size = MPI_COMM_WORLD->size();
  send_size = parse_integer<int>(action[2]);
  recv_size = parse_integer<int>(action[3]);
  datatype1 = parse_datatype(action, 4);
  datatype2 = parse_datatype(action, 5);
}

void GatherArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string& name)
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
  send_size = parse_integer<int>(action[2]);
  recv_size = parse_integer<int>(action[3]);

  if (name == "gather") {
    root      = parse_root(action, 4);
    datatype1 = parse_datatype(action, 5);
    datatype2 = parse_datatype(action, 6);
  } else {
    root      = 0;
    datatype1 = parse_datatype(action, 4);
    datatype2 = parse_datatype(action, 5);
  }
}

void GatherVArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string& name)
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
  comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size + 1, 2)
  send_size  = parse_integer<int>(action[2]);
  disps      = std::vector<int>(comm_size, 0);
  recvcounts = std::make_shared<std::vector<int>>(comm_size);

  if (name == "gatherv") {
    root      = parse_root(action, 3 + comm_size);
    datatype1 = parse_datatype(action, 4 + comm_size);
    datatype2 = parse_datatype(action, 5 + comm_size);
  } else {
    root                = 0;
    unsigned disp_index = 0;
    /* The 3 comes from "0 gather <sendcount>", which must always be present.
     * The + comm_size is the recvcounts array, which must also be present
     */
    if (action.size() > 3 + comm_size + comm_size) {
      // datatype + disp are specified
      datatype1  = parse_datatype(action, 3 + comm_size);
      datatype2  = parse_datatype(action, 4 + comm_size);
      disp_index = 5 + comm_size;
    } else if (action.size() > 3 + comm_size + 2) {
      // disps specified; datatype is not specified; use the default one
      datatype1  = MPI_DEFAULT_TYPE;
      datatype2  = MPI_DEFAULT_TYPE;
      disp_index = 3 + comm_size;
    } else {
      // no disp specified, maybe only datatype,
      datatype1 = parse_datatype(action, 3 + comm_size);
      datatype2 = parse_datatype(action, 4 + comm_size);
    }

    if (disp_index != 0) {
      xbt_assert(disp_index + comm_size <= action.size());
      for (unsigned i = 0; i < comm_size; i++)
        disps[i]          = std::stoi(action[disp_index + i]);
    }
  }

  for (unsigned int i = 0; i < comm_size; i++) {
    (*recvcounts)[i] = std::stoi(action[i + 3]);
  }
  recv_size_sum = std::accumulate(recvcounts->begin(), recvcounts->end(), 0);
}

void ScatterArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
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
  comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, 2, 3)
  comm_size = MPI_COMM_WORLD->size();
  send_size = parse_integer<int>(action[2]);
  recv_size = parse_integer<int>(action[3]);
  root      = parse_root(action, 4);
  datatype1 = parse_datatype(action, 5);
  datatype2 = parse_datatype(action, 6);
}

void ScatterVArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
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
  comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size + 1, 2)
  recv_size  = parse_integer<int>(action[2 + comm_size]);
  disps      = std::vector<int>(comm_size, 0);
  sendcounts = std::make_shared<std::vector<int>>(comm_size);

  root      = parse_root(action, 3 + comm_size);
  datatype1 = parse_datatype(action, 4 + comm_size);
  datatype2 = parse_datatype(action, 5 + comm_size);

  for (unsigned int i = 0; i < comm_size; i++) {
    (*sendcounts)[i] = std::stoi(action[i + 2]);
  }
  send_size_sum = std::accumulate(sendcounts->begin(), sendcounts->end(), 0);
}

void ReduceScatterArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  /* The structure of the reducescatter action for the rank 0 (total 4 processes) is the following:
       0 reducescatter 275427 275427 275427 204020 11346849 0
     where:
       1) The first four values after the name of the action declare the recvcounts array
       2) The value 11346849 is the amount of instructions
       3) The last value corresponds to the datatype, see simgrid::smpi::Datatype::decode().
  */
  comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, comm_size + 1, 1)
  comp_size  = parse_double(action[2 + comm_size]);
  recvcounts = std::make_shared<std::vector<int>>(comm_size);
  datatype1  = parse_datatype(action, 3 + comm_size);

  for (unsigned int i = 0; i < comm_size; i++) {
    (*recvcounts)[i]= std::stoi(action[i + 2]);
  }
  recv_size_sum = std::accumulate(recvcounts->begin(), recvcounts->end(), 0);
}

void ScanArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  CHECK_ACTION_PARAMS(action, 2, 1)
  size      = parse_integer<size_t>(action[2]);
  comp_size = parse_double(action[3]);
  datatype1 = parse_datatype(action, 4);
}

void AllToAllVArgParser::parse(simgrid::xbt::ReplayAction& action, const std::string&)
{
  /* The structure of the alltoallv action for the rank 0 (total 4 processes) is the following:
        0 alltoallv 100 1 7 10 12 100 1 70 10 5
     where:
      1) 100 is the size of the send buffer *sizeof(int),
      2) 1 7 10 12 is the sendcounts array
      3) 100*sizeof(int) is the size of the receiver buffer
      4)  1 70 10 5 is the recvcounts array
  */
  comm_size = MPI_COMM_WORLD->size();
  CHECK_ACTION_PARAMS(action, 2 * comm_size + 2, 2)
  sendcounts = std::make_shared<std::vector<int>>(comm_size);
  recvcounts = std::make_shared<std::vector<int>>(comm_size);
  senddisps  = std::vector<int>(comm_size, 0);
  recvdisps  = std::vector<int>(comm_size, 0);

  datatype1 = parse_datatype(action, 4 + 2 * comm_size);
  datatype2 = parse_datatype(action, 5 + 2 * comm_size);

  send_buf_size = parse_integer<int>(action[2]);
  recv_buf_size = parse_integer<int>(action[3 + comm_size]);
  for (unsigned int i = 0; i < comm_size; i++) {
    (*sendcounts)[i] = std::stoi(action[3 + i]);
    (*recvcounts)[i] = std::stoi(action[4 + comm_size + i]);
  }
  send_size_sum = std::accumulate(sendcounts->begin(), sendcounts->end(), 0);
  recv_size_sum = std::accumulate(recvcounts->begin(), recvcounts->end(), 0);
}

void WaitAction::kernel(simgrid::xbt::ReplayAction& action)
{
  std::string s = boost::algorithm::join(action, " ");
  xbt_assert(req_storage.size(), "action wait not preceded by any irecv or isend: %s", s.c_str());
  const WaitTestParser& args = get_args();
  MPI_Request request = req_storage.pop(args.src, args.dst, args.tag);

  if (request == MPI_REQUEST_NULL) {
    /* Assume that the trace is well formed, meaning the comm might have been caught by an MPI_test. Then just
     * return.*/
    return;
  }

  // Must be taken before Request::wait() since the request may be set to
  // MPI_REQUEST_NULL by Request::wait!
  bool is_wait_for_receive = (request->flags() & MPI_REQ_RECV);

  TRACE_smpi_comm_in(get_pid(), __func__, new simgrid::instr::WaitTIData("wait", args.src, args.dst, args.tag));

  MPI_Status status;
  Request::wait(&request, &status);
  if(request!=MPI_REQUEST_NULL)
    Request::unref(&request);
  TRACE_smpi_comm_out(get_pid());
  if (is_wait_for_receive)
    TRACE_smpi_recv(MPI_COMM_WORLD->group()->actor(args.src), MPI_COMM_WORLD->group()->actor(args.dst), args.tag);
}

void SendAction::kernel(simgrid::xbt::ReplayAction&)
{
  const SendOrRecvParser& args = get_args();
  aid_t dst_traced           = MPI_COMM_WORLD->group()->actor(args.partner);

  TRACE_smpi_comm_in(
      get_pid(), __func__,
      new simgrid::instr::Pt2PtTIData(get_name(), args.partner, args.size, args.tag, Datatype::encode(args.datatype1)));
  if (not TRACE_smpi_view_internals())
    TRACE_smpi_send(get_pid(), get_pid(), dst_traced, args.tag, args.size * args.datatype1->size());

  if (get_name() == "send") {
    Request::send(nullptr, args.size, args.datatype1, args.partner, args.tag, MPI_COMM_WORLD);
  } else if (get_name() == "isend") {
    MPI_Request request = Request::isend(nullptr, args.size, args.datatype1, args.partner, args.tag, MPI_COMM_WORLD);
    req_storage.add(request);
  } else {
    xbt_die("Don't know this action, %s", get_name().c_str());
  }

  TRACE_smpi_comm_out(get_pid());
}

void RecvAction::kernel(simgrid::xbt::ReplayAction&)
{
  const SendOrRecvParser& args = get_args();
  TRACE_smpi_comm_in(
      get_pid(), __func__,
      new simgrid::instr::Pt2PtTIData(get_name(), args.partner, args.size, args.tag, Datatype::encode(args.datatype1)));

  MPI_Status status;
  // unknown size from the receiver point of view
  ssize_t arg_size = args.size;
  if (arg_size < 0) {
    Request::probe(args.partner, args.tag, MPI_COMM_WORLD, &status);
    arg_size = status.count;
  }

  bool is_recv = false; // Help analyzers understanding that status is not used uninitialized
  if (get_name() == "recv") {
    is_recv = true;
    Request::recv(nullptr, arg_size, args.datatype1, args.partner, args.tag, MPI_COMM_WORLD, &status);
  } else if (get_name() == "irecv") {
    MPI_Request request = Request::irecv(nullptr, arg_size, args.datatype1, args.partner, args.tag, MPI_COMM_WORLD);
    req_storage.add(request);
  } else {
    THROW_IMPOSSIBLE;
  }

  TRACE_smpi_comm_out(get_pid());
  if (is_recv && not TRACE_smpi_view_internals()) {
    aid_t src_traced = MPI_COMM_WORLD->group()->actor(status.MPI_SOURCE);
    TRACE_smpi_recv(src_traced, get_pid(), args.tag);
  }
}

void SendRecvAction::kernel(simgrid::xbt::ReplayAction&)
{
  XBT_DEBUG("Enters SendRecv");
  const SendRecvParser& args = get_args();
  aid_t my_proc_id = simgrid::s4u::this_actor::get_pid();
  aid_t src_traced = MPI_COMM_WORLD->group()->actor(args.src);
  aid_t dst_traced = MPI_COMM_WORLD->group()->actor(args.dst);

  MPI_Status status;
  int sendtag=0;
  int recvtag=0;

  // FIXME: Hack the way to trace this one
  auto dst_hack = std::make_shared<std::vector<int>>();
  auto src_hack = std::make_shared<std::vector<int>>();
  dst_hack->push_back(dst_traced);
  src_hack->push_back(src_traced);
  TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::VarCollTIData(
                           "sendRecv", -1, args.sendcount,
              		 dst_hack, args.recvcount, src_hack,
                           simgrid::smpi::Datatype::encode(args.datatype1), simgrid::smpi::Datatype::encode(args.datatype2)));

  TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, sendtag, args.sendcount * args.datatype1->size());

  simgrid::smpi::Request::sendrecv(nullptr, args.sendcount, args.datatype1, args.dst, sendtag, nullptr, args.recvcount, args.datatype2, args.src,
                                     recvtag, MPI_COMM_WORLD, &status);

  TRACE_smpi_recv(src_traced, my_proc_id, recvtag);
  TRACE_smpi_comm_out(my_proc_id);
  XBT_DEBUG("Exits SendRecv");
}

void ComputeAction::kernel(simgrid::xbt::ReplayAction&)
{
  const ComputeParser& args = get_args();
  if (smpi_cfg_simulate_computation()) {
    smpi_execute_flops(args.flops/smpi_adjust_comp_speed());
  }
}

void SleepAction::kernel(simgrid::xbt::ReplayAction&)
{
  const SleepParser& args = get_args();
  XBT_DEBUG("Sleep for: %lf secs", args.time);
  aid_t pid = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_sleeping_in(pid, args.time);
  simgrid::s4u::this_actor::sleep_for(args.time/smpi_adjust_comp_speed());
  TRACE_smpi_sleeping_out(pid);
}

void LocationAction::kernel(simgrid::xbt::ReplayAction&)
{
  const LocationParser& args = get_args();
  smpi_trace_set_call_location(args.filename.c_str(), args.line, "replay_action");
}

void TestAction::kernel(simgrid::xbt::ReplayAction&)
{
  const WaitTestParser& args = get_args();
  MPI_Request request = req_storage.pop(args.src, args.dst, args.tag);
  // if request is null here, this may mean that a previous test has succeeded
  // Different times in traced application and replayed version may lead to this
  // In this case, ignore the extra calls.
  if (request != MPI_REQUEST_NULL) {
    TRACE_smpi_comm_in(get_pid(), __func__, new simgrid::instr::WaitTIData("test", args.src, args.dst, args.tag));

    MPI_Status status;
    int flag = 0;
    Request::test(&request, &status, &flag);

    XBT_DEBUG("MPI_Test result: %d", flag);
    /* push back request in vector to be caught by a subsequent wait. if the test did succeed, the request is now
     * nullptr.*/
    if (request == MPI_REQUEST_NULL)
      req_storage.addNullRequest(args.src, args.dst, args.tag);
    else
      req_storage.add(request);

    TRACE_smpi_comm_out(get_pid());
  }
}

void InitAction::kernel(simgrid::xbt::ReplayAction& action)
{
  CHECK_ACTION_PARAMS(action, 0, 1)
    MPI_DEFAULT_TYPE = (action.size() > 2) ? MPI_DOUBLE // default MPE datatype
    : MPI_BYTE;  // default TAU datatype

  /* start a simulated timer */
  smpi_process()->simulated_start();
}

void CommunicatorAction::kernel(simgrid::xbt::ReplayAction&)
{
  /* nothing to do */
}

void WaitAllAction::kernel(simgrid::xbt::ReplayAction&)
{
  if (req_storage.size() > 0) {
    std::vector<std::pair</*sender*/ aid_t, /*recv*/ aid_t>> sender_receiver;
    std::vector<MPI_Request> reqs;
    req_storage.get_requests(reqs);
    unsigned long count_requests = reqs.size();
    TRACE_smpi_comm_in(get_pid(), __func__, new simgrid::instr::CpuTIData("waitall", count_requests));
    for (auto const& req : reqs) {
      if (req && (req->flags() & MPI_REQ_RECV)) {
        sender_receiver.emplace_back(req->src(), req->dst());
      }
    }
    Request::waitall(count_requests, &(reqs.data())[0], MPI_STATUSES_IGNORE);
    req_storage.get_store().clear();

    for (MPI_Request& req : reqs)
      if (req != MPI_REQUEST_NULL)
        Request::unref(&req);

    for (auto const& [src, dst] : sender_receiver) {
      TRACE_smpi_recv(src, dst, 0);
    }
    TRACE_smpi_comm_out(get_pid());
  }
}

void BarrierAction::kernel(simgrid::xbt::ReplayAction&)
{
  TRACE_smpi_comm_in(get_pid(), __func__, new simgrid::instr::NoOpTIData("barrier"));
  colls::barrier(MPI_COMM_WORLD);
  TRACE_smpi_comm_out(get_pid());
}

void BcastAction::kernel(simgrid::xbt::ReplayAction&)
{
  const BcastArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_bcast",
                     new simgrid::instr::CollTIData("bcast", args.root, -1.0, args.size,
                                                    0, Datatype::encode(args.datatype1), ""));

  colls::bcast(send_buffer(args.size * args.datatype1->size()), args.size, args.datatype1, args.root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(get_pid());
}

void ReduceAction::kernel(simgrid::xbt::ReplayAction&)
{
  const ReduceArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_reduce",
                     new simgrid::instr::CollTIData("reduce", args.root, args.comp_size,
                                                    args.comm_size, 0, Datatype::encode(args.datatype1), ""));

  colls::reduce(send_buffer(args.comm_size * args.datatype1->size()),
                recv_buffer(args.comm_size * args.datatype1->size()), args.comm_size, args.datatype1, MPI_OP_NULL,
                args.root, MPI_COMM_WORLD);
  if (args.comp_size != 0.0)
    simgrid::s4u::this_actor::exec_init(args.comp_size)
      ->set_name("computation")
      ->start()
      ->wait();

  TRACE_smpi_comm_out(get_pid());
}

void AllReduceAction::kernel(simgrid::xbt::ReplayAction&)
{
  const AllReduceArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_allreduce",
                     new simgrid::instr::CollTIData("allreduce", -1, args.comp_size, args.comm_size, 0,
                                                    Datatype::encode(args.datatype1), ""));

  colls::allreduce(send_buffer(args.comm_size * args.datatype1->size()),
                   recv_buffer(args.comm_size * args.datatype1->size()), args.comm_size, args.datatype1, MPI_OP_NULL,
                   MPI_COMM_WORLD);
  if (args.comp_size != 0.0)
    simgrid::s4u::this_actor::exec_init(args.comp_size)
      ->set_name("computation")
      ->start()
      ->wait();

  TRACE_smpi_comm_out(get_pid());
}

void AllToAllAction::kernel(simgrid::xbt::ReplayAction&)
{
  const AllToAllArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_alltoall",
                     new simgrid::instr::CollTIData("alltoall", -1, -1.0, args.send_size, args.recv_size,
                                                    Datatype::encode(args.datatype1),
                                                    Datatype::encode(args.datatype2)));

  colls::alltoall(send_buffer(args.datatype1->size() * args.send_size * args.comm_size), args.send_size, args.datatype1,
                  recv_buffer(args.datatype2->size() * args.recv_size * args.comm_size), args.recv_size, args.datatype2,
                  MPI_COMM_WORLD);

  TRACE_smpi_comm_out(get_pid());
}

void GatherAction::kernel(simgrid::xbt::ReplayAction&)
{
  const GatherArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), get_name().c_str(),
                     new simgrid::instr::CollTIData(get_name(), (get_name() == "gather") ? args.root : -1, -1.0,
                                                    args.send_size, args.recv_size, Datatype::encode(args.datatype1),
                                                    Datatype::encode(args.datatype2)));

  if (get_name() == "gather") {
    int rank = MPI_COMM_WORLD->rank();
    colls::gather(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                  (rank == args.root) ? recv_buffer(args.datatype2->size() * args.recv_size * args.comm_size) : nullptr,
                  args.recv_size, args.datatype2, args.root, MPI_COMM_WORLD);
  } else
    colls::allgather(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                     recv_buffer(args.recv_size * args.datatype2->size()), args.recv_size, args.datatype2,
                     MPI_COMM_WORLD);

  TRACE_smpi_comm_out(get_pid());
}

void GatherVAction::kernel(simgrid::xbt::ReplayAction&)
{
  int rank = MPI_COMM_WORLD->rank();
  const GatherVArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), get_name().c_str(),
                     new simgrid::instr::VarCollTIData(
                         get_name(), (get_name() == "gatherv") ? args.root : -1, args.send_size, nullptr, -1,
                         args.recvcounts, Datatype::encode(args.datatype1), Datatype::encode(args.datatype2)));

  if (get_name() == "gatherv") {
    colls::gatherv(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                   (rank == args.root) ? recv_buffer(args.recv_size_sum * args.datatype2->size()) : nullptr,
                   args.recvcounts->data(), args.disps.data(), args.datatype2, args.root, MPI_COMM_WORLD);
  } else {
    colls::allgatherv(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                      recv_buffer(args.recv_size_sum * args.datatype2->size()), args.recvcounts->data(),
                      args.disps.data(), args.datatype2, MPI_COMM_WORLD);
  }

  TRACE_smpi_comm_out(get_pid());
}

void ScatterAction::kernel(simgrid::xbt::ReplayAction&)
{
  int rank = MPI_COMM_WORLD->rank();
  const ScatterArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_scatter",
                     new simgrid::instr::CollTIData(get_name(), args.root, -1.0, args.send_size, args.recv_size,
                                                    Datatype::encode(args.datatype1),
                                                    Datatype::encode(args.datatype2)));

  colls::scatter(send_buffer(args.send_size * args.datatype1->size()), args.send_size, args.datatype1,
                 (rank == args.root) ? recv_buffer(args.recv_size * args.datatype2->size()) : nullptr, args.recv_size,
                 args.datatype2, args.root, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(get_pid());
}

void ScatterVAction::kernel(simgrid::xbt::ReplayAction&)
{
  int rank = MPI_COMM_WORLD->rank();
  const ScatterVArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_scatterv",
                     new simgrid::instr::VarCollTIData(get_name(), args.root, -1, args.sendcounts, args.recv_size,
                                                       nullptr, Datatype::encode(args.datatype1),
                                                       Datatype::encode(args.datatype2)));

  colls::scatterv((rank == args.root) ? send_buffer(args.send_size_sum * args.datatype1->size()) : nullptr,
                  args.sendcounts->data(), args.disps.data(), args.datatype1,
                  recv_buffer(args.recv_size * args.datatype2->size()), args.recv_size, args.datatype2, args.root,
                  MPI_COMM_WORLD);

  TRACE_smpi_comm_out(get_pid());
}

void ReduceScatterAction::kernel(simgrid::xbt::ReplayAction&)
{
  const ReduceScatterArgParser& args = get_args();
  TRACE_smpi_comm_in(
      get_pid(), "action_reducescatter",
      new simgrid::instr::VarCollTIData(get_name(), -1, -1, nullptr, -1, args.recvcounts,
                                        /* ugly as we use datatype field to pass computation as string */
                                        /* and because of the trick to avoid getting 0.000000 when 0 is given */
                                        args.comp_size == 0 ? "0" : std::to_string(args.comp_size),
                                        Datatype::encode(args.datatype1)));

  colls::reduce_scatter(send_buffer(args.recv_size_sum * args.datatype1->size()),
                        recv_buffer(args.recv_size_sum * args.datatype1->size()), args.recvcounts->data(),
                        args.datatype1, MPI_OP_NULL, MPI_COMM_WORLD);
  if (args.comp_size != 0.0)
    simgrid::s4u::this_actor::exec_init(args.comp_size)
      ->set_name("computation")
      ->start()
      ->wait();
  TRACE_smpi_comm_out(get_pid());
}

void ScanAction::kernel(simgrid::xbt::ReplayAction&)
{
  const ScanArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), "action_scan",
                     new simgrid::instr::CollTIData(get_name(), -1, args.comp_size,
                     args.size, 0, Datatype::encode(args.datatype1), ""));
  if (get_name() == "scan")
    colls::scan(send_buffer(args.size * args.datatype1->size()),
              recv_buffer(args.size * args.datatype1->size()), args.size,
              args.datatype1, MPI_OP_NULL, MPI_COMM_WORLD);
  else
    colls::exscan(send_buffer(args.size * args.datatype1->size()),
              recv_buffer(args.size * args.datatype1->size()), args.size,
              args.datatype1, MPI_OP_NULL, MPI_COMM_WORLD);

  if (args.comp_size != 0.0)
    simgrid::s4u::this_actor::exec_init(args.comp_size)
      ->set_name("computation")
      ->start()
      ->wait();
  TRACE_smpi_comm_out(get_pid());
}

void AllToAllVAction::kernel(simgrid::xbt::ReplayAction&)
{
  const AllToAllVArgParser& args = get_args();
  TRACE_smpi_comm_in(get_pid(), __func__,
                     new simgrid::instr::VarCollTIData(
                         "alltoallv", -1, args.send_size_sum, args.sendcounts, args.recv_size_sum, args.recvcounts,
                         Datatype::encode(args.datatype1), Datatype::encode(args.datatype2)));

  colls::alltoallv(send_buffer(args.send_buf_size * args.datatype1->size()), args.sendcounts->data(),
                   args.senddisps.data(), args.datatype1, recv_buffer(args.recv_buf_size * args.datatype2->size()),
                   args.recvcounts->data(), args.recvdisps.data(), args.datatype2, MPI_COMM_WORLD);

  TRACE_smpi_comm_out(get_pid());
}
} // namespace simgrid::smpi::replay

static std::unordered_map<aid_t, simgrid::smpi::replay::RequestStorage> storage;
/** @brief Only initialize the replay, don't do it for real */
void smpi_replay_init(const char* instance_id, int rank, double start_delay_flops)
{
  xbt_assert(not smpi_process()->initializing());

  simgrid::s4u::Actor::self()->set_property("instance_id", instance_id);
  simgrid::s4u::Actor::self()->set_property("rank", std::to_string(rank));
  simgrid::smpi::ActorExt::init();

  smpi_process()->mark_as_initialized();
  smpi_process()->set_replaying(true);

  TRACE_smpi_init(simgrid::s4u::this_actor::get_pid(), "smpi_replay_run_init");
  xbt_replay_action_register("init", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::InitAction().execute(action); });
  xbt_replay_action_register("finalize", [](simgrid::xbt::ReplayAction const&) { /* nothing to do */ });
  xbt_replay_action_register("comm_size", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::CommunicatorAction().execute(action); });
  xbt_replay_action_register("comm_split",[](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::CommunicatorAction().execute(action); });
  xbt_replay_action_register("comm_dup",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::CommunicatorAction().execute(action); });
  xbt_replay_action_register("send",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::SendAction("send", storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("isend", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::SendAction("isend", storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("recv",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::RecvAction("recv", storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("irecv", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::RecvAction("irecv", storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("test",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::TestAction(storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("sendRecv", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::SendRecvAction().execute(action); });
  xbt_replay_action_register("wait",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::WaitAction(storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("waitall", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::WaitAllAction(storage[simgrid::s4u::this_actor::get_pid()]).execute(action); });
  xbt_replay_action_register("barrier", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::BarrierAction().execute(action); });
  xbt_replay_action_register("bcast",   [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::BcastAction().execute(action); });
  xbt_replay_action_register("reduce",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ReduceAction().execute(action); });
  xbt_replay_action_register("allreduce", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::AllReduceAction().execute(action); });
  xbt_replay_action_register("alltoall", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::AllToAllAction().execute(action); });
  xbt_replay_action_register("alltoallv", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::AllToAllVAction().execute(action); });
  xbt_replay_action_register("gather",   [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::GatherAction("gather").execute(action); });
  xbt_replay_action_register("scatter",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ScatterAction().execute(action); });
  xbt_replay_action_register("gatherv",  [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::GatherVAction("gatherv").execute(action); });
  xbt_replay_action_register("scatterv", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ScatterVAction().execute(action); });
  xbt_replay_action_register("allgather", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::GatherAction("allgather").execute(action); });
  xbt_replay_action_register("allgatherv", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::GatherVAction("allgatherv").execute(action); });
  xbt_replay_action_register("reducescatter", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ReduceScatterAction().execute(action); });
  xbt_replay_action_register("scan", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ScanAction("scan").execute(action); });
  xbt_replay_action_register("exscan", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ScanAction("exscan").execute(action); });
  xbt_replay_action_register("compute", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::ComputeAction().execute(action); });
  xbt_replay_action_register("sleep", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::SleepAction().execute(action); });
  xbt_replay_action_register("location", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::replay::LocationAction().execute(action); });

  //if we have a delayed start, sleep here.
  if (start_delay_flops > 0) {
    XBT_VERB("Delayed start for instance - Sleeping for %f flops ", start_delay_flops);
    private_execute_flops(start_delay_flops);
  } else {
    // Wait for the other actors to initialize also
    simgrid::s4u::this_actor::yield();
  }
  if(_smpi_init_sleep > 0)
    simgrid::s4u::this_actor::sleep_for(_smpi_init_sleep);
}

/** @brief actually run the replay after initialization */
void smpi_replay_main(int rank, const char* private_trace_filename)
{
  static int active_processes = 0;
  active_processes++;
  storage[simgrid::s4u::this_actor::get_pid()] = simgrid::smpi::replay::RequestStorage();
  std::string rank_string                      = std::to_string(rank);
  simgrid::xbt::replay_runner(rank_string.c_str(), private_trace_filename);

  /* and now, finalize everything */
  /* One active process will stop. Decrease the counter*/
  unsigned int count_requests = storage[simgrid::s4u::this_actor::get_pid()].size();
  XBT_DEBUG("There are %u elements in reqq[*]", count_requests);
  if (count_requests > 0) {
    std::vector<MPI_Request> requests(count_requests);
    unsigned int i=0;

    for (auto const& [_, reqs] : storage[simgrid::s4u::this_actor::get_pid()].get_store()) {
      for (const auto& req : reqs) {
        requests[i] = req; // FIXME: overwritten at each iteration?
      }
      i++;
    }
    simgrid::smpi::Request::waitall(count_requests, requests.data(), MPI_STATUSES_IGNORE);
  }

  if (simgrid::config::get_value<bool>("smpi/barrier-finalization"))
    simgrid::smpi::colls::barrier(MPI_COMM_WORLD);

  active_processes--;

  if(active_processes==0){
    /* Last process alive speaking: end the simulated timer */
    XBT_INFO("Simulation time %f", smpi_process()->simulated_elapsed());
    smpi_free_replay_tmp_buffers();
  }

  TRACE_smpi_comm_in(simgrid::s4u::this_actor::get_pid(), "smpi_replay_run_finalize",
                     new simgrid::instr::NoOpTIData("finalize"));

  smpi_process()->finalize();

  TRACE_smpi_comm_out(simgrid::s4u::this_actor::get_pid());
}

/** @brief chain a replay initialization and a replay start */
void smpi_replay_run(const char* instance_id, int rank, double start_delay_flops, const char* private_trace_filename)
{
  smpi_replay_init(instance_id, rank, start_delay_flops);
  smpi_replay_main(rank, private_trace_filename);
}
