/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionComm.hpp"
#include "xbt/asserts.h"
#include <simgrid/config.h>
#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/api/State.hpp"
#endif

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_comm, mc_transition,
                                "Logging specific to MC transitions about communications");

namespace simgrid {
namespace mc {

CommWaitTransition::CommWaitTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_WAIT, issuer, times_considered)
{
  xbt_assert(stream >> timeout_ >> comm_ >> sender_ >> receiver_ >> mbox_ >> sbuff_ >> rbuff_ >> size_);
  XBT_DEBUG("CommWaitTransition %s comm:%" PRIxPTR ", sender:%ld receiver:%ld mbox:%u sbuff:%" PRIxPTR
            " rbuff:%" PRIxPTR " size:%zu",
            (timeout_ ? "timeout" : "no-timeout"), comm_, sender_, receiver_, mbox_, sbuff_, rbuff_, size_);
}
std::string CommWaitTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("WaitComm(from %ld to %ld, mbox=%u, %s", sender_, receiver_, mbox_,
                                (timeout_ ? "timeout" : "no timeout"));
  if (verbose) {
    res += ", sbuff=" + xbt::string_printf("%" PRIxPTR, sbuff_) + ", size=" + std::to_string(size_);
    res += ", rbuff=" + xbt::string_printf("%" PRIxPTR, rbuff_);
  }
  res += ")";
  return res;
}
bool CommWaitTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (other->type_ < type_)
    return other->depends(this);

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (timeout_ || wait->timeout_)
      return true; // Timeouts are not considered by the independence theorem, thus assumed dependent

    if (sbuff_ == wait->sbuff_ && rbuff_ == wait->rbuff_)
      return false;
    if (sbuff_ != 0 && rbuff_ != 0 && wait->sbuff_ != 0 && wait->rbuff_ != 0 && rbuff_ != wait->sbuff_ &&
        rbuff_ != wait->rbuff_ && rbuff_ != sbuff_)
      return false;
  }

  return true;
}
CommTestTransition::CommTestTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_TEST, issuer, times_considered)
{
  xbt_assert(stream >> comm_ >> sender_ >> receiver_ >> mbox_ >> sbuff_ >> rbuff_ >> size_);
  XBT_DEBUG("CommTestTransition comm:%" PRIxPTR ", sender:%ld receiver:%ld mbox:%u sbuff:%" PRIxPTR " rbuff:%" PRIxPTR
            " size:%zu",
            comm_, sender_, receiver_, mbox_, sbuff_, rbuff_, size_);
}
std::string CommTestTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("TestComm(from %ld to %ld, mbox=%u", sender_, receiver_, mbox_);
  if (verbose) {
    res += ", sbuff=" + xbt::string_printf("%" PRIxPTR, sbuff_) + ", size=" + std::to_string(size_);
    res += ", rbuff=" + xbt::string_printf("%" PRIxPTR, rbuff_);
  }
  res += ")";
  return res;
}
bool CommTestTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (other->type_ < type_)
    return other->depends(this);

  if (dynamic_cast<const CommTestTransition*>(other) != nullptr)
    return false; // Test & Test are independent

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true; // Timeouts are not considered by the independence theorem, thus assumed dependent

    /* Wait & Test are independent */
    return false;
  }

  return true;
}

CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_RECV, issuer, times_considered)
{
  xbt_assert(stream >> comm_ >> mbox_ >> rbuff_ >> tag_);
}
std::string CommRecvTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("iRecv(mbox=%u", mbox_);
  if (verbose)
    res += ", rbuff=" + xbt::string_printf("%" PRIxPTR, rbuff_);
  res += ")";
  return res;
}
bool CommRecvTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (other->type_ < type_)
    return other->depends(this);

  if (const auto* recv = dynamic_cast<const CommRecvTransition*>(other))
    return mbox_ == recv->mbox_;

  if (dynamic_cast<const CommSendTransition*>(other) != nullptr)
    return false;

  if (const auto* test = dynamic_cast<const CommTestTransition*>(other)) {
    if (mbox_ != test->mbox_)
      return false;

    if ((aid_ != test->sender_) && (aid_ != test->receiver_) && (test->rbuff_ != rbuff_))
      return false;
  }

  if (auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true;

    if (mbox_ != wait->mbox_)
      return false;

    if ((aid_ != wait->sender_) && (aid_ != wait->receiver_) && (wait->rbuff_ != rbuff_))
      return false;
  }

  return true;
}

CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_SEND, issuer, times_considered)
{
  xbt_assert(stream >> comm_ >> mbox_ >> sbuff_ >> size_ >> tag_);
  XBT_DEBUG("SendTransition comm:%" PRIxPTR " mbox:%u sbuff:%" PRIxPTR " size:%zu", comm_, mbox_, sbuff_, size_);
}
std::string CommSendTransition::to_string(bool verbose = false) const
{
  auto res = xbt::string_printf("iSend(mbox=%u", mbox_);
  if (verbose)
    res += ", sbuff=" + xbt::string_printf("%" PRIxPTR, sbuff_) + ", size=" + std::to_string(size_);
  res += ")";
  return res;
}

bool CommSendTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (other->type_ < type_)
    return other->depends(this);

  if (const auto* other_isend = dynamic_cast<const CommSendTransition*>(other))
    return mbox_ == other_isend->mbox_;

  if (dynamic_cast<const CommRecvTransition*>(other) != nullptr)
    return false;

  if (const auto* test = dynamic_cast<const CommTestTransition*>(other)) {
    if (mbox_ != test->mbox_)
      return false;

    if ((aid_ != test->sender_) && (aid_ != test->receiver_) && (test->sbuff_ != sbuff_))
      return false;
  }

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true;

    if (mbox_ != wait->mbox_)
      return false;

    if ((aid_ != wait->sender_) && (aid_ != wait->receiver_) && (wait->sbuff_ != sbuff_))
      return false;
  }

  return true;
}

} // namespace mc
} // namespace simgrid
