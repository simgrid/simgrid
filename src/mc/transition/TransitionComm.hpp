/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_COMM_HPP
#define SIMGRID_MC_TRANSITION_COMM_HPP

#include "simgrid/s4u/Mailbox.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"

#include <cstdint>
#include <string>

namespace simgrid::mc {

class CommRecvTransition;
class CommSendTransition;
class CommTestTransition;

class CommWaitTransition final : public Transition {
  bool timeout_;
  unsigned comm_;
  unsigned mbox_;
  Aid sender_;
  Aid receiver_;
  friend CommRecvTransition;
  friend CommSendTransition;
  friend CommTestTransition;

public:
  CommWaitTransition(Aid issuer, int times_considered, bool timeout_, unsigned comm_, Aid sender_, Aid receiver_,
                     unsigned mbox_);
  CommWaitTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  bool is_enabled() const { return sender_.has_value() and receiver_.has_value(); }
  bool get_timeout() const { return timeout_; }
  /** ID of the corresponding Communication object in the application, or 0 if unknown */
  unsigned get_comm() const { return comm_; }
  /** Sender ID */
  Aid get_sender() const { return sender_; }
  /** Receiver ID */
  Aid get_receiver() const { return receiver_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
};
class CommTestTransition final : public Transition {
  unsigned comm_;
  unsigned mbox_;
  Aid sender_;
  Aid receiver_;
  friend CommSendTransition;
  friend CommRecvTransition;

public:
  CommTestTransition(Aid issuer, int times_considered, unsigned comm_, Aid sender_, Aid receiver_, unsigned mbox_);
  CommTestTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  /** ID of the corresponding Communication object in the application, or 0 if unknown */
  unsigned get_comm() const { return comm_; }
  /** Sender ID */
  Aid get_sender() const { return sender_; }
  /** Receiver ID */
  Aid get_receiver() const { return receiver_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
};

class CommIprobeTransition final : public Transition {
  bool is_sender_;
  unsigned mbox_;
  int tag_;

public:
  CommIprobeTransition(Aid issuer, int times_considered, bool is_sender, unsigned mbox, int tag);
  CommIprobeTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
  bool can_be_co_enabled(const Transition* o) const override;

  bool is_sender_side() const { return is_sender_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

class CommRecvTransition final : public Transition {
  unsigned comm_; /* ID of the CommImpl or 0 if not known */
  unsigned mbox_;
  int tag_;

public:
  CommRecvTransition(Aid issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_);
  CommRecvTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;

  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  /** ID of the corresponding Communication object in the application (or 0 if unknown)*/
  unsigned get_comm() const { return comm_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

class CommSendTransition final : public Transition {
  unsigned comm_;
  unsigned mbox_;
  int tag_;

public:
  CommSendTransition(Aid issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_);
  CommSendTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  /** ID of the corresponding Communication object in the application, or 0 if unknown */
  unsigned get_comm() const { return comm_; }
  /** Mailbox ID */
  unsigned get_mailbox() const { return mbox_; }
  /** If using SMPI, the tag */
  int get_tag() const { return tag_; }
};

} // namespace simgrid::mc

#endif
