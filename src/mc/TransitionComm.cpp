/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/TransitionComm.hpp"
#include "src/mc/Transition.hpp"
#include "xbt/asserts.h"
#include <simgrid/config.h>
#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_state.hpp"
#endif

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_comm, mc_transition,
                                "Logging specific to MC transitions about communications");

namespace simgrid {
namespace mc {

CommWaitTransition::CommWaitTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> timeout_ >> comm_ >> sender_ >> receiver_ >> mbox_ >> src_buff_ >> dst_buff_ >> size_;
  XBT_DEBUG("CommWaitTransition %s comm:%p, sender:%ld receiver:%ld mbox:%u sbuff:%p rbuff:%p size:%zu",
            (timeout_ ? "timeout" : "no-timeout"), comm_, sender_, receiver_, mbox_, src_buff_, dst_buff_, size_);
}
std::string CommWaitTransition::to_string(bool verbose)
{
  textual_ = xbt::string_printf("%ld: WaitComm(from %ld to %ld, mbox=%u, %s", aid_, sender_, receiver_, mbox_,
                                (timeout_ ? "timeout" : "no timeout"));
  if (verbose) {
    textual_ += ", src_buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
    textual_ += ", dst_buff=" + xbt::string_printf("%p", dst_buff_);
  }
  textual_ += ")";
  return textual_;
}
bool CommWaitTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (auto* send = dynamic_cast<const CommSendTransition*>(other))
    return send->depends(this);

  if (auto* recv = dynamic_cast<const CommRecvTransition*>(other))
    return recv->depends(this);

  /* Timeouts in wait transitions are not considered by the independence theorem, thus assumed dependent */
  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (timeout_ || wait->timeout_)
      return true;

    if (src_buff_ == wait->src_buff_ && dst_buff_ == wait->dst_buff_)
      return false;
    if (src_buff_ != nullptr && dst_buff_ != nullptr && wait->src_buff_ != nullptr && wait->dst_buff_ != nullptr &&
        dst_buff_ != wait->src_buff_ && dst_buff_ != wait->dst_buff_ && dst_buff_ != src_buff_)
      return false;
  }

  return true;
}

CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> mbox_ >> dst_buff_;
}
std::string CommRecvTransition::to_string(bool verbose)
{
  textual_ = xbt::string_printf("%ld: iRecv(mbox=%u", aid_, mbox_);
  if (verbose)
    textual_ += ", buff=" + xbt::string_printf("%p", dst_buff_);
  textual_ += ")";
  return textual_;
}
bool CommRecvTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (const auto* other_irecv = dynamic_cast<const CommRecvTransition*>(other))
    return mbox_ == other_irecv->mbox_;

  if (auto* isend = dynamic_cast<const CommSendTransition*>(other))
    return isend->depends(this);

  if (auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true;

    if (mbox_ != wait->mbox_)
      return false;

    if ((aid_ != wait->sender_) && (aid_ != wait->receiver_))
      return false;

    if (wait->dst_buff_ != dst_buff_)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the isend/irecv call is not stored in a buffer used in the
   * test call. */
#if 0
  if (dynamic_cast<ActivityTestSimcall*>(other))
    return false;
#endif

  return true;
}

CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> mbox_ >> src_buff_ >> size_;
  XBT_DEBUG("SendTransition mbox:%u buff:%p size:%zu", mbox_, src_buff_, size_);
}
std::string CommSendTransition::to_string(bool verbose = false)
{
  textual_ = xbt::string_printf("%ld: iSend(mbox=%u", aid_, mbox_);
  if (verbose)
    textual_ += ", buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
  textual_ += ")";
  return textual_;
}
bool CommSendTransition::depends(const Transition* other) const
{
  if (aid_ == other->aid_)
    return false;

  if (const auto* other_isend = dynamic_cast<const CommSendTransition*>(other))
    return mbox_ == other_isend->mbox_;

  // FIXME: Not in the former dependency check because of the ordering but seems logical to add it
  if (dynamic_cast<const CommRecvTransition*>(other) != nullptr)
    return false;

  if (const auto* wait = dynamic_cast<const CommWaitTransition*>(other)) {
    if (wait->timeout_)
      return true;

    if (mbox_ != wait->mbox_)
      return false;

    if ((aid_ != wait->sender_) && (aid_ != wait->receiver_))
      return false;

    if (wait->src_buff_ != src_buff_)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the isend/irecv call is not stored in a buffer used in the
   * test call. */
#if 0
  if (dynamic_cast<ActivityTestSimcall*>(other))
    return false;
#endif

  return true;
}

Transition* recv_transition(aid_t issuer, int times_considered, kernel::actor::SimcallObserver::Simcall simcall,
                            char* buffer)
{
  switch (simcall) {
    case kernel::actor::SimcallObserver::Simcall::COMM_WAIT:
      return new CommWaitTransition(issuer, times_considered, buffer);
    case kernel::actor::SimcallObserver::Simcall::IRECV:
      return new CommRecvTransition(issuer, times_considered, buffer);
    case kernel::actor::SimcallObserver::Simcall::ISEND:
      return new CommSendTransition(issuer, times_considered, buffer);
    case kernel::actor::SimcallObserver::Simcall::UNKNOWN:
      return new Transition(issuer, times_considered);
    default:
      xbt_die("recv_transition of type %s unimplemented", kernel::actor::SimcallObserver::to_c_str(simcall));
  }
}

} // namespace mc
} // namespace simgrid
