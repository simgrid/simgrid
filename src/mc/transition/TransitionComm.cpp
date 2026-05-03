/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionComm.hpp"
#include "simgrid/config.h"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/string.hpp"

#include <inttypes.h>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_comm, mc_transition,
                                "Logging specific to MC transitions about communications");

namespace simgrid::mc {

CommWaitTransition::CommWaitTransition(aid_t issuer, int times_considered, bool timeout_, unsigned comm_, aid_t sender_,
                                       aid_t receiver_, unsigned mbox_)
    : Transition(Type::COMM_WAIT, issuer, times_considered)
    , timeout_(timeout_)
    , comm_(comm_)
    , mbox_(mbox_)
    , sender_(sender_)
    , receiver_(receiver_)
{
}
CommWaitTransition::CommWaitTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_WAIT, issuer, times_considered)
{
  timeout_ = channel.unpack<bool>();
  comm_    = channel.unpack<unsigned>();

  sender_        = channel.unpack<aid_t>();
  receiver_      = channel.unpack<aid_t>();
  mbox_          = channel.unpack<unsigned>();
  call_location_ = std::make_unique<std::string>(channel.unpack<std::string>());

  XBT_DEBUG("CommWaitTransition %s comm:%u, sender:%ld receiver:%ld mbox:%u call_loc:%s",
            (timeout_ ? "timeout" : "no-timeout"), comm_, sender_, receiver_, mbox_, call_location_->c_str());
}
std::string CommWaitTransition::to_string(bool verbose) const
{
  if (not verbose)
    return xbt::string_printf("WaitComm(from %ld to %ld, mbox=%u, %s)", sender_, receiver_, mbox_,
                              (timeout_ ? "timeout" : "no timeout"));
  else
    return xbt::string_printf("WaitComm(from %ld to %ld, mbox=%u, %s, comm=%u)", sender_, receiver_, mbox_,
                              (timeout_ ? "timeout" : "no timeout"), comm_);
}

bool CommWaitTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_WAIT, "Unexpected transition type %s", to_c_str(type_));

  // The only case that could be blocking is an other wait transition
  return other->type_ != Type::COMM_WAIT;
}

CommTestTransition::CommTestTransition(aid_t issuer, int times_considered, unsigned comm_, aid_t sender_,
                                       aid_t receiver_, unsigned mbox_)
    : Transition(Type::COMM_TEST, issuer, times_considered)
    , comm_(comm_)
    , mbox_(mbox_)
    , sender_(sender_)
    , receiver_(receiver_)
{
}
CommTestTransition::CommTestTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_TEST, issuer, times_considered)
{
  comm_          = channel.unpack<unsigned>();
  sender_        = channel.unpack<aid_t>();
  receiver_      = channel.unpack<aid_t>();
  mbox_          = channel.unpack<unsigned>();
  call_location_ = std::make_unique<std::string>(channel.unpack<std::string>());
  XBT_DEBUG("CommTestTransition comm:%u, sender:%ld receiver:%ld mbox:%u call_loc:%s", comm_, sender_, receiver_, mbox_,
            call_location_->c_str());
}
std::string CommTestTransition::to_string(bool verbose) const
{
  return xbt::string_printf("TestComm(from %ld to %ld, mbox=%u)", sender_, receiver_, mbox_);
}

bool CommTestTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_TEST, "Unexpected transition type %s", to_c_str(type_));
  return true; // CommTest is always enabled
}

CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_)
    : Transition(Type::COMM_ASYNC_RECV, issuer, times_considered), comm_(comm_), mbox_(mbox_), tag_(tag_)
{
}
CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_ASYNC_RECV, issuer, times_considered)
{
  comm_          = channel.unpack<unsigned>();
  mbox_          = channel.unpack<unsigned>();
  tag_           = channel.unpack<int>();
  call_location_ = std::make_unique<std::string>(channel.unpack<std::string>());

  XBT_DEBUG("CommRecvTransition comm:%u, mbox:%u tag:%d call_loc:%s", comm_, mbox_, tag_, call_location_->c_str());
}
std::string CommRecvTransition::to_string(bool verbose) const
{
  if (not verbose)
    return xbt::string_printf("iRecv(mbox=%u)", mbox_);
  else
    return xbt::string_printf("iRecv(mbox=%u, comm=%u, tag=%d))", mbox_, comm_, tag_);
}
bool CommRecvTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_ASYNC_RECV, "Unexpected transition type %s", to_c_str(type_));

  // If this recv enabled the comm_wait, then it's not reversible
  if (other->type_ == Type::COMM_WAIT) {
    const auto* wait = static_cast<const CommWaitTransition*>(other);
    if (wait->comm_ == this->comm_)
      return false;
  }

  if (other->type_ == Type::WAITANY) {
    const auto* wait =
        static_cast<const CommWaitTransition*>(static_cast<const WaitAnyTransition*>(other)->get_current_transition());
    if (wait->comm_ == this->comm_)
      return false;
  }

  return true;
}

CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_)
    : Transition(Type::COMM_ASYNC_SEND, issuer, times_considered), comm_(comm_), mbox_(mbox_), tag_(tag_)
{
}
CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_ASYNC_SEND, issuer, times_considered)
{
  comm_          = channel.unpack<unsigned>();
  mbox_          = channel.unpack<unsigned>();
  tag_           = channel.unpack<int>();
  call_location_ = std::make_unique<std::string>(channel.unpack<std::string>());

  XBT_DEBUG("SendTransition comm:%u mbox:%u tag:%d call_loc:%s", comm_, mbox_, tag_, call_location_->c_str());
}
std::string CommSendTransition::to_string(bool verbose) const
{
  if (not verbose)
    return xbt::string_printf("iSend(mbox=%u)", mbox_);
  else
    return xbt::string_printf("iSend(mbox=%u, comm=%u, tag=%d)", mbox_, comm_, tag_);
}

bool CommSendTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_ASYNC_SEND, "Unexpected transition type %s", to_c_str(type_));

  // If this send enabled the comm_wait, then it's not reversible
  if (other->type_ == Type::COMM_WAIT) {
    const auto* wait = static_cast<const CommWaitTransition*>(other);
    if (wait->comm_ == this->comm_)
      return false;
  }

  if (other->type_ == Type::WAITANY) {
    const auto* wait =
        static_cast<const CommWaitTransition*>(static_cast<const WaitAnyTransition*>(other)->get_current_transition());
    if (wait->comm_ == this->comm_)
      return false;
  }

  return true;
}

CommIprobeTransition::CommIprobeTransition(aid_t issuer, int times_considered, bool is_sender, unsigned mbox, int tag)
    : Transition(Type::COMM_IPROBE, issuer, times_considered), is_sender_(is_sender), mbox_(mbox), tag_(tag)
{
}
CommIprobeTransition::CommIprobeTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_IPROBE, issuer, times_considered)
{
  mbox_      = channel.unpack<unsigned>();
  is_sender_ = channel.unpack<bool>();
  tag_       = channel.unpack<int>();

  XBT_DEBUG("SendTransition mbox:%u %s tag:%d", mbox_, (is_sender_ ? "sender side" : "recv side"), tag_);
}

std::string CommIprobeTransition::to_string(bool verbose) const
{
  if (not verbose)
    return xbt::string_printf("iProbe(mbox=%u, %s)", mbox_, (is_sender_ ? "sender side" : "recv side"));
  else
    return xbt::string_printf("iProbe(mbox=%u, %s, tag=%d)", mbox_, (is_sender_ ? "sender side" : "recv side"), tag_);
}
bool CommIprobeTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                           EventHandle this_handle, EventHandle other_handle) const
{
  // In every cases, we can execute Iprobe before someone else
  return true;
}
bool CommIprobeTransition::can_be_co_enabled(const Transition* o) const
{
  // Iprobe can be executed at any time
  return true;
}

} // namespace simgrid::mc
