/* libsmx.c - public interface to simix                                       */
/* --------                                                                   */
/* These functions are the only ones that are visible from the higher levels  */
/* (most of them simply add some documentation to the generated simcall body) */
/*                                                                            */
/* This is somehow the "libc" of SimGrid                                      */

/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "xbt/random.hpp"

#include "popping_bodies.cpp"

#include <boost/core/demangle.hpp>
#include <string>
#include <typeinfo>
XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");

/**
 * @ingroup simix_host_management
 * @brief Waits for the completion of an execution synchro and destroy it.
 *
 * @param execution The execution synchro
 */
simgrid::kernel::activity::State simcall_execution_wait(simgrid::kernel::activity::ActivityImpl* execution,
                                                        double timeout) // XBT_ATTRIB_DEPRECATED_v330
{
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::simcall_blocking([execution, issuer, timeout] { execution->wait_for(issuer, timeout); });
  return simgrid::kernel::activity::State::DONE;
}

simgrid::kernel::activity::State simcall_execution_wait(const simgrid::kernel::activity::ActivityImplPtr& execution,
                                                        double timeout) // XBT_ATTRIB_DEPRECATED_v330
{
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::simcall_blocking([execution, issuer, timeout] { execution->wait_for(issuer, timeout); });
  return simgrid::kernel::activity::State::DONE;
}

bool simcall_execution_test(simgrid::kernel::activity::ActivityImpl* execution) // XBT_ATTRIB_DEPRECATED_v330
{
  return simgrid::kernel::actor::simcall([execution] { return execution->test(); });
}

bool simcall_execution_test(const simgrid::kernel::activity::ActivityImplPtr& execution) // XBT_ATTRIB_DEPRECATED_v330
{
  return simgrid::kernel::actor::simcall([execution] { return execution->test(); });
}

unsigned int simcall_execution_waitany_for(simgrid::kernel::activity::ExecImpl* execs[], size_t count,
                                           double timeout) // XBT_ATTRIB_DEPRECATED_v331
{
  std::vector<simgrid::kernel::activity::ExecImpl*> execs_vec(execs, execs + count);
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::ExecutionWaitanySimcall observer{issuer, execs_vec, timeout};
  return simgrid::kernel::actor::simcall_blocking(
      [&observer] {
        simgrid::kernel::activity::ExecImpl::wait_any_for(observer.get_issuer(), observer.get_execs(),
                                                          observer.get_timeout());
      },
      &observer);
}

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
    /* the model-checker wants two separate simcalls */
    simgrid::kernel::activity::ActivityImplPtr comm =
        nullptr; /* MC needs the comm to be set to nullptr during the simcall */
    comm = simcall_comm_isend(sender, mbox, task_size, rate, src_buff, src_buff_size, match_fun, nullptr, copy_data_fun,
                              data, false);
    simcall_comm_wait(comm.get(), timeout);
    comm = nullptr;
  }
  else {
    simcall_BODY_comm_send(sender, mbox, task_size, rate, static_cast<unsigned char*>(src_buff), src_buff_size,
                           match_fun, copy_data_fun, data, timeout);
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

  return simcall_BODY_comm_isend(sender, mbox, task_size, rate, static_cast<unsigned char*>(src_buff), src_buff_size,
                                 match_fun, clean_fun, copy_data_fun, data, detached);
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
    /* the model-checker wants two separate simcalls */
    simgrid::kernel::activity::ActivityImplPtr comm =
        nullptr; /* MC needs the comm to be set to nullptr during the simcall */
    comm = simcall_comm_irecv(receiver, mbox, dst_buff, dst_buff_size,
                              match_fun, copy_data_fun, data, rate);
    simcall_comm_wait(comm.get(), timeout);
    comm = nullptr;
  }
  else {
    simcall_BODY_comm_recv(receiver, mbox, static_cast<unsigned char*>(dst_buff), dst_buff_size, match_fun,
                           copy_data_fun, data, timeout, rate);
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

  return simcall_BODY_comm_irecv(receiver, mbox, static_cast<unsigned char*>(dst_buff), dst_buff_size, match_fun,
                                 copy_data_fun, data, rate);
}

/**
 * @ingroup simix_comm_management
 */
simgrid::kernel::activity::ActivityImplPtr
simcall_comm_iprobe(smx_mailbox_t mbox, int type, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                    void* data) // XBT_ATTRIB_DEPRECATED_v330
{
  xbt_assert(mbox, "No rendez-vous point defined for iprobe");

  return simgrid::kernel::actor::simcall([mbox, type, match_fun, data] { return mbox->iprobe(type, match_fun, data); });
}

/**
 * @ingroup simix_comm_management
 */
unsigned int simcall_comm_waitany(simgrid::kernel::activity::ActivityImplPtr comms[], size_t count,
                                  double timeout) // XBT_ATTRIB_DEPRECATED_v330
{
  std::vector<simgrid::kernel::activity::CommImpl*> rcomms(count);
  std::transform(comms, comms + count, begin(rcomms), [](const simgrid::kernel::activity::ActivityImplPtr& comm) {
    return static_cast<simgrid::kernel::activity::CommImpl*>(comm.get());
  });
  return static_cast<unsigned int>(simcall_BODY_comm_waitany(rcomms.data(), rcomms.size(), timeout));
}

ssize_t simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count, double timeout)
{
  return simcall_BODY_comm_waitany(comms, count, timeout);
}

