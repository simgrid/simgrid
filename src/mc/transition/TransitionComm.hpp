/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_COMM_HPP
#define SIMGRID_MC_TRANSITION_COMM_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"

#include <sstream>
#include <string>

namespace simgrid {
namespace mc {

class CommRecvTransition;
class CommSendTransition;
class CommTestTransition;

class CommWaitTransition : public Transition {
  bool timeout_;
  uintptr_t comm_;
  aid_t sender_;
  aid_t receiver_;
  unsigned mbox_;
  uintptr_t sbuff_;
  uintptr_t rbuff_;
  size_t size_;
  friend CommRecvTransition;
  friend CommSendTransition;
  friend CommTestTransition;

public:
  CommWaitTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;

  bool get_timeout() const { return timeout_; }
  /** Address of the corresponding Communication object in the application */
  uintptr_t get_comm() const { return comm_; }
  /** Sender ID */
  aid_t get_sender() const { return sender_; }
  /** Receiver ID */
  aid_t get_receiver() const { return receiver_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** Sender buffer */
  uintptr_t get_sbuff() const { return sbuff_; }
  /** Receiver buffer */
  uintptr_t get_rbuff() const { return rbuff_; }
  /** data size */
  size_t get_size() const { return size_; }
};
class CommTestTransition : public Transition {
  uintptr_t comm_;
  aid_t sender_;
  aid_t receiver_;
  unsigned mbox_;
  uintptr_t sbuff_;
  uintptr_t rbuff_;
  size_t size_;
  friend CommSendTransition;
  friend CommRecvTransition;

public:
  CommTestTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;

  /** Address of the corresponding Communication object in the application */
  uintptr_t get_comm() const { return comm_; }
  /** Sender ID */
  aid_t get_sender() const { return sender_; }
  /** Receiver ID */
  aid_t get_receiver() const { return receiver_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** Sender buffer */
  uintptr_t get_sbuff() const { return sbuff_; }
  /** Receiver buffer */
  uintptr_t get_rbuff() const { return rbuff_; }
  /** data size */
  size_t get_size() const { return size_; }
};

class CommRecvTransition : public Transition {
  uintptr_t comm_; /* Addr of the CommImpl */
  unsigned mbox_;
  uintptr_t rbuff_;
  int tag_;

public:
  CommRecvTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;

  /** Address of the corresponding Communication object in the application */
  uintptr_t get_comm() const { return comm_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** Receiver buffer */
  uintptr_t get_rbuff() const { return rbuff_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

class CommSendTransition : public Transition {
  uintptr_t comm_; /* Addr of the CommImpl */
  unsigned mbox_;
  uintptr_t sbuff_;
  size_t size_;
  int tag_;

public:
  CommSendTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;

  /** Address of the corresponding Communication object in the application */
  uintptr_t get_comm() const { return comm_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** Sender buffer */
  uintptr_t get_sbuff() const { return sbuff_; }
  /** data size */
  size_t get_size() const { return size_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

/** Make a new transition from serialized description */
Transition* deserialize_transition(aid_t issuer, int times_considered, std::stringstream& stream);

} // namespace mc
} // namespace simgrid

#endif
