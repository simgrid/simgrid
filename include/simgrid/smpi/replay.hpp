/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "private.hpp"
#include <xbt/replay.hpp>

#include <sstream>

#define CHECK_ACTION_PARAMS(action, mandatory, optional)                                                             \
{                                                                                                                    \
  if (action.size() < static_cast<unsigned long>(mandatory + 2)) {                                                   \
    std::stringstream ss;                                                                                            \
    for (const auto& elem : action) {                                                                                \
      ss << elem << " ";                                                                                             \
    }                                                                                                                \
    THROWF(arg_error, 0, "%s replay failed.\n"                                                                       \
                         "%zu items were given on the line. First two should be process_id and action.  "            \
                         "This action needs after them %lu mandatory arguments, and accepts %lu optional ones. \n"   \
                         "The full line that was given is:\n   %s\n"                                                 \
                         "Please contact the Simgrid team if support is needed",                                     \
           __func__, action.size(), static_cast<unsigned long>(mandatory), static_cast<unsigned long>(optional),     \
           ss.str().c_str());                                                                                        \
  }                                                                                                                  \
}

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
  virtual void parse(simgrid::xbt::ReplayAction& action, std::string name) { CHECK_ACTION_PARAMS(action, 0, 0) }
};

class WaitTestParser : public ActionArgParser {
public:
  int src;
  int dst;
  int tag;

  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class SendRecvParser : public ActionArgParser {
public:
  /* communication partner; if we send, this is the receiver and vice versa */
  int partner;
  double size;
  int tag;
  MPI_Datatype datatype1 = MPI_DEFAULT_TYPE;

  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class ComputeParser : public ActionArgParser {
public:
  /* communication partner; if we send, this is the receiver and vice versa */
  double flops;

  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
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

  virtual void parse(simgrid::xbt::ReplayAction& action, std::string name) = 0;
};

class BcastArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class ReduceArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class AllReduceArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class AllToAllArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class GatherArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class GatherVArgParser : public CollCommParser {
public:
  int recv_size_sum;
  std::shared_ptr<std::vector<int>> recvcounts;
  std::vector<int> disps;
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class ScatterArgParser : public CollCommParser {
public:
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class ScatterVArgParser : public CollCommParser {
public:
  int recv_size_sum;
  int send_size_sum;
  std::shared_ptr<std::vector<int>> sendcounts;
  std::vector<int> disps;
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};

class ReduceScatterArgParser : public CollCommParser {
public:
  int recv_size_sum;
  std::shared_ptr<std::vector<int>> recvcounts;
  std::vector<int> disps;
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
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
  void parse(simgrid::xbt::ReplayAction& action, std::string name) override;
};


}
}
}
