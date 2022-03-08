/* libsmx.c - public interface to simix                                       */
/* --------                                                                   */
/* These functions are the only ones that are visible from the higher levels  */
/* (most of them simply add some documentation to the generated simcall body) */
/*                                                                            */
/* This is somehow the "libc" of SimGrid                                      */

/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include <simgrid/s4u/Activity.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix);

/**
 * @ingroup simix_comm_management
 */
void simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                       size_t src_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                       double timeout) // XBT_ATTRIB_DEPRECATED_v335
{
  xbt_assert(mbox, "No rendez-vous point defined for send");
  simgrid::s4u::Comm::send(sender, mbox->get_iface(), task_size, rate, src_buff, src_buff_size, match_fun,
                           copy_data_fun, data, timeout);
}

/**
 * @ingroup simix_comm_management
 */
simgrid::kernel::activity::ActivityImplPtr
simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                   size_t src_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*clean_fun)(void*), void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                   void* data, bool detached) // XBT_ATTRIB_DEPRECATED_v335
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");

  xbt_assert(mbox, "No rendez-vous point defined for isend");

  simgrid::kernel::actor::CommIsendSimcall observer(sender, mbox, task_size, rate,
                                                    static_cast<unsigned char*>(src_buff), src_buff_size, match_fun,
                                                    clean_fun, copy_data_fun, data, detached);
  return simgrid::kernel::actor::simcall_answered(
      [&observer] { return simgrid::kernel::activity::CommImpl::isend(&observer); });
}

/**
 * @ingroup simix_comm_management
 */
void simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                       bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                       double timeout, double rate) // XBT_ATTRIB_DEPRECATED_v335
{
  xbt_assert(mbox, "No rendez-vous point defined for recv");
  simgrid::s4u::Comm::recv(receiver, mbox->get_iface(), dst_buff, dst_buff_size, match_fun, copy_data_fun, data,
                           timeout, rate);
}

/**
 * @ingroup simix_comm_management
 */
simgrid::kernel::activity::ActivityImplPtr
simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                   bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                   double rate) // XBT_ATTRIB_DEPRECATED_v335
{
  xbt_assert(mbox, "No rendez-vous point defined for irecv");

  simgrid::kernel::actor::CommIrecvSimcall observer(receiver, mbox, static_cast<unsigned char*>(dst_buff),
                                                    dst_buff_size, match_fun, copy_data_fun, data, rate);
  return simgrid::kernel::actor::simcall_answered(
      [&observer] { return simgrid::kernel::activity::CommImpl::irecv(&observer); });
}

/**
 * @ingroup simix_comm_management
 */
ssize_t simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count,
                             double timeout) // XBT_ATTRIB_DEPRECATED_v335
{
  std::vector<simgrid::kernel::activity::ActivityImpl*> activities;
  for (size_t i = 0; i < count; i++)
    activities.push_back(static_cast<simgrid::kernel::activity::ActivityImpl*>(comms[i]));
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::ActivityWaitanySimcall observer{issuer, activities, timeout};
  ssize_t changed_pos = simgrid::kernel::actor::simcall_blocking(
      [&observer] {
        simgrid::kernel::activity::ActivityImpl::wait_any_for(observer.get_issuer(), observer.get_activities(),
                                                              observer.get_timeout());
      },
      &observer);
  if (changed_pos != -1)
    activities.at(changed_pos)->get_iface()->complete(simgrid::s4u::Activity::State::FINISHED);
  return changed_pos;
}

/**
 * @ingroup simix_comm_management
 */
ssize_t simcall_comm_testany(simgrid::kernel::activity::CommImpl* comms[], size_t count) // XBT_ATTRIB_DEPRECATED_v335
{
  if (count == 0)
    return -1;
  std::vector<simgrid::kernel::activity::ActivityImpl*> activities;
  for (size_t i = 0; i < count; i++)
    activities.push_back(static_cast<simgrid::kernel::activity::ActivityImpl*>(comms[i]));

  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::ActivityTestanySimcall observer{issuer, activities};
  ssize_t changed_pos = simgrid::kernel::actor::simcall_blocking(
      [&observer] {
        simgrid::kernel::activity::ActivityImpl::test_any(observer.get_issuer(), observer.get_activities());
      },
      &observer);
  if (changed_pos != -1)
    comms[changed_pos]->get_iface()->complete(simgrid::s4u::Activity::State::FINISHED);
  return changed_pos;
}

/**
 * @ingroup simix_comm_management
 */
void simcall_comm_wait(simgrid::kernel::activity::ActivityImpl* comm, double timeout) // XBT_ATTRIB_DEPRECATED_v335
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::simcall_blocking([issuer, comm, timeout] { comm->wait_for(issuer, timeout); });
}

/**
 * @ingroup simix_comm_management
 *
 */
bool simcall_comm_test(simgrid::kernel::activity::ActivityImpl* comm) // XBT_ATTRIB_DEPRECATED_v335
{
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::ActivityTestSimcall observer{issuer, comm};
  if (simgrid::kernel::actor::simcall_blocking([&observer] { observer.get_activity()->test(observer.get_issuer()); },
                                               &observer)) {
    comm->get_iface()->complete(simgrid::s4u::Activity::State::FINISHED);
    return true;
  }
  return false;
}

static void simcall(simgrid::simix::Simcall call, std::function<void()> const& code)
{
  auto self = simgrid::kernel::actor::ActorImpl::self();
  self->simcall_.call_ = call;
  self->simcall_.code_ = &code;
  if (not simgrid::kernel::EngineImpl::get_instance()->is_maestro(self)) {
    XBT_DEBUG("Yield process '%s' on simcall %s", self->get_cname(), SIMIX_simcall_name(self->simcall_));
    self->yield();
  } else {
    self->simcall_handle(0);
  }
}

void simcall_run_answered(std::function<void()> const& code, simgrid::kernel::actor::SimcallObserver* observer)
{
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = observer;
  // The function `code` is called in kernel mode (either because we are already in maestor or after a context switch)
  // and simcall_answer() is called
  simcall(simgrid::simix::Simcall::RUN_ANSWERED, code);
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = nullptr;
}

void simcall_run_blocking(std::function<void()> const& code, simgrid::kernel::actor::SimcallObserver* observer)
{
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = observer;
  // The function `code` is called in kernel mode (either because we are already in maestor or after a context switch)
  // BUT simcall_answer IS NOT CALLED
  simcall(simgrid::simix::Simcall::RUN_BLOCKING, code);
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = nullptr;
}
