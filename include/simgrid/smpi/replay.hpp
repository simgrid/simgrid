/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef SMPI_REPLAY_HPP_
#define SMPI_REPLAY_HPP_

#include "src/smpi/include/smpi_actor.hpp"

#include <boost/algorithm/string/join.hpp>
#include <xbt/replay.hpp>
#include <xbt/ex.h>

#include <memory>
#include <sstream>

#define CHECK_ACTION_PARAMS(action, mandatory, optional)                                                               \
  {                                                                                                                    \
    if ((action).size() < static_cast<unsigned long>((mandatory) + 2)) {                                               \
      std::stringstream ss;                                                                                            \
      ss << __func__ << " replay failed.\n"                                                                            \
         << (action).size() << " items were given on the line. First two should be process_id and action.  "           \
         << "This action needs after them " << (mandatory) << " mandatory arguments, and accepts " << (optional)       \
         << " optional ones. \n"                                                                                       \
         << "The full line that was given is:\n   ";                                                                   \
      for (const auto& elem : (action)) {                                                                              \
        ss << elem << " ";                                                                                             \
      }                                                                                                                \
      ss << "\nPlease contact the Simgrid team if support is needed";                                                  \
      throw std::invalid_argument(ss.str());                                                                           \
    }                                                                                                                  \
  }

XBT_PRIVATE unsigned char* smpi_get_tmp_sendbuffer(size_t size);
XBT_PRIVATE unsigned char* smpi_get_tmp_recvbuffer(size_t size);

XBT_PRIVATE void log_timed_action(const simgrid::xbt::ReplayAction& action, double clock);

