/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_request.hpp"
#include "src/include/mc/mc.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/checker/SimcallInspector.hpp"
#include "src/mc/mc_smx.hpp"
#include <array>

using simgrid::mc::remote;
using simgrid::simix::Simcall;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_request, mc, "Logging specific to MC (request)");

static inline simgrid::kernel::activity::CommImpl* MC_get_comm(smx_simcall_t r)
{
  switch (r->call_) {
    case Simcall::COMM_WAIT:
      return simcall_comm_wait__getraw__comm(r);
    case Simcall::COMM_TEST:
      return simcall_comm_test__getraw__comm(r);
    default:
      return nullptr;
  }
}

static inline
smx_mailbox_t MC_get_mbox(smx_simcall_t r)
{
  switch (r->call_) {
    case Simcall::COMM_ISEND:
      return simcall_comm_isend__get__mbox(r);
    case Simcall::COMM_IRECV:
      return simcall_comm_irecv__get__mbox(r);
    default:
      return nullptr;
  }
}

namespace simgrid {
namespace mc {

// Does half the job
static inline
bool request_depend_asymmetric(smx_simcall_t r1, smx_simcall_t r2)
{
  if (r1->call_ == Simcall::COMM_ISEND && r2->call_ == Simcall::COMM_IRECV)
    return false;

  if (r1->call_ == Simcall::COMM_IRECV && r2->call_ == Simcall::COMM_ISEND)
    return false;

  // Those are internal requests, we do not need indirection because those objects are copies:
  const kernel::activity::CommImpl* synchro1 = MC_get_comm(r1);
  const kernel::activity::CommImpl* synchro2 = MC_get_comm(r2);

  if ((r1->call_ == Simcall::COMM_ISEND || r1->call_ == Simcall::COMM_IRECV) && r2->call_ == Simcall::COMM_WAIT) {
    const kernel::activity::MailboxImpl* mbox = MC_get_mbox(r1);

    if (mbox != synchro2->mbox_cpy
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->issuer_ != synchro2->src_actor_.get()) && (r1->issuer_ != synchro2->dst_actor_.get()) &&
        simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == Simcall::COMM_ISEND) && (synchro2->type_ == kernel::activity::CommImpl::Type::SEND) &&
        (synchro2->src_buff_ != simcall_comm_isend__get__src_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == Simcall::COMM_IRECV) && (synchro2->type_ == kernel::activity::CommImpl::Type::RECEIVE) &&
        (synchro2->dst_buff_ != simcall_comm_irecv__get__dst_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the isend/irecv call is not stored in a buffer used in the
   * test call. */
#if 0
  if((r1->call == Simcall::COMM_ISEND || r1->call == Simcall::COMM_IRECV)
      &&  r2->call == Simcall::COMM_TEST)
    return false;
#endif

  if (r1->call_ == Simcall::COMM_WAIT && (r2->call_ == Simcall::COMM_WAIT || r2->call_ == Simcall::COMM_TEST) &&
      (synchro1->src_actor_.get() == nullptr || synchro1->dst_actor_.get() == nullptr))
    return false;

  if (r1->call_ == Simcall::COMM_TEST &&
      (simcall_comm_test__get__comm(r1) == nullptr || synchro1->src_buff_ == nullptr || synchro1->dst_buff_ == nullptr))
    return false;

  if (r1->call_ == Simcall::COMM_TEST && r2->call_ == Simcall::COMM_WAIT &&
      synchro1->src_buff_ == synchro2->src_buff_ && synchro1->dst_buff_ == synchro2->dst_buff_)
    return false;

  if (r1->call_ == Simcall::COMM_WAIT && r2->call_ == Simcall::COMM_TEST && synchro1->src_buff_ != nullptr &&
      synchro1->dst_buff_ != nullptr && synchro2->src_buff_ != nullptr && synchro2->dst_buff_ != nullptr &&
      synchro1->dst_buff_ != synchro2->src_buff_ && synchro1->dst_buff_ != synchro2->dst_buff_ &&
      synchro2->dst_buff_ != synchro1->src_buff_)
    return false;

  return true;
}

// Those are internal_req
bool request_depend(smx_simcall_t req1, smx_simcall_t req2)
{
  if (req1->issuer_ == req2->issuer_)
    return false;

  /* Wait with timeout transitions are not considered by the independence theorem, thus we consider them as dependent with all other transitions */
  if ((req1->call_ == Simcall::COMM_WAIT && simcall_comm_wait__get__timeout(req1) > 0) ||
      (req2->call_ == Simcall::COMM_WAIT && simcall_comm_wait__get__timeout(req2) > 0))
    return true;

  if (req1->call_ != req2->call_)
    return request_depend_asymmetric(req1, req2) && request_depend_asymmetric(req2, req1);

  // Those are internal requests, we do not need indirection because those objects are copies:
  const kernel::activity::CommImpl* synchro1 = MC_get_comm(req1);
  const kernel::activity::CommImpl* synchro2 = MC_get_comm(req2);

  switch (req1->call_) {
    case Simcall::COMM_ISEND:
      return simcall_comm_isend__get__mbox(req1) == simcall_comm_isend__get__mbox(req2);
    case Simcall::COMM_IRECV:
      return simcall_comm_irecv__get__mbox(req1) == simcall_comm_irecv__get__mbox(req2);
    case Simcall::COMM_WAIT:
      if (synchro1->src_buff_ == synchro2->src_buff_ && synchro1->dst_buff_ == synchro2->dst_buff_)
        return false;
      if (synchro1->src_buff_ != nullptr && synchro1->dst_buff_ != nullptr && synchro2->src_buff_ != nullptr &&
          synchro2->dst_buff_ != nullptr && synchro1->dst_buff_ != synchro2->src_buff_ &&
          synchro1->dst_buff_ != synchro2->dst_buff_ && synchro2->dst_buff_ != synchro1->src_buff_)
        return false;
      return true;
    default:
      return true;
  }
}

} // namespace mc
} // namespace simgrid

namespace simgrid {
namespace mc {

bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx)
{
  kernel::activity::CommImpl* remote_act = nullptr;
  switch (req->call_) {
    case Simcall::COMM_WAIT:
      /* FIXME: check also that src and dst processes are not suspended */
      remote_act = simcall_comm_wait__getraw__comm(req);
      break;

    case Simcall::COMM_WAITANY:
      remote_act = mc_model_checker->get_remote_simulation().read(remote(simcall_comm_waitany__get__comms(req) + idx));
      break;

    case Simcall::COMM_TESTANY:
      remote_act = mc_model_checker->get_remote_simulation().read(remote(simcall_comm_testany__get__comms(req) + idx));
      break;

    default:
      return true;
  }

  Remote<kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, remote(remote_act));
  const kernel::activity::CommImpl* comm = temp_comm.get_buffer();
  return comm->src_actor_.get() && comm->dst_actor_.get();
}

} // namespace mc
} // namespace simgrid
