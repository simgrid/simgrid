/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_COMM_HPP
#define SIMGRID_MC_TRANSITION_COMM_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace simgrid::mc {

class CommRecvTransition;
class CommSendTransition;
class CommTestTransition;

class CommWaitTransition : public Transition {
  bool timeout_;
  unsigned comm_;
  unsigned mbox_;
  aid_t sender_;
  aid_t receiver_;
  friend CommRecvTransition;
  friend CommSendTransition;
  friend CommTestTransition;

public:
  CommWaitTransition(aid_t issuer, int times_considered, bool timeout_, unsigned comm_, aid_t sender_, aid_t receiver_,
                     unsigned mbox_);
  CommWaitTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  bool get_timeout() const { return timeout_; }
  /** ID of the corresponding Communication object in the application, or 0 if unknown */
  unsigned get_comm() const { return comm_; }
  /** Sender ID */
  aid_t get_sender() const { return sender_; }
  /** Receiver ID */
  aid_t get_receiver() const { return receiver_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
};
class CommTestTransition : public Transition {
  unsigned comm_;
  unsigned mbox_;
  aid_t sender_;
  aid_t receiver_;
  friend CommSendTransition;
  friend CommRecvTransition;

public:
  CommTestTransition(aid_t issuer, int times_considered, unsigned comm_, aid_t sender_, aid_t receiver_,
                     unsigned mbox_);
  CommTestTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  /** ID of the corresponding Communication object in the application, or 0 if unknown */
  unsigned get_comm() const { return comm_; }
  /** Sender ID */
  aid_t get_sender() const { return sender_; }
  /** Receiver ID */
  aid_t get_receiver() const { return receiver_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
};

class CommRecvTransition : public Transition {
  unsigned comm_; /* ID of the CommImpl or 0 if not known */
  unsigned mbox_;
  int tag_;

public:
  CommRecvTransition(aid_t issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_);
  CommRecvTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  /** ID of the corresponding Communication object in the application (or 0 if unknown)*/
  unsigned get_comm() const { return comm_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

class CommSendTransition : public Transition {
  unsigned comm_;
  unsigned mbox_;
  int tag_;

public:
  CommSendTransition(aid_t issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_);
  CommSendTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  /** ID of the corresponding Communication object in the application, or 0 if unknown */
  unsigned get_comm() const { return comm_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

/** Make a new transition from serialized description */
Transition* deserialize_transition(aid_t issuer, int times_considered, std::stringstream& stream);

} // namespace simgrid::mc

#endif
