/* Copyright (c) 2015-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionComm.hpp"
#include "simgrid/config.h"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/string.hpp"

#include <inttypes.h>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_comm, mc_transition,
                                "Logging specific to MC transitions about communications");

namespace simgrid::mc {

CommWaitTransition::CommWaitTransition(Aid issuer, int times_considered, bool timeout_, unsigned comm_, Aid sender_,
                                       Aid receiver_, unsigned mbox_)
    : Transition(Type::COMM_WAIT, issuer, times_considered)
    , timeout_(timeout_)
    , comm_(comm_)
    , mbox_(mbox_)
    , sender_(sender_)
    , receiver_(receiver_)
{
}
CommWaitTransition::CommWaitTransition(Aid issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_WAIT, issuer, times_considered)
{
  timeout_ = channel.unpack<bool>();
  comm_    = channel.unpack<unsigned>();

  auto data = channel.unpack<aid_t>();
  sender_   = data == -1 ? Aid::INVALID : Aid{(int)data};
  data      = channel.unpack<aid_t>();
  receiver_ = data == -1 ? Aid::INVALID : Aid{(int)data};

  mbox_          = channel.unpack<unsigned>();
  call_location_ = std::make_unique<std::string>(channel.unpack<std::string>());

  XBT_DEBUG("CommWaitTransition %s comm:%u, sender:%d receiver:%d mbox:%u call_loc:%s",
            (timeout_ ? "timeout" : "no-timeout"), comm_, sender_.c_val(), receiver_.c_val(), mbox_,
            call_location_->c_str());
}
std::string CommWaitTransition::to_string(bool verbose) const
{
  if (not verbose)
    return xbt::string_printf("WaitComm(from %d to %d, mbox=%u, %s)", sender_.c_val(), receiver_.c_val(), mbox_,
                              (timeout_ ? "timeout" : "no timeout"));
  else
    return xbt::string_printf("WaitComm(from %d to %d, mbox=%u, %s, comm=%u)", sender_.c_val(), receiver_.c_val(),
                              mbox_, (timeout_ ? "timeout" : "no timeout"), comm_);
}

bool CommWaitTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_WAIT, "Unexpected transition type %s", to_c_str(type_));

  // The only case that could be blocking is an other wait transition
  return other->type_ != Type::COMM_WAIT;
}

CommTestTransition::CommTestTransition(Aid issuer, int times_considered, unsigned comm_, Aid sender_, Aid receiver_,
                                       unsigned mbox_)
    : Transition(Type::COMM_TEST, issuer, times_considered)
    , comm_(comm_)
    , mbox_(mbox_)
    , sender_(sender_)
    , receiver_(receiver_)
{
}
CommTestTransition::CommTestTransition(Aid issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::COMM_TEST, issuer, times_considered)
{
  comm_          = channel.unpack<unsigned>();

  auto data = channel.unpack<aid_t>();
  sender_   = data == -1 ? Aid::INVALID : Aid{(int)data};
  data      = channel.unpack<aid_t>();
  receiver_ = data == -1 ? Aid::INVALID : Aid{(int)data};

  mbox_          = channel.unpack<unsigned>();
  call_location_ = std::make_unique<std::string>(channel.unpack<std::string>());
  XBT_DEBUG("CommTestTransition comm:%u, sender:%d receiver:%d mbox:%u call_loc:%s", comm_, sender_.c_val(),
            receiver_.c_val(), mbox_, call_location_->c_str());
}
std::string CommTestTransition::to_string(bool verbose) const
{
  return xbt::string_printf("TestComm(from %d to %d, mbox=%u)", sender_.c_val(), receiver_.c_val(), mbox_);
}

bool CommTestTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_TEST, "Unexpected transition type %s", to_c_str(type_));
  return true; // CommTest is always enabled
}

/* Whether the probe could fire without the given send (or receive, for a sender-side probe) that precedes it: that is
 * the case if an earlier communication of the same kind on that mailbox could have enabled it instead.
 * Over-approximated, as allowed for reversible races: such earlier communications count even if they were already
 * matched, or if the application would refuse them for reasons that are not visible here (such as the communicator). */
