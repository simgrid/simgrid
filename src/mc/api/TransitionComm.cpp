/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/TransitionComm.hpp"
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
  auto res = xbt::string_printf("%ld: WaitComm(from %ld to %ld, mbox=%u, %s", aid_, sender_, receiver_, mbox_,
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
  auto res = xbt::string_printf("%ld: TestComm(from %ld to %ld, mbox=%u", aid_, sender_, receiver_, mbox_);
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
  auto res = xbt::string_printf("%ld: iRecv(mbox=%u", aid_, mbox_);
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
  auto res = xbt::string_printf("%ld: iSend(mbox=%u", aid_, mbox_);
  if (verbose)
    res += ", sbuff=" + xbt::string_printf("%" PRIxPTR, sbuff_) + ", size=" + std::to_string(size_);
  res += ")";
  return res;
}
TestAnyTransition::TestAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::TESTANY, issuer, times_considered)
{
  int size;
  xbt_assert(stream >> size);
  for (int i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, stream);
    XBT_DEBUG("TestAny received a transition %s", t->to_string(true).c_str());
    transitions_.push_back(t);
  }
}
std::string TestAnyTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: TestAny{ ", aid_);
  for (auto const* t : transitions_)
    res += t->to_string(verbose);
  res += "}";
  return res;
}
bool TestAnyTransition::depends(const Transition* other) const
{
  return transitions_[times_considered_]->depends(other);
}
WaitAnyTransition::WaitAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::WAITANY, issuer, times_considered)
{
  int size;
  xbt_assert(stream >> size);
  for (int i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, stream);
    transitions_.push_back(t);
  }
}
std::string WaitAnyTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: WaitAny{ ", aid_);
  for (auto const* t : transitions_)
    res += t->to_string(verbose);
  res += "}";
  return res;
}
bool WaitAnyTransition::depends(const Transition* other) const
{
  return transitions_[times_considered_]->depends(other);
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

Transition* deserialize_transition(aid_t issuer, int times_considered, std::stringstream& stream)
{
  short type;
  xbt_assert(stream >> type);
  xbt_assert(type >= 0 && type <= static_cast<short>(Transition::Type::UNKNOWN), "Invalid transition type %d received",
             type);

  auto simcall = static_cast<Transition::Type>(type);

  switch (simcall) {
    case Transition::Type::COMM_RECV:
      return new CommRecvTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_SEND:
      return new CommSendTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_TEST:
      return new CommTestTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_WAIT:
      return new CommWaitTransition(issuer, times_considered, stream);

    case Transition::Type::TESTANY:
      return new TestAnyTransition(issuer, times_considered, stream);
    case Transition::Type::WAITANY:
      return new WaitAnyTransition(issuer, times_considered, stream);

    case Transition::Type::RANDOM:
      return new RandomTransition(issuer, times_considered, stream);

    case Transition::Type::UNKNOWN:
      return new Transition(Transition::Type::UNKNOWN, issuer, times_considered);
  }
  THROW_IMPOSSIBLE; // Some compilers don't detect that each branch of the above switch has a return
}

} // namespace mc
} // namespace simgrid
