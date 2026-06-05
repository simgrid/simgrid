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
  bool depends(const Transition* other) const override
  {
    if (other->type_ == Type::COMM_WAIT) {
      const auto* wait = static_cast<const CommWaitTransition*>(other);

      if (timeout_ || wait->timeout_)
        return true; // Timeouts are not considered by the independence theorem, thus assumed dependent
    }

    return false; // Comm transitions are INDEP with non-comm transitions
  }

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
  bool depends(const Transition* other) const override
  {
    if (other->type_ == Type::COMM_TEST)
      return false; // Test & Test are independent

    if (other->type_ == Type::COMM_WAIT) {
      if (static_cast<const CommWaitTransition*>(other)->timeout_)
        return true; // Timeouts are not considered by the independence theorem, thus assumed dependent

      /* Wait & Test are independent */
      return false;
    }

    return false; // Comm transitions are INDEP with non-comm transitions
  }
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
  bool depends(const Transition* other) const override
  {
    if (other->type_ == Type::COMM_IPROBE)
      return mbox_ == static_cast<const CommIprobeTransition*>(other)->get_mailbox();

    // Iprobe can't enable a wait and is independent with every non Recv nor Send transition
    return false;
  }
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
  bool depends(const Transition* other) const override
  {
    switch (other->type_) {
      case Type::COMM_ASYNC_RECV:
        return mbox_ == static_cast<const CommRecvTransition*>(other)->mbox_;

      case Type::COMM_ASYNC_SEND:
        return false;

      case Type::COMM_IPROBE:
        return mbox_ == static_cast<const CommIprobeTransition*>(other)->get_mailbox();

      case Type::COMM_TEST: {
        const auto* test = static_cast<const CommTestTransition*>(other);

        if (mbox_ != test->mbox_)
          return false;

        if ((aid_ != test->sender_) && (aid_ != test->receiver_))
          return false;

        // If the test is checking a paired comm already, we're independent!
        // If we happen to make up that pair, then we're dependent...
        if (test->comm_ != comm_)
          return false;

        return true; // DEP with other send transitions
      }

      case Type::COMM_WAIT: {
        const auto* wait = static_cast<const CommWaitTransition*>(other);

        if (wait->timeout_)
          return true;

        if (mbox_ != wait->mbox_)
          return false;

        if ((aid_ != wait->sender_) && (aid_ != wait->receiver_))
          return false;

        // If the wait is waiting on a paired comm already, we're independent!
        // If we happen to make up that pair, then we're dependent...
        if ((aid_ != wait->aid_) && wait->comm_ != comm_)
          return false;

        return true; // DEP with other wait transitions
      }
      default:
        return false; // Comm transitions are INDEP with non-comm transitions
    }
  }

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
  bool depends(const Transition* other) const override
  {
    switch (other->type_) {
      case Type::COMM_ASYNC_SEND:
        return mbox_ == static_cast<const CommSendTransition*>(other)->mbox_;

      case Type::COMM_ASYNC_RECV:
        return false;

      case Type::COMM_IPROBE:
        return mbox_ == static_cast<const CommIprobeTransition*>(other)->get_mailbox();

      case Type::COMM_TEST: {
        const auto* test = static_cast<const CommTestTransition*>(other);

        if (mbox_ != test->mbox_)
          return false;
        if ((aid_ != test->sender_) && (aid_ != test->receiver_))
          return false;

        // If the test is checking a paired comm already, we're independent!
        // If we happen to make up that pair, then we're dependent...
        if (test->comm_ != comm_)
          return false;

        return true; // DEP with other test transitions
      }

      case Type::COMM_WAIT: {
        const auto* wait = static_cast<const CommWaitTransition*>(other);

        if (wait->timeout_)
          return true;
        if (mbox_ != wait->mbox_)
          return false;
        if ((aid_ != wait->sender_) && (aid_ != wait->receiver_))
          return false;
        // If the wait is waiting on a paired comm already, we're independent!
        // If we happen to make up that pair, then we're dependent...
        if ((aid_ != wait->aid_) && wait->comm_ != comm_)
          return false;

        return true; // DEP with other wait transitions
      }

      default:
        return false; // Comm transitions are INDEP with non-comm transitions
    }
  }
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
