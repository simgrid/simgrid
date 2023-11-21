/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionComm.hpp"
#include "simgrid/config.h"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/State.hpp"
#include "xbt/asserts.h"
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
CommWaitTransition::CommWaitTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_WAIT, issuer, times_considered)
{
  xbt_assert(stream >> timeout_ >> comm_ >> sender_ >> receiver_ >> mbox_ >> call_location_);
  XBT_DEBUG("CommWaitTransition %s comm:%u, sender:%ld receiver:%ld mbox:%u call_loc:%s",
            (timeout_ ? "timeout" : "no-timeout"), comm_, sender_, receiver_, mbox_, call_location_.c_str());
}
std::string CommWaitTransition::to_string(bool verbose) const
{
  return xbt::string_printf("WaitComm(from %ld to %ld, mbox=%u, %s)", sender_, receiver_, mbox_,
                            (timeout_ ? "timeout" : "no timeout"));
}
bool CommWaitTransition::depends(const Transition* other) const
{
  if (other->type_ < type_)
    return other->depends(this);

  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (timeout_ || wait->timeout_)
      return true; // Timeouts are not considered by the independence theorem, thus assumed dependent
  }

  return false; // Comm transitions are INDEP with non-comm transitions
}

bool CommWaitTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::COMM_WAIT, "Unexpected transition type %s", to_c_str(type_));

  // If the other event is a communication event, then we are not reversible; otherwise we are reversible.
  return other->type_ != Transition::Type::COMM_ASYNC_SEND && other->type_ != Transition::Type::COMM_ASYNC_RECV;
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
CommTestTransition::CommTestTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_TEST, issuer, times_considered)
{
  xbt_assert(stream >> comm_ >> sender_ >> receiver_ >> mbox_ >> call_location_);
  XBT_DEBUG("CommTestTransition comm:%u, sender:%ld receiver:%ld mbox:%u call_loc:%s", comm_, sender_, receiver_, mbox_,
            call_location_.c_str());
}
std::string CommTestTransition::to_string(bool verbose) const
{
  return xbt::string_printf("TestComm(from %ld to %ld, mbox=%u)", sender_, receiver_, mbox_);
}

bool CommTestTransition::depends(const Transition* other) const
{
  if (other->type_ < type_)
    return other->depends(this);

  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  if (dynamic_cast<const CommTestTransition*>(other) != nullptr)
    return false; // Test & Test are independent

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true; // Timeouts are not considered by the independence theorem, thus assumed dependent

    /* Wait & Test are independent */
    return false;
  }

  return false; // Comm transitions are INDEP with non-comm transitions
}

bool CommTestTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::COMM_TEST, "Unexpected transition type %s", to_c_str(type_));
  return true; // CommTest is always enabled
}

CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_)
    : Transition(Type::COMM_ASYNC_RECV, issuer, times_considered), comm_(comm_), mbox_(mbox_), tag_(tag_)
{
}
CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_ASYNC_RECV, issuer, times_considered)
{
  xbt_assert(stream >> comm_ >> mbox_ >> tag_ >> call_location_);
  XBT_DEBUG("CommRecvTransition comm:%u, mbox:%u tag:%d call_loc:%s", comm_, mbox_, tag_, call_location_.c_str());
}
std::string CommRecvTransition::to_string(bool verbose) const
{
  return xbt::string_printf("iRecv(mbox=%u)", mbox_);
}
bool CommRecvTransition::depends(const Transition* other) const
{
  if (other->type_ < type_)
    return other->depends(this);

  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  if (const auto* recv = dynamic_cast<const CommRecvTransition*>(other))
    return mbox_ == recv->mbox_;

  if (dynamic_cast<const CommSendTransition*>(other) != nullptr)
    return false;

  if (const auto* test = dynamic_cast<const CommTestTransition*>(other)) {
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

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
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

  return false; // Comm transitions are INDEP with non-comm transitions
}

bool CommRecvTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::COMM_ASYNC_RECV, "Unexpected transition type %s", to_c_str(type_));

  return true; // CommRecv is always enabled
}

CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, unsigned comm_, unsigned mbox_, int tag_)
    : Transition(Type::COMM_ASYNC_SEND, issuer, times_considered), comm_(comm_), mbox_(mbox_), tag_(tag_)
{
}
CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_ASYNC_SEND, issuer, times_considered)
{
  xbt_assert(stream >> comm_ >> mbox_ >> tag_ >> call_location_);
  XBT_DEBUG("SendTransition comm:%u mbox:%u tag:%d call_loc:%s", comm_, mbox_, tag_, call_location_.c_str());
}
std::string CommSendTransition::to_string(bool verbose = false) const
{
  return xbt::string_printf("iSend(mbox=%u)", mbox_);
}

bool CommSendTransition::depends(const Transition* other) const
{
  if (other->type_ < type_)
    return other->depends(this);

  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  if (const auto* other_isend = dynamic_cast<const CommSendTransition*>(other))
    return mbox_ == other_isend->mbox_;

  if (dynamic_cast<const CommRecvTransition*>(other) != nullptr)
    return false;

  if (const auto* test = dynamic_cast<const CommTestTransition*>(other)) {
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

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
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

  return false; // Comm transitions are INDEP with non-comm transitions
}

bool CommSendTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::COMM_ASYNC_SEND, "Unexpected transition type %s", to_c_str(type_));

  return true; // CommSend is always enabled
}

} // namespace simgrid::mc
