/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Transition.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_state.hpp"
#include "src/mc/remote/RemoteProcess.hpp"
#include "xbt/asserts.h"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid {
namespace mc {
unsigned long Transition::executed_transitions_ = 0;
unsigned long Transition::replayed_transitions_ = 0;

std::string Transition::to_string(bool verbose)
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  return textual_;
}
const char* Transition::to_cstring(bool verbose)
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  to_string();
  return textual_.c_str();
}
void Transition::init(aid_t aid, int times_considered)
{
  aid_              = aid;
  times_considered_ = times_considered;
}
void Transition::replay() const
{
  replayed_transitions_++;

  mc_model_checker->handle_simcall(*this, false);
  mc_model_checker->wait_for_requests();
}

CommWaitTransition::CommWaitTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> timeout_ >> comm_ >> sender_ >> receiver_ >> mbox_ >> src_buff_ >> dst_buff_ >> size_;
}
std::string CommWaitTransition::to_string(bool verbose)
{
  textual_ = Transition::to_string(verbose);
  textual_ += xbt::string_printf("[src=%ld -> dst=%ld, mbox=%u, tout=%f", sender_, receiver_, mbox_, timeout_);
  if (verbose) {
    textual_ += ", src_buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
    textual_ += ", dst_buff=" + xbt::string_printf("%p", dst_buff_);
  }
  textual_ += "]";
  return textual_;
}

CommRecvTransition::CommRecvTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> mbox_ >> dst_buff_;
}
std::string CommRecvTransition::to_string(bool verbose)
{
  textual_ = xbt::string_printf("iRecv(recver=%ld mbox=%u", aid_, mbox_);
  if (verbose)
    textual_ += ", buff=" + xbt::string_printf("%p", dst_buff_);
  textual_ += ")";
  return textual_;
}
CommSendTransition::CommSendTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> mbox_ >> src_buff_ >> size_;
}
std::string CommSendTransition::to_string(bool verbose = false)
{
  textual_ = xbt::string_printf("iSend(sender=%ld mbox=%u", aid_, mbox_);
  if (verbose)
    textual_ += ", buff=" + xbt::string_printf("%p", src_buff_) + ", size=" + std::to_string(size_);
  textual_ += ")";
  return textual_;
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