/**
 * @ingroup simix_comm_management
 */
int simcall_comm_testany(simgrid::kernel::activity::ActivityImplPtr comms[], size_t count) // XBT_ATTRIB_DEPRECATED_v330
{
  if (count == 0)
    return -1;
  std::vector<simgrid::kernel::activity::CommImpl*> rcomms(count);
  std::transform(comms, comms + count, begin(rcomms), [](const simgrid::kernel::activity::ActivityImplPtr& comm) {
    return static_cast<simgrid::kernel::activity::CommImpl*>(comm.get());
  });
  return static_cast<int>(simcall_BODY_comm_testany(rcomms.data(), rcomms.size()));
}

ssize_t simcall_comm_testany(simgrid::kernel::activity::CommImpl* comms[], size_t count)
{
  if (count == 0)
    return -1;
  return simcall_BODY_comm_testany(comms, count);
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
bool simcall_comm_test(simgrid::kernel::activity::ActivityImpl* comm)
{
  return simcall_BODY_comm_test(static_cast<simgrid::kernel::activity::CommImpl*>(comm));
}

/**
 * @ingroup simix_synchro_management
 *
 */
smx_mutex_t simcall_mutex_init() // XBT_ATTRIB_DEPRECATED_v330
{
  return simgrid::kernel::actor::simcall([] { return new simgrid::kernel::activity::MutexImpl(); });
}

/**
 * @ingroup simix_synchro_management
 *
 */
void simcall_mutex_lock(smx_mutex_t mutex) // XBT_ATTRIB_DEPRECATD_v331
{
  mutex->mutex().lock();
}

/**
 * @ingroup simix_synchro_management
 *
 */
int simcall_mutex_trylock(smx_mutex_t mutex) // XBT_ATTRIB_DEPRECATD_v331
{
  return mutex->mutex().try_lock();
}

/**
 * @ingroup simix_synchro_management
 *
 */
void simcall_mutex_unlock(smx_mutex_t mutex) // XBT_ATTRIB_DEPRECATD_v331
{
  mutex->mutex().unlock();
}

/**
 * @ingroup simix_synchro_management
 *
 */
smx_cond_t simcall_cond_init() // XBT_ATTRIB_DEPRECATED_v330
{
  return simgrid::kernel::actor::simcall([] { return new simgrid::kernel::activity::ConditionVariableImpl(); });
}

/**
 * @ingroup simix_synchro_management
 *
 */
void simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex) // XBT_ATTRIB_DEPRECATED_v331
{
  cond->get_iface()->wait(std::unique_lock<simgrid::s4u::Mutex>(mutex->mutex()));
}

/**
 * @ingroup simix_synchro_management
 *
 */
int simcall_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double timeout) // XBT_ATTRIB_DEPRECATD_v331
{
  return cond->get_iface()->wait_for(std::unique_lock<simgrid::s4u::Mutex>(mutex->mutex()), timeout) ==
         std::cv_status::timeout;
}

/**
 * @ingroup simix_synchro_management
 *
 */
void simcall_sem_acquire(smx_sem_t sem) // XBT_ATTRIB_DEPRECATD_v331
{
  return sem->sem().acquire();
}

/**
 * @ingroup simix_synchro_management
 *
 */
int simcall_sem_acquire_timeout(smx_sem_t sem, double timeout) // XBT_ATTRIB_DEPRECATD_v331
{
  return sem->sem().acquire_timeout(timeout);
}

simgrid::kernel::activity::State simcall_io_wait(simgrid::kernel::activity::ActivityImpl* io,
                                                 double timeout) // XBT_ATTRIB_DEPRECATED_v330
{
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::simcall_blocking([io, issuer, timeout] { io->wait_for(issuer, timeout); });
  return simgrid::kernel::activity::State::DONE;
}

simgrid::kernel::activity::State simcall_io_wait(const simgrid::kernel::activity::ActivityImplPtr& io,
                                                 double timeout) // XBT_ATTRIB_DEPRECATED_v330
{
  simgrid::kernel::actor::ActorImpl* issuer = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::simcall_blocking([io, issuer, timeout] { io->wait_for(issuer, timeout); });
  return simgrid::kernel::activity::State::DONE;
}

bool simcall_io_test(simgrid::kernel::activity::ActivityImpl* io) // XBT_ATTRIB_DEPRECATED_v330
{
  return simgrid::kernel::actor::simcall([io] { return io->test(); });
}

bool simcall_io_test(const simgrid::kernel::activity::ActivityImplPtr& io) // XBT_ATTRIB_DEPRECATD_v330
{
  return simgrid::kernel::actor::simcall([io] { return io->test(); });
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

int simcall_mc_random(int min, int max) // XBT_ATTRIB_DEPRECATD_v331
{
  return MC_random(min, max);
}

/* ************************************************************************** */

/** @brief returns a printable string representing a simcall */
const char* SIMIX_simcall_name(const s_smx_simcall& simcall)
{
  if (simcall.observer_ != nullptr) {
    static std::string name;
    name              = boost::core::demangle(typeid(*simcall.observer_).name());
    const char* cname = name.c_str();
    if (name.rfind("simgrid::kernel::", 0) == 0)
      cname += 17; // strip prefix "simgrid::kernel::"
    return cname;
  } else {
    return simcall_names[static_cast<int>(simcall.call_)];
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
