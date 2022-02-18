/* Copyright (c) 2015-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_COMM_HPP
#define SIMGRID_MC_TRANSITION_COMM_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/api/Transition.hpp"

#include <sstream>
#include <string>

namespace simgrid {
namespace mc {

class CommRecvTransition;
class CommSendTransition;
class CommTestTransition;

class CommWaitTransition : public Transition {
  bool timeout_;
  void* comm_;
  aid_t sender_;
  aid_t receiver_;
  unsigned mbox_;
  void* src_buff_;
  void* dst_buff_;
  size_t size_;
  friend CommRecvTransition;
  friend CommSendTransition;
  friend CommTestTransition;

public:
  CommWaitTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
};
class CommTestTransition : public Transition {
  void* comm_;
  aid_t sender_;
  aid_t receiver_;
  unsigned mbox_;
  void* src_buff_;
  void* dst_buff_;
  size_t size_;
  friend CommSendTransition;
  friend CommRecvTransition;

public:
  CommTestTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
};

class CommRecvTransition : public Transition {
  void* comm_; /* Addr of the CommImpl */
  unsigned mbox_;
  void* dst_buff_;

public:
  CommRecvTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
};

class CommSendTransition : public Transition {
  void* comm_; /* Addr of the CommImpl */
  unsigned mbox_;
  void* src_buff_;
  size_t size_;

public:
  CommSendTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
};

class TestAnyTransition : public Transition {
  std::vector<Transition*> transitions_;

public:
  TestAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
};

class WaitAnyTransition : public Transition {
  std::vector<Transition*> transitions_;

public:
  WaitAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
};

/** Make a new transition from serialized description */
Transition* deserialize_transition(aid_t issuer, int times_considered, std::stringstream& stream);

} // namespace mc
} // namespace simgrid

#endif
