/* Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/barrier.h>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Barrier.hpp>

#include "src/kernel/activity/BarrierImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_barrier, s4u, "S4U barrier");

namespace simgrid {
namespace s4u {

/** @brief Create a new barrier
 *
 * See @ref s4u_raii.
 */
BarrierPtr Barrier::create(unsigned int expected_actors)
{
  auto* res = new kernel::activity::BarrierImpl(expected_actors);
  return BarrierPtr(&res->piface_);
}

/** @brief Block the current actor until all expected actors reach the barrier.
 *
 * This method is meant to be somewhat consistent with the pthread_barrier_wait function.
 *
 * @return false for all actors but one: exactly one actor will get true as a return value.
 */
int Barrier::wait()
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();

  if (MC_is_active() || MC_record_replay_is_active()) { // Split in 2 simcalls for transition persistency
    kernel::actor::BarrierObserver lock_observer{issuer, mc::Transition::Type::BARRIER_LOCK, pimpl_};
    auto acquisition =
        kernel::actor::simcall_answered([issuer, this] { return pimpl_->acquire_async(issuer); }, &lock_observer);

    kernel::actor::BarrierObserver wait_observer{issuer, mc::Transition::Type::BARRIER_WAIT, acquisition.get()};
    return kernel::actor::simcall_blocking([issuer, acquisition] { return acquisition->wait_for(issuer, -1); },
                                           &wait_observer);

  } else { // Do it in one simcall only
    kernel::activity::BarrierAcquisitionImpl* acqui = nullptr; // unused here, but must be typed to pick the right ctor
    kernel::actor::BarrierObserver observer{issuer, mc::Transition::Type::BARRIER_WAIT, acqui};
    return kernel::actor::simcall_blocking([issuer, this] { pimpl_->acquire_async(issuer)->wait_for(issuer, -1); },
                                           &observer);
  }
}

void intrusive_ptr_add_ref(Barrier* barrier)
{
  intrusive_ptr_add_ref(barrier->pimpl_);
}

void intrusive_ptr_release(Barrier* barrier)
{
  intrusive_ptr_release(barrier->pimpl_);
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

sg_bar_t sg_barrier_init(unsigned int count)
{
  simgrid::s4u::BarrierPtr bar = simgrid::s4u::Barrier::create(count);
  intrusive_ptr_add_ref(bar.get());
  return bar.get();
}

/** @brief Initializes a barrier, with count elements */
void sg_barrier_destroy(sg_bar_t bar)
{
  intrusive_ptr_release(bar);
}

/** @brief Performs a barrier already initialized.
 *
 * @return 0 for all actors but one: exactly one actor will get SG_BARRIER_SERIAL_THREAD as a return value. */
int sg_barrier_wait(sg_bar_t bar)
{
  if (bar->wait())
    return SG_BARRIER_SERIAL_THREAD;
  return 0;
}
