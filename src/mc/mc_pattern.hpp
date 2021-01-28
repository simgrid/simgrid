/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PATTERN_H
#define SIMGRID_MC_PATTERN_H

#include "src/kernel/activity/CommImpl.hpp"

namespace simgrid {
namespace mc {

enum class PatternCommunicationType {
  none    = 0,
  send    = 1,
  receive = 2,
};

class PatternCommunication {
public:
  int num = 0;
  simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr;
  PatternCommunicationType type = PatternCommunicationType::send;
  unsigned long src_proc        = 0;
  unsigned long dst_proc        = 0;
  const char* src_host          = nullptr;
  const char* dst_host          = nullptr;
  std::string rdv;
  std::vector<char> data;
  int tag   = 0;
  int index = 0;

  PatternCommunication() { std::memset(&comm_addr, 0, sizeof(comm_addr)); }

  PatternCommunication dup() const
  {
    simgrid::mc::PatternCommunication res;
    // num?
    res.comm_addr = this->comm_addr;
    res.type      = this->type;
    // src_proc?
    // dst_proc?
    res.dst_proc = this->dst_proc;
    res.dst_host = this->dst_host;
    res.rdv      = this->rdv;
    res.data     = this->data;
    // tag?
    res.index = this->index;
    return res;
  }
};

/* On every state, each actor has an entry of the following type.
 * This represents both the actor and its transition because
 *   an actor cannot have more than one enabled transition at a given time.
 */
class ActorState {
  /* Possible exploration status of an actor transition in a state.
   * Either the checker did not consider the transition, or it was considered and still to do, or considered and done.
   */
  enum class InterleavingType {
    /** This actor transition is not considered by the checker (yet?) */
    disabled = 0,
    /** The checker algorithm decided that this actor transitions should be done at some point */
    todo,
    /** The checker algorithm decided that this should be done, but it was done in the meanwhile */
    done,
  };

  /** Exploration control information */
  InterleavingType state = InterleavingType::disabled;

public:
  /** Number of times that the process was considered to be executed */
  // TODO, make this private
  unsigned int times_considered = 0;

  bool is_disabled() const { return this->state == InterleavingType::disabled; }
  bool is_done() const { return this->state == InterleavingType::done; }
  bool is_todo() const { return this->state == InterleavingType::todo; }
  /** Mark that we should try executing this process at some point in the future of the checker algorithm */
  void consider()
  {
    this->state            = InterleavingType::todo;
    this->times_considered = 0;
  }
  void set_done() { this->state = InterleavingType::done; }
};

} // namespace mc
} // namespace simgrid

#endif
