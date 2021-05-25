/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_base.hpp"
#include "mc/mc.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"

#include "xbt/random.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/remote/RemoteProcess.hpp"

using simgrid::mc::remote;
#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(mc, "All MC categories");

int MC_random(int min, int max)
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr);
#endif
  if (not MC_is_active() && not MC_record_replay_is_active()) { // no need to do a simcall in this case
    static simgrid::xbt::random::XbtRandom prng;
    return prng.uniform_int(min, max);
  }
  simgrid::kernel::actor::RandomSimcall observer{SIMIX_process_self(), min, max};
  return simgrid::kernel::actor::simcall([&observer] { return observer.get_value(); }, &observer);
}

namespace simgrid {
namespace mc {

void execute_actors()
{
  auto* engine = kernel::EngineImpl::get_instance();
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr, "This must be called from the client");
#endif
  while (engine->has_actors_to_run()) {
    engine->run_all_actors();
    for (auto const& actor : engine->get_actors_that_ran()) {
      const s_smx_simcall* req = &actor->simcall_;
      if (req->call_ != simix::Simcall::NONE && not simgrid::mc::request_is_visible(req))
        actor->simcall_handle(0);
    }
  }
#if SIMGRID_HAVE_MC
  engine->reset_actor_dynar();
  for (auto const& kv : engine->get_actor_list()) {
    auto actor = kv.second;
    if (actor->simcall_.observer_ != nullptr)
      actor->simcall_.mc_max_consider_ = actor->simcall_.observer_->get_max_consider();
    engine->add_actor_to_dynar(actor);
  }
#endif
}

/** @brief returns if there this transition can proceed in a finite amount of time
 *
 * It is used in the model-checker to not get into self-deadlock where it would execute a never ending transition.
 *
 * Only WAIT operations (on comm, on mutex, etc) can ever return false because they could lock the MC exploration.
 * Wait operations are OK and return true in only two situations:
 *  - if the wait will succeed immediately (if both peer of the comm are there already or if the mutex is available)
 *  - if a timeout is provided, because we can fire the timeout if the transition is not ready without blocking in this
 * transition for ever.
 *
 */
// Called from both MCer and MCed:
bool actor_is_enabled(smx_actor_t actor)
{
// #del
#if SIMGRID_HAVE_MC
  // If in the MCer, ask the client app since it has all the data
  if (mc_model_checker != nullptr) {
    return simgrid::mc::session_singleton->actor_is_enabled(actor->get_pid());
  }
#endif
// #

  // Now, we are in the client app, no need for remote memory reading.
  smx_simcall_t req = &actor->simcall_;

  if (req->observer_ != nullptr)
    return req->observer_->is_enabled();

  using simix::Simcall;
  switch (req->call_) {
    case Simcall::NONE:
      return false;

    case Simcall::COMM_WAIT: {
      /* FIXME: check also that src and dst processes are not suspended */
      const kernel::activity::CommImpl* act = simcall_comm_wait__getraw__comm(req);

      if (act->src_timeout_ || act->dst_timeout_) {
        /* If it has a timeout it will be always be enabled (regardless of who declared the timeout),
         * because even if the communication is not ready, it can timeout and won't block. */
        if (_sg_mc_timeout == 1)
          return true;
      }
      /* On the other hand if it hasn't a timeout, check if the comm is ready.*/
      else if (act->detached() && act->src_actor_ == nullptr && act->state_ == simgrid::kernel::activity::State::READY)
        return (act->dst_actor_ != nullptr);
      return (act->src_actor_ && act->dst_actor_);
    }

    case Simcall::COMM_WAITANY: {
      simgrid::kernel::activity::CommImpl** comms = simcall_comm_waitany__get__comms(req);
      size_t count                                = simcall_comm_waitany__get__count(req);
      for (unsigned int index = 0; index < count; ++index) {
        auto const* comm = comms[index];
        if (comm->src_actor_ && comm->dst_actor_)
          return true;
      }
      return false;
    }

    default:
      /* The rest of the requests are always enabled */
      return true;
  }
}

/* This is the list of requests that are visible from the checker algorithm.
 * Any other requests are handled right away on the application side.
 */
bool request_is_visible(const s_smx_simcall* req)
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr, "This should be called from the client side");
#endif
  if (req->observer_ != nullptr)
    return req->observer_->is_visible();

  using simix::Simcall;
  return req->call_ == Simcall::COMM_ISEND || req->call_ == Simcall::COMM_IRECV || req->call_ == Simcall::COMM_WAIT ||
         req->call_ == Simcall::COMM_WAITANY || req->call_ == Simcall::COMM_TEST || req->call_ == Simcall::COMM_TESTANY;
}

}
}
