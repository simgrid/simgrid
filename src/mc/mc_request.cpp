/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_request.hpp"
#include "src/include/mc/mc.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/checker/SimcallInspector.hpp"
#include "src/mc/mc_smx.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_request, mc, "Logging specific to MC (request)");

static char *pointer_to_string(void *pointer);
static char *buff_size_to_string(size_t size);

static inline simgrid::kernel::activity::CommImpl* MC_get_comm(smx_simcall_t r)
{
  switch (r->call_) {
    case SIMCALL_COMM_WAIT:
      return static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_wait__getraw__comm(r));
    case SIMCALL_COMM_TEST:
      return static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_test__getraw__comm(r));
    default:
      return nullptr;
  }
}

static inline
smx_mailbox_t MC_get_mbox(smx_simcall_t r)
{
  switch (r->call_) {
    case SIMCALL_COMM_ISEND:
      return simcall_comm_isend__get__mbox(r);
    case SIMCALL_COMM_IRECV:
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
  if (r1->call_ == SIMCALL_COMM_ISEND && r2->call_ == SIMCALL_COMM_IRECV)
    return false;

  if (r1->call_ == SIMCALL_COMM_IRECV && r2->call_ == SIMCALL_COMM_ISEND)
    return false;

  // Those are internal requests, we do not need indirection because those objects are copies:
  const kernel::activity::CommImpl* synchro1 = MC_get_comm(r1);
  const kernel::activity::CommImpl* synchro2 = MC_get_comm(r2);

  if ((r1->call_ == SIMCALL_COMM_ISEND || r1->call_ == SIMCALL_COMM_IRECV) && r2->call_ == SIMCALL_COMM_WAIT) {
    const kernel::activity::MailboxImpl* mbox = MC_get_mbox(r1);

    if (mbox != synchro2->mbox_cpy
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->issuer_ != synchro2->src_actor_.get()) && (r1->issuer_ != synchro2->dst_actor_.get()) &&
        simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == SIMCALL_COMM_ISEND) && (synchro2->type_ == kernel::activity::CommImpl::Type::SEND) &&
        (synchro2->src_buff_ != simcall_comm_isend__get__src_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == SIMCALL_COMM_IRECV) && (synchro2->type_ == kernel::activity::CommImpl::Type::RECEIVE) &&
        (synchro2->dst_buff_ != simcall_comm_irecv__get__dst_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the isend/irecv call is not stored in a buffer used in the
   * test call. */
#if 0
  if((r1->call == SIMCALL_COMM_ISEND || r1->call == SIMCALL_COMM_IRECV)
      &&  r2->call == SIMCALL_COMM_TEST)
    return false;
#endif

  if (r1->call_ == SIMCALL_COMM_WAIT && (r2->call_ == SIMCALL_COMM_WAIT || r2->call_ == SIMCALL_COMM_TEST) &&
      (synchro1->src_actor_.get() == nullptr || synchro1->dst_actor_.get() == nullptr))
    return false;

  if (r1->call_ == SIMCALL_COMM_TEST &&
      (simcall_comm_test__get__comm(r1) == nullptr || synchro1->src_buff_ == nullptr || synchro1->dst_buff_ == nullptr))
    return false;

  if (r1->call_ == SIMCALL_COMM_TEST && r2->call_ == SIMCALL_COMM_WAIT && synchro1->src_buff_ == synchro2->src_buff_ &&
      synchro1->dst_buff_ == synchro2->dst_buff_)
    return false;

  if (r1->call_ == SIMCALL_COMM_WAIT && r2->call_ == SIMCALL_COMM_TEST && synchro1->src_buff_ != nullptr &&
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
  if ((req1->call_ == SIMCALL_COMM_WAIT && simcall_comm_wait__get__timeout(req1) > 0) ||
      (req2->call_ == SIMCALL_COMM_WAIT && simcall_comm_wait__get__timeout(req2) > 0))
    return true;

  if (req1->call_ != req2->call_)
    return request_depend_asymmetric(req1, req2) && request_depend_asymmetric(req2, req1);

  // Those are internal requests, we do not need indirection because those objects are copies:
  const kernel::activity::CommImpl* synchro1 = MC_get_comm(req1);
  const kernel::activity::CommImpl* synchro2 = MC_get_comm(req2);

  switch (req1->call_) {
    case SIMCALL_COMM_ISEND:
      return simcall_comm_isend__get__mbox(req1) == simcall_comm_isend__get__mbox(req2);
    case SIMCALL_COMM_IRECV:
      return simcall_comm_irecv__get__mbox(req1) == simcall_comm_irecv__get__mbox(req2);
    case SIMCALL_COMM_WAIT:
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

static char *pointer_to_string(void *pointer)
{
  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%p", pointer);

  return xbt_strdup("(verbose only)");
}

static char *buff_size_to_string(size_t buff_size)
{
  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%zu", buff_size);

  return xbt_strdup("(verbose only)");
}


std::string simgrid::mc::request_to_string(smx_simcall_t req, int value, simgrid::mc::RequestType request_type)
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  if (req->inspector_ != nullptr)
    return req->inspector_->to_string();

  bool use_remote_comm = true;
  switch(request_type) {
  case simgrid::mc::RequestType::simix:
    use_remote_comm = true;
    break;
  case simgrid::mc::RequestType::executed:
  case simgrid::mc::RequestType::internal:
    use_remote_comm = false;
    break;
  default:
    THROW_IMPOSSIBLE;
  }

  const char* type = nullptr;
  char *args = nullptr;

  smx_actor_t issuer = MC_smx_simcall_get_issuer(req);

  switch (req->call_) {
    case SIMCALL_COMM_ISEND: {
      type     = "iSend";
      char* p  = pointer_to_string(simcall_comm_isend__get__src_buff(req));
      char* bs = buff_size_to_string(simcall_comm_isend__get__src_buff_size(req));
      if (issuer->get_host())
        args = bprintf("src=(%ld)%s (%s), buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                       MC_smx_actor_get_name(issuer), p, bs);
      else
        args = bprintf("src=(%ld)%s, buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_name(issuer), p, bs);
      xbt_free(bs);
      xbt_free(p);
      break;
    }

    case SIMCALL_COMM_IRECV: {
      size_t* remote_size = simcall_comm_irecv__get__dst_buff_size(req);
      size_t size         = 0;
      if (remote_size)
        mc_model_checker->process().read_bytes(&size, sizeof(size), remote(remote_size));

      type     = "iRecv";
      char* p  = pointer_to_string(simcall_comm_irecv__get__dst_buff(req));
      char* bs = buff_size_to_string(size);
      if (issuer->get_host())
        args = bprintf("dst=(%ld)%s (%s), buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                       MC_smx_actor_get_name(issuer), p, bs);
      else
        args = bprintf("dst=(%ld)%s, buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_name(issuer), p, bs);
      xbt_free(bs);
      xbt_free(p);
      break;
    }

    case SIMCALL_COMM_WAIT: {
      simgrid::kernel::activity::CommImpl* remote_act =
          static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_wait__getraw__comm(req));
      char* p;
      if (value == -1) {
        type = "WaitTimeout";
        p    = pointer_to_string(remote_act);
        args = bprintf("comm=%s", p);
      } else {
        type = "Wait";
        p    = pointer_to_string(remote_act);

        simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_synchro;
        const simgrid::kernel::activity::CommImpl* act;
        if (use_remote_comm) {
          mc_model_checker->process().read(temp_synchro,
                                           remote(static_cast<simgrid::kernel::activity::CommImpl*>(remote_act)));
          act = temp_synchro.get_buffer();
        } else
          act = remote_act;

        smx_actor_t src_proc = mc_model_checker->process().resolve_actor(simgrid::mc::remote(act->src_actor_.get()));
        smx_actor_t dst_proc = mc_model_checker->process().resolve_actor(simgrid::mc::remote(act->dst_actor_.get()));
        args                 = bprintf("comm=%s [(%ld)%s (%s)-> (%ld)%s (%s)]", p, src_proc ? src_proc->get_pid() : 0,
                       src_proc ? MC_smx_actor_get_host_name(src_proc) : "",
                       src_proc ? MC_smx_actor_get_name(src_proc) : "", dst_proc ? dst_proc->get_pid() : 0,
                       dst_proc ? MC_smx_actor_get_host_name(dst_proc) : "",
                       dst_proc ? MC_smx_actor_get_name(dst_proc) : "");
      }
      xbt_free(p);
      break;
    }

    case SIMCALL_COMM_TEST: {
      simgrid::kernel::activity::CommImpl* remote_act =
          static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_test__getraw__comm(req));
      simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_synchro;
      const simgrid::kernel::activity::CommImpl* act;
      if (use_remote_comm) {
        mc_model_checker->process().read(temp_synchro,
                                         remote(static_cast<simgrid::kernel::activity::CommImpl*>(remote_act)));
        act = temp_synchro.get_buffer();
      } else
        act = remote_act;

      char* p;
      if (act->src_actor_.get() == nullptr || act->dst_actor_.get() == nullptr) {
        type = "Test FALSE";
        p    = pointer_to_string(remote_act);
        args = bprintf("comm=%s", p);
      } else {
        type = "Test TRUE";
        p    = pointer_to_string(remote_act);

        smx_actor_t src_proc = mc_model_checker->process().resolve_actor(simgrid::mc::remote(act->src_actor_.get()));
        smx_actor_t dst_proc = mc_model_checker->process().resolve_actor(simgrid::mc::remote(act->dst_actor_.get()));
        args                 = bprintf("comm=%s [(%ld)%s (%s) -> (%ld)%s (%s)]", p, src_proc->get_pid(),
                       MC_smx_actor_get_name(src_proc), MC_smx_actor_get_host_name(src_proc), dst_proc->get_pid(),
                       MC_smx_actor_get_name(dst_proc), MC_smx_actor_get_host_name(dst_proc));
      }
      xbt_free(p);
      break;
    }

    case SIMCALL_COMM_WAITANY: {
      type         = "WaitAny";
      size_t count = simcall_comm_waitany__get__count(req);
      if (count > 0) {
        simgrid::kernel::activity::CommImpl* remote_sync;
        remote_sync = mc_model_checker->process().read(remote(simcall_comm_waitany__get__comms(req) + value));
        char* p     = pointer_to_string(remote_sync);
        args        = bprintf("comm=%s (%d of %zu)", p, value + 1, count);
        xbt_free(p);
      } else
        args = bprintf("comm at idx %d", value);
      break;
    }

    case SIMCALL_COMM_TESTANY:
      if (value == -1) {
        type = "TestAny FALSE";
        args = xbt_strdup("-");
      } else {
        type = "TestAny";
        args = bprintf("(%d of %zu)", value + 1, simcall_comm_testany__get__count(req));
      }
      break;

    case SIMCALL_MUTEX_TRYLOCK:
    case SIMCALL_MUTEX_LOCK: {
      if (req->call_ == SIMCALL_MUTEX_LOCK)
        type = "Mutex LOCK";
      else
        type = "Mutex TRYLOCK";

      simgrid::mc::Remote<simgrid::kernel::activity::MutexImpl> mutex;
      mc_model_checker->process().read_bytes(mutex.get_buffer(), sizeof(mutex),
                                             remote(req->call_ == SIMCALL_MUTEX_LOCK
                                                        ? simcall_mutex_lock__get__mutex(req)
                                                        : simcall_mutex_trylock__get__mutex(req)));
      args = bprintf("locked = %d, owner = %d, sleeping = n/a", mutex.get_buffer()->is_locked(),
                     mutex.get_buffer()->owner_ != nullptr
                         ? (int)mc_model_checker->process()
                               .resolve_actor(simgrid::mc::remote(mutex.get_buffer()->owner_))
                               ->get_pid()
                         : -1);
      break;
    }

    case SIMCALL_MC_RANDOM:
      type = "MC_RANDOM";
      args = bprintf("%d", value);
      break;

    default:
      type = SIMIX_simcall_name(req->call_);
      args = bprintf("??");
      break;
  }

  std::string str;
  if (args != nullptr)
    str = simgrid::xbt::string_printf("[(%ld)%s (%s)] %s(%s)", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                      MC_smx_actor_get_name(issuer), type, args);
  else
    str = simgrid::xbt::string_printf("[(%ld)%s (%s)] %s ", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                      MC_smx_actor_get_name(issuer), type);
  xbt_free(args);
  return str;
}

namespace simgrid {
namespace mc {

bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx)
{
  kernel::activity::CommImpl* remote_act = nullptr;
  switch (req->call_) {
    case SIMCALL_COMM_WAIT:
      /* FIXME: check also that src and dst processes are not suspended */
      remote_act = simcall_comm_wait__getraw__comm(req);
      break;

    case SIMCALL_COMM_WAITANY:
      remote_act = mc_model_checker->process().read(remote(simcall_comm_testany__get__comms(req) + idx));
      break;

    case SIMCALL_COMM_TESTANY:
      remote_act = mc_model_checker->process().read(remote(simcall_comm_testany__get__comms(req) + idx));
      break;

    default:
      return true;
  }

  Remote<kernel::activity::CommImpl> temp_comm;
  mc_model_checker->process().read(temp_comm, remote(remote_act));
  const kernel::activity::CommImpl* comm = temp_comm.get_buffer();
  return comm->src_actor_.get() && comm->dst_actor_.get();
}

static const char* colors[] = {
    "blue",       "red",    "green3",      "goldenrod", "brown",     "purple", "magenta",
    "turquoise4", "gray25", "forestgreen", "hotpink",   "lightblue", "tan",
};

static inline const char* get_color(int id)
{
  return colors[id % (sizeof(colors) / sizeof(colors[0])) ];
}

std::string request_get_dot_output(smx_simcall_t req, int value)
{
  const smx_actor_t issuer = MC_smx_simcall_get_issuer(req);
  const char* color        = get_color(issuer->get_pid() - 1);

  if (req->inspector_ != nullptr)
    return simgrid::xbt::string_printf("label = \"%s\", color = %s, fontcolor = %s",
                                       req->inspector_->dot_label().c_str(), color, color);

  std::string label;

  switch (req->call_) {
    case SIMCALL_COMM_ISEND:
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] iSend", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
      else
        label = bprintf("[(%ld)] iSend", issuer->get_pid());
      break;

    case SIMCALL_COMM_IRECV:
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] iRecv", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
      else
        label = xbt::string_printf("[(%ld)] iRecv", issuer->get_pid());
      break;

    case SIMCALL_COMM_WAIT:
      if (value == -1) {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] WaitTimeout", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = xbt::string_printf("[(%ld)] WaitTimeout", issuer->get_pid());
      } else {
        kernel::activity::ActivityImpl* remote_act = simcall_comm_wait__getraw__comm(req);
        Remote<kernel::activity::CommImpl> temp_comm;
        mc_model_checker->process().read(temp_comm, remote(static_cast<kernel::activity::CommImpl*>(remote_act)));
        const kernel::activity::CommImpl* comm = temp_comm.get_buffer();

        const kernel::actor::ActorImpl* src_proc =
            mc_model_checker->process().resolve_actor(mc::remote(comm->src_actor_.get()));
        const kernel::actor::ActorImpl* dst_proc =
            mc_model_checker->process().resolve_actor(mc::remote(comm->dst_actor_.get()));
        if (issuer->get_host())
          label =
              xbt::string_printf("[(%ld)%s] Wait [(%ld)->(%ld)]", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                 src_proc ? src_proc->get_pid() : 0, dst_proc ? dst_proc->get_pid() : 0);
        else
          label = xbt::string_printf("[(%ld)] Wait [(%ld)->(%ld)]", issuer->get_pid(),
                                     src_proc ? src_proc->get_pid() : 0, dst_proc ? dst_proc->get_pid() : 0);
      }
      break;

    case SIMCALL_COMM_TEST: {
      kernel::activity::ActivityImpl* remote_act = simcall_comm_test__getraw__comm(req);
      Remote<simgrid::kernel::activity::CommImpl> temp_comm;
      mc_model_checker->process().read(temp_comm, remote(static_cast<kernel::activity::CommImpl*>(remote_act)));
      const kernel::activity::CommImpl* comm = temp_comm.get_buffer();
      if (comm->src_actor_.get() == nullptr || comm->dst_actor_.get() == nullptr) {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] Test FALSE", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = bprintf("[(%ld)] Test FALSE", issuer->get_pid());
      } else {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] Test TRUE", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = xbt::string_printf("[(%ld)] Test TRUE", issuer->get_pid());
      }
      break;
    }

    case SIMCALL_COMM_WAITANY: {
      size_t comms_size = simcall_comm_waitany__get__count(req);
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] WaitAny [%d of %zu]", issuer->get_pid(),
                                   MC_smx_actor_get_host_name(issuer), value + 1, comms_size);
      else
        label = xbt::string_printf("[(%ld)] WaitAny [%d of %zu]", issuer->get_pid(), value + 1, comms_size);
      break;
    }

    case SIMCALL_COMM_TESTANY:
      if (value == -1) {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] TestAny FALSE", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = xbt::string_printf("[(%ld)] TestAny FALSE", issuer->get_pid());
      } else {
        if (issuer->get_host())
          label =
              xbt::string_printf("[(%ld)%s] TestAny TRUE [%d of %lu]", issuer->get_pid(),
                                 MC_smx_actor_get_host_name(issuer), value + 1, simcall_comm_testany__get__count(req));
        else
          label = xbt::string_printf("[(%ld)] TestAny TRUE [%d of %lu]", issuer->get_pid(), value + 1,
                                     simcall_comm_testany__get__count(req));
      }
      break;

    case SIMCALL_MUTEX_TRYLOCK:
      label = xbt::string_printf("[(%ld)] Mutex TRYLOCK", issuer->get_pid());
      break;

    case SIMCALL_MUTEX_LOCK:
      label = xbt::string_printf("[(%ld)] Mutex LOCK", issuer->get_pid());
      break;

    case SIMCALL_MC_RANDOM:
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] MC_RANDOM (%d)", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                   value);
      else
        label = xbt::string_printf("[(%ld)] MC_RANDOM (%d)", issuer->get_pid(), value);
      break;

    default:
      THROW_UNIMPLEMENTED;
  }

  return xbt::string_printf("label = \"%s\", color = %s, fontcolor = %s", label.c_str(), color, color);
}

} // namespace mc
} // namespace simgrid