namespace simgrid {
namespace smpi {
namespace replay {
extern MPI_Datatype MPI_DEFAULT_TYPE;

class RequestStorage; // Forward decl

/**
 * Base class for all parsers.
 */
class ActionArgParser {
public:
  virtual ~ActionArgParser() = default;
  virtual void parse(xbt::ReplayAction& action, const std::string& name) { CHECK_ACTION_PARAMS(action, 0, 0) }
};

class WaitTestParser : public ActionArgParser {
public:
  int src;
  int dst;
  int tag;

  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class SendRecvParser : public ActionArgParser {
public:
  /* communication partner; if we send, this is the receiver and vice versa */
  int partner;
  double size;
  int tag;
  MPI_Datatype datatype1 = MPI_DEFAULT_TYPE;

  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class ComputeParser : public ActionArgParser {
public:
  double flops;

  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class SleepParser : public ActionArgParser {
public:
  double time;

  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class LocationParser : public ActionArgParser {
public:
  std::string filename;
  int line;

  void parse(xbt::ReplayAction& action, const std::string& name) override;
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
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class ReduceArgParser : public CollCommParser {
public:
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class AllReduceArgParser : public CollCommParser {
public:
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class AllToAllArgParser : public CollCommParser {
public:
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class GatherArgParser : public CollCommParser {
public:
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class GatherVArgParser : public CollCommParser {
public:
  int recv_size_sum;
  std::shared_ptr<std::vector<int>> recvcounts;
  std::vector<int> disps;
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class ScatterArgParser : public CollCommParser {
public:
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class ScatterVArgParser : public CollCommParser {
public:
  int recv_size_sum;
  int send_size_sum;
  std::shared_ptr<std::vector<int>> sendcounts;
  std::vector<int> disps;
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class ReduceScatterArgParser : public CollCommParser {
public:
  int recv_size_sum;
  std::shared_ptr<std::vector<int>> recvcounts;
  std::vector<int> disps;
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

class AllToAllVArgParser : public CollCommParser {
public:
  int recv_size_sum;
  int send_size_sum;
  std::shared_ptr<std::vector<int>> recvcounts;
  std::shared_ptr<std::vector<int>> sendcounts;
  std::vector<int> senddisps;
  std::vector<int> recvdisps;
  int send_buf_size;
  int recv_buf_size;
  void parse(xbt::ReplayAction& action, const std::string& name) override;
};

/**
 * Base class for all ReplayActions.
 * Note that this class actually implements the behavior of each action
 * while the parsing of the replay arguments is done in the @ref ActionArgParser class.
 * In other words: The logic goes here, the setup is done by the ActionArgParser.
 */
template <class T> class ReplayAction {
  const std::string name_;
  const aid_t my_proc_id_ = s4u::this_actor::get_pid();
  T args_;

protected:
  const std::string& get_name() const { return name_; }
  aid_t get_pid() const { return my_proc_id_; }
  const T& get_args() const { return args_; }

public:
  explicit ReplayAction(const std::string& name) : name_(name) {}
  virtual ~ReplayAction() = default;

  void execute(xbt::ReplayAction& action)
  {
    // Needs to be re-initialized for every action, hence here
    double start_time = smpi_process()->simulated_elapsed();
    args_.parse(action, name_);
    kernel(action);
    if (name_ != "Init")
      log_timed_action(action, start_time);
  }

  virtual void kernel(simgrid::xbt::ReplayAction& action) = 0;
  unsigned char* send_buffer(int size) { return smpi_get_tmp_sendbuffer(size); }
  unsigned char* recv_buffer(int size) { return smpi_get_tmp_recvbuffer(size); }
};

class WaitAction : public ReplayAction<WaitTestParser> {
  RequestStorage& req_storage;

public:
  explicit WaitAction(RequestStorage& storage) : ReplayAction("Wait"), req_storage(storage) {}
  void kernel(xbt::ReplayAction& action) override;
};

class SendAction : public ReplayAction<SendRecvParser> {
  RequestStorage& req_storage;

public:
  explicit SendAction(const std::string& name, RequestStorage& storage) : ReplayAction(name), req_storage(storage) {}
  void kernel(xbt::ReplayAction& action) override;
};

class RecvAction : public ReplayAction<SendRecvParser> {
  RequestStorage& req_storage;

public:
  explicit RecvAction(const std::string& name, RequestStorage& storage) : ReplayAction(name), req_storage(storage) {}
  void kernel(xbt::ReplayAction& action) override;
};

class ComputeAction : public ReplayAction<ComputeParser> {
public:
  explicit ComputeAction() : ReplayAction("compute") {}
  void kernel(xbt::ReplayAction& action) override;
};

class SleepAction : public ReplayAction<SleepParser> {
public:
  explicit SleepAction() : ReplayAction("sleep") {}
  void kernel(xbt::ReplayAction& action) override;
};

class LocationAction : public ReplayAction<LocationParser> {
public:
  explicit LocationAction() : ReplayAction("location") {}
  void kernel(xbt::ReplayAction& action) override;
};

class TestAction : public ReplayAction<WaitTestParser> {
private:
  RequestStorage& req_storage;

public:
  explicit TestAction(RequestStorage& storage) : ReplayAction("Test"), req_storage(storage) {}
  void kernel(xbt::ReplayAction& action) override;
};

class InitAction : public ReplayAction<ActionArgParser> {
public:
  explicit InitAction() : ReplayAction("Init") {}
  void kernel(xbt::ReplayAction& action) override;
};

class CommunicatorAction : public ReplayAction<ActionArgParser> {
public:
  explicit CommunicatorAction() : ReplayAction("Comm") {}
  void kernel(xbt::ReplayAction& action) override;
};

class WaitAllAction : public ReplayAction<ActionArgParser> {
  RequestStorage& req_storage;

public:
  explicit WaitAllAction(RequestStorage& storage) : ReplayAction("waitall"), req_storage(storage) {}
  void kernel(xbt::ReplayAction& action) override;
};

class BarrierAction : public ReplayAction<ActionArgParser> {
public:
  explicit BarrierAction() : ReplayAction("barrier") {}
  void kernel(xbt::ReplayAction& action) override;
};

class BcastAction : public ReplayAction<BcastArgParser> {
public:
  explicit BcastAction() : ReplayAction("bcast") {}
  void kernel(xbt::ReplayAction& action) override;
};

class ReduceAction : public ReplayAction<ReduceArgParser> {
public:
  explicit ReduceAction() : ReplayAction("reduce") {}
  void kernel(xbt::ReplayAction& action) override;
};

class AllReduceAction : public ReplayAction<AllReduceArgParser> {
public:
  explicit AllReduceAction() : ReplayAction("allreduce") {}
  void kernel(xbt::ReplayAction& action) override;
};

class AllToAllAction : public ReplayAction<AllToAllArgParser> {
public:
  explicit AllToAllAction() : ReplayAction("alltoall") {}
  void kernel(xbt::ReplayAction& action) override;
};

class GatherAction : public ReplayAction<GatherArgParser> {
public:
  using ReplayAction::ReplayAction;
  void kernel(xbt::ReplayAction& action) override;
};

class GatherVAction : public ReplayAction<GatherVArgParser> {
public:
  using ReplayAction::ReplayAction;
  void kernel(xbt::ReplayAction& action) override;
};

class ScatterAction : public ReplayAction<ScatterArgParser> {
public:
  explicit ScatterAction() : ReplayAction("scatter") {}
  void kernel(xbt::ReplayAction& action) override;
};

class ScatterVAction : public ReplayAction<ScatterVArgParser> {
public:
  explicit ScatterVAction() : ReplayAction("scatterv") {}
  void kernel(xbt::ReplayAction& action) override;
};

class ReduceScatterAction : public ReplayAction<ReduceScatterArgParser> {
public:
  explicit ReduceScatterAction() : ReplayAction("reducescatter") {}
  void kernel(xbt::ReplayAction& action) override;
};

class AllToAllVAction : public ReplayAction<AllToAllVArgParser> {
public:
  explicit AllToAllVAction() : ReplayAction("alltoallv") {}
  void kernel(xbt::ReplayAction& action) override;
};

} // namespace replay
} // namespace smpi
} // namespace simgrid

#endif