static bool is_probe_fireable_without(const odpor::Execution* exec, EventHandle comm_handle, EventHandle probe_handle)
{
  const auto* probe = static_cast<const CommProbeTransition*>(exec->get_transition_for_handle(probe_handle));
  const auto enabler_type =
      probe->is_sender_side() ? Transition::Type::COMM_ASYNC_RECV : Transition::Type::COMM_ASYNC_SEND;
  // A communication of the other kind can only consume what the probe looks for, so it did not enable the probe
  if (exec->get_transition_for_handle(comm_handle)->type_ != enabler_type)
    return true;

  for (EventHandle e = 0; e < comm_handle; e++) {
    const Transition* t = exec->get_transition_for_handle(e);
    if (t->type_ != enabler_type)
      continue;
    unsigned mbox;
    int tag;
    if (enabler_type == Transition::Type::COMM_ASYNC_SEND) {
      mbox = static_cast<const CommSendTransition*>(t)->get_mailbox();
      tag  = static_cast<const CommSendTransition*>(t)->get_tag();
    } else {
      mbox = static_cast<const CommRecvTransition*>(t)->get_mailbox();
      tag  = static_cast<const CommRecvTransition*>(t)->get_tag();
    }
    if (mbox == probe->get_mailbox() && (not probe->get_src().has_value() || t->aid_ == probe->get_src()) &&
        (probe->is_any_tag() ? tag >= 0 : tag == probe->get_tag()))
      return true;
  }
  return false;
}

CommRecvTransition::CommRecvTransition(Aid issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_)
    : Transition(Type::COMM_ASYNC_RECV, issuer, times_considered), comm_(comm_), mbox_(mbox_), tag_(tag_)
{
}
CommRecvTransition::CommRecvTransition(Aid issuer, int times_considered, mc::Channel& channel)
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
    return xbt::string_printf("iRecv(mbox=%u, comm=%u, tag=%d)", mbox_, comm_, tag_);
}
bool CommRecvTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                         EventHandle other_handle) const
{
  xbt_assert(type_ == Type::COMM_ASYNC_RECV, "Unexpected transition type %s", to_c_str(type_));

  if (other->type_ == Type::COMM_PROBE)
    return is_probe_fireable_without(exec, this_handle, other_handle);

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

CommSendTransition::CommSendTransition(Aid issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_)
    : Transition(Type::COMM_ASYNC_SEND, issuer, times_considered), comm_(comm_), mbox_(mbox_), tag_(tag_)
{
}
CommSendTransition::CommSendTransition(Aid issuer, int times_considered, mc::Channel& channel)
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

  if (other->type_ == Type::COMM_PROBE)
    return is_probe_fireable_without(exec, this_handle, other_handle);

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

CommProbeTransition::CommProbeTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel)
    : Transition(type, issuer, times_considered)
{
  mbox_      = channel.unpack<unsigned>();
  is_sender_ = channel.unpack<bool>();
  tag_       = channel.unpack<int>();
  any_tag_   = channel.unpack<bool>();
  auto src   = channel.unpack<aid_t>();
  src_       = src == -1 ? Aid::INVALID : Aid{(int)src};

  XBT_DEBUG("%s mbox:%u %s tag:%d any_tag:%d src:%d", to_c_str(type_), mbox_,
            (is_sender_ ? "sender side" : "recv side"), tag_, any_tag_, src_.c_val());
}

std::string CommProbeTransition::to_string(bool verbose) const
{
  const char* name = type_ == Type::COMM_PROBE ? "Probe" : "iProbe";
  if (not verbose)
    return xbt::string_printf("%s(mbox=%u, %s)", name, mbox_, (is_sender_ ? "sender side" : "recv side"));
  else
    return xbt::string_printf("%s(mbox=%u, %s, tag=%d)", name, mbox_, (is_sender_ ? "sender side" : "recv side"), tag_);
}
bool CommProbeTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                          EventHandle this_handle, EventHandle other_handle) const
{
  // A probe enables nothing, so whatever comes after it could have been executed before it
  return true;
}

} // namespace simgrid::mc
