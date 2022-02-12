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
  stream >> timeout_ >> comm_ >> sender_ >> receiver_ >> mbox_ >> src_buff_ >> dst_buff_ >> size_;
  XBT_DEBUG("CommWaitTransition %s comm:%p, sender:%ld receiver:%ld mbox:%u sbuff:%p rbuff:%p size:%zu",
            (timeout_ ? "timeout" : "no-timeout"), comm_, sender_, receiver_, mbox_, src_buff_, dst_buff_, size_);
}
std::string CommWaitTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: WaitComm(from %ld to %ld, mbox=%u, %s", aid_, sender_, receiver_, mbox_,
                                (timeout_ ? "timeout" : "no timeout"));
  if (verbose) {
    res += ", src_buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
    res += ", dst_buff=" + xbt::string_printf("%p", dst_buff_);
  }
  res += ")";
  return res;
}
bool CommWaitTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (dynamic_cast<const RandomTransition*>(other) != nullptr)
    return false; // Random is indep with any transition

  if (auto* recv = dynamic_cast<const CommRecvTransition*>(other))
    return recv->depends(this);

  if (auto* send = dynamic_cast<const CommSendTransition*>(other))
    return send->depends(this);

  if (auto* test = dynamic_cast<const CommTestTransition*>(other))
    return test->depends(this);

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (timeout_ || wait->timeout_)
      return true; // Timeouts are not considered by the independence theorem, thus assumed dependent

    if (src_buff_ == wait->src_buff_ && dst_buff_ == wait->dst_buff_)
      return false;
    if (src_buff_ != nullptr && dst_buff_ != nullptr && wait->src_buff_ != nullptr && wait->dst_buff_ != nullptr &&
        dst_buff_ != wait->src_buff_ && dst_buff_ != wait->dst_buff_ && dst_buff_ != src_buff_)
      return false;
  }

  return true;
}
CommTestTransition::CommTestTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_TEST, issuer, times_considered)
{
  stream >> comm_ >> sender_ >> receiver_ >> mbox_ >> src_buff_ >> dst_buff_ >> size_;
  XBT_DEBUG("CommTestTransition comm:%p, sender:%ld receiver:%ld mbox:%u sbuff:%p rbuff:%p size:%zu", comm_, sender_,
            receiver_, mbox_, src_buff_, dst_buff_, size_);
}
std::string CommTestTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: TestComm(from %ld to %ld, mbox=%u", aid_, sender_, receiver_, mbox_);
  if (verbose) {
    res += ", src_buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
    res += ", dst_buff=" + xbt::string_printf("%p", dst_buff_);
  }
  res += ")";
  return res;
}
bool CommTestTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (dynamic_cast<const RandomTransition*>(other) != nullptr)
    return false; // Test & Random are independent (Random is independent with anything)

  if (auto* recv = dynamic_cast<const CommRecvTransition*>(other))
    return recv->depends(this); // Recv < Test (alphabetical ordering)

  if (auto* send = dynamic_cast<const CommSendTransition*>(other))
    return send->depends(this); // Send < Test (alphabetical ordering)

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
  stream >> mbox_ >> dst_buff_;
}
std::string CommRecvTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: iRecv(mbox=%u", aid_, mbox_);
  if (verbose)
    res += ", buff=" + xbt::string_printf("%p", dst_buff_);
  res += ")";
  return res;
}
bool CommRecvTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (dynamic_cast<const RandomTransition*>(other) != nullptr)
    return false; // Random is indep with any transition

  if (const auto* recv = dynamic_cast<const CommRecvTransition*>(other))
    return mbox_ == recv->mbox_;

  if (dynamic_cast<const CommSendTransition*>(other) != nullptr)
    return false;

  if (const auto* test = dynamic_cast<const CommTestTransition*>(other)) {
    if (mbox_ != test->mbox_)
      return false;

    if ((aid_ != test->sender_) && (aid_ != test->receiver_) && (test->dst_buff_ != dst_buff_))
      return false;
  }

  if (auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true;

    if (mbox_ != wait->mbox_)
      return false;

    if ((aid_ != wait->sender_) && (aid_ != wait->receiver_) && (wait->dst_buff_ != dst_buff_))
      return false;
  }

  return true;
}

CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::COMM_SEND, issuer, times_considered)
{
  stream >> mbox_ >> src_buff_ >> size_;
  XBT_DEBUG("SendTransition mbox:%u buff:%p size:%zu", mbox_, src_buff_, size_);
}
std::string CommSendTransition::to_string(bool verbose = false) const
{
  auto res = xbt::string_printf("%ld: iSend(mbox=%u", aid_, mbox_);
  if (verbose)
    res += ", buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
  res += ")";
  return res;
}

bool CommSendTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (dynamic_cast<const RandomTransition*>(other) != nullptr)
    return false; // Random is indep with any transition

  if (const auto* other_isend = dynamic_cast<const CommSendTransition*>(other))
    return mbox_ == other_isend->mbox_;

  if (dynamic_cast<const CommRecvTransition*>(other) != nullptr)
    return false;

  if (const auto* test = dynamic_cast<const CommTestTransition*>(other)) {
    if (mbox_ != test->mbox_)
      return false;

    if ((aid_ != test->sender_) && (aid_ != test->receiver_) && (test->src_buff_ != src_buff_))
      return false;
  }

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true;

    if (mbox_ != wait->mbox_)
      return false;

    if ((aid_ != wait->sender_) && (aid_ != wait->receiver_) && (wait->src_buff_ != src_buff_))
      return false;
  }

  return true;
}

Transition* recv_transition(aid_t issuer, int times_considered, char* buffer)
{
  std::stringstream stream(buffer);
  short type;
  stream >> type;
  Transition::Type simcall = static_cast<Transition::Type>(type);

  switch (simcall) {
    case Transition::Type::COMM_RECV:
      return new CommRecvTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_SEND:
      return new CommSendTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_TEST:
      return new CommTestTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_WAIT:
      return new CommWaitTransition(issuer, times_considered, stream);

    case Transition::Type::RANDOM:
      return new RandomTransition(issuer, times_considered, stream);

    case Transition::Type::UNKNOWN:
      return new Transition(Transition::Type::UNKNOWN, issuer, times_considered);
    default:
      xbt_die("recv_transition of type %s unimplemented", Transition::to_c_str(simcall));
  }
}

} // namespace mc
} // namespace simgrid
