/* libsmx.c - public interface to simix                                       */
/* --------                                                                   */
/* These functions are the only ones that are visible from the higher levels  */
/* (most of them simply add some documentation to the generated simcall body) */
/*                                                                            */
/* This is somehow the "libc" of SimGrid                                      */

/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_replay.hpp"
#include "xbt/random.hpp"
#include <simgrid/Exception.hpp>

#include "popping_bodies.cpp"

#include <boost/core/demangle.hpp>
#include <string>
#include <typeinfo>

/**
 * @ingroup simix_comm_management
 */
void simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                       size_t src_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                       double timeout)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");

  xbt_assert(mbox, "No rendez-vous point defined for send");

  if (MC_is_active() || MC_record_replay_is_active()) {
    /* the model-checker wants two separate simcalls, and wants comm to be nullptr during the simcall */
    simgrid::kernel::activity::ActivityImplPtr comm = nullptr;

    simgrid::kernel::actor::CommIsendSimcall send_observer{
        sender,  mbox,          task_size, rate, static_cast<unsigned char*>(src_buff), src_buff_size, match_fun,
        nullptr, copy_data_fun, data,      false};
    comm = simgrid::kernel::actor::simcall(
        [&send_observer] { return simgrid::kernel::activity::CommImpl::isend(&send_observer); }, &send_observer);

    simgrid::kernel::actor::ActivityWaitSimcall wait_observer{sender, comm.get(), timeout};
    if (simgrid::kernel::actor::simcall_blocking(
            [&wait_observer] {
              wait_observer.get_activity()->wait_for(wait_observer.get_issuer(), wait_observer.get_timeout());
            },
            &wait_observer)) {
      throw simgrid::TimeoutException(XBT_THROW_POINT, "Timeouted");
    }
    comm = nullptr;
  }
  else {
    simgrid::kernel::actor::CommIsendSimcall observer(sender, mbox, task_size, rate,
                                                      static_cast<unsigned char*>(src_buff), src_buff_size, match_fun,
                                                      nullptr, copy_data_fun, data, false);
    simgrid::kernel::actor::simcall_blocking([&observer, timeout] {
      simgrid::kernel::activity::ActivityImplPtr comm = simgrid::kernel::activity::CommImpl::isend(&observer);
      comm->wait_for(observer.get_issuer(), timeout);
    });
  }
}

/**
 * @ingroup simix_comm_management
 */
simgrid::kernel::activity::ActivityImplPtr
simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                   size_t src_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*clean_fun)(void*), void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                   void* data, bool detached)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");

  xbt_assert(mbox, "No rendez-vous point defined for isend");

  simgrid::kernel::actor::CommIsendSimcall observer(sender, mbox, task_size, rate,
                                                    static_cast<unsigned char*>(src_buff), src_buff_size, match_fun,
                                                    clean_fun, copy_data_fun, data, detached);
  return simgrid::kernel::actor::simcall([&observer] { return simgrid::kernel::activity::CommImpl::isend(&observer); });
}

/**
 * @ingroup simix_comm_management
 */
void simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                       bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                       double timeout, double rate)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  xbt_assert(mbox, "No rendez-vous point defined for recv");

  if (MC_is_active() || MC_record_replay_is_active()) {
    /* the model-checker wants two separate simcalls, and wants comm to be nullptr during the simcall */
    simgrid::kernel::activity::ActivityImplPtr comm = nullptr;

    simgrid::kernel::actor::CommIrecvSimcall observer{
        receiver, mbox, static_cast<unsigned char*>(dst_buff), dst_buff_size, match_fun, copy_data_fun, data, rate};
    comm = simgrid::kernel::actor::simcall(
        [&observer] { return simgrid::kernel::activity::CommImpl::irecv(&observer); }, &observer);

    simgrid::kernel::actor::ActivityWaitSimcall wait_observer{receiver, comm.get(), timeout};
    if (simgrid::kernel::actor::simcall_blocking(
            [&wait_observer] {
              wait_observer.get_activity()->wait_for(wait_observer.get_issuer(), wait_observer.get_timeout());
            },
            &wait_observer)) {
      throw simgrid::TimeoutException(XBT_THROW_POINT, "Timeouted");
    }
    comm = nullptr;
  }
  else {
    simgrid::kernel::actor::CommIrecvSimcall observer(receiver, mbox, static_cast<unsigned char*>(dst_buff),
                                                      dst_buff_size, match_fun, copy_data_fun, data, rate);
    simgrid::kernel::actor::simcall_blocking([&observer, timeout] {
      simgrid::kernel::activity::ActivityImplPtr comm = simgrid::kernel::activity::CommImpl::irecv(&observer);
      comm->wait_for(observer.get_issuer(), timeout);
    });
  }
}
/**
 * @ingroup simix_comm_management
 */
simgrid::kernel::activity::ActivityImplPtr
simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                   bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data, double rate)
{
  xbt_assert(mbox, "No rendez-vous point defined for irecv");

  simgrid::kernel::actor::CommIrecvSimcall observer(receiver, mbox, static_cast<unsigned char*>(dst_buff),
                                                    dst_buff_size, match_fun, copy_data_fun, data, rate);
  return simgrid::kernel::actor::simcall([&observer] { return simgrid::kernel::activity::CommImpl::irecv(&observer); });
}

/**
 * @ingroup simix_comm_management
 */
ssize_t simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count,
                             double timeout) // XBT_ATTRIB_DEPRECATED_v335
{
  return simcall_BODY_comm_waitany(comms, count, timeout);
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
void simcall_comm_wait(simgrid::kernel::activity::ActivityImpl* comm, double timeout)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simcall_BODY_comm_wait(static_cast<simgrid::kernel::activity::CommImpl*>(comm), timeout);
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

void simcall_run_kernel(std::function<void()> const& code, simgrid::kernel::actor::SimcallObserver* observer)
{
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = observer;
  simcall_BODY_run_kernel(&code);
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = nullptr;
}

void simcall_run_blocking(std::function<void()> const& code, simgrid::kernel::actor::SimcallObserver* observer)
{
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = observer;
  simcall_BODY_run_blocking(&code);
  simgrid::kernel::actor::ActorImpl::self()->simcall_.observer_ = nullptr;
}

/* ************************************************************************** */

/** @brief returns a printable string representing a simcall */
const char* SIMIX_simcall_name(const s_smx_simcall& simcall)
{
  if (simcall.observer_ != nullptr) {
#if SIMGRID_HAVE_MC
    if (mc_model_checker != nullptr) // Do not try to use the observer from the MCer
      return "(remotely observed)";
#endif

    static std::string name;
    name              = boost::core::demangle(typeid(*simcall.observer_).name());
    const char* cname = name.c_str();
    if (name.rfind("simgrid::kernel::", 0) == 0)
      cname += 17; // strip prefix "simgrid::kernel::"
    return cname;
  } else {
    return simcall_names.at(static_cast<int>(simcall.call_));
  }
}

namespace simgrid {
namespace simix {

void unblock(smx_actor_t actor)
{
  xbt_assert(s4u::Actor::is_maestro());
  actor->simcall_answer();
}
} // namespace simix
} // namespace simgrid
