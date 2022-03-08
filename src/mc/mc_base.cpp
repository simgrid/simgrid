/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

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
  simgrid::kernel::actor::RandomSimcall observer{simgrid::kernel::actor::ActorImpl::self(), min, max};
  return simgrid::kernel::actor::simcall_answered([&observer] { return observer.get_value(); }, &observer);
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
      const kernel::actor::Simcall* req = &actor->simcall_;
      if (req->call_ != kernel::actor::Simcall::Type::NONE && not simgrid::mc::request_is_visible(req))
        actor->simcall_handle(0);
    }
  }
#if SIMGRID_HAVE_MC
  engine->reset_actor_dynar();
  for (auto const& kv : engine->get_actor_list()) {
    auto actor = kv.second;
    // Only visible requests remain at this point, and they all have an observer
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
 * This is controlled in the is_enabled() method of the corresponding observers.
 */
// Called from both MCer and MCed:
bool actor_is_enabled(smx_actor_t actor)
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr, "This should be called from the client side");
#endif

  // Now, we are in the client app, no need for remote memory reading.
  kernel::actor::Simcall* req = &actor->simcall_;

  if (req->observer_ != nullptr)
    return req->observer_->is_enabled();

  if (req->call_ == kernel::actor::Simcall::Type::NONE)
    return false;
  else
    /* The rest of the requests are always enabled */
    return true;
}

/* This is the list of requests that are visible from the checker algorithm.
 * Any other requests are handled right away on the application side.
 */
bool request_is_visible(const kernel::actor::Simcall* req)
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr, "This should be called from the client side");
#endif
  if (req->observer_ != nullptr)
    return req->observer_->is_visible();
  else
    return false;
}

} // namespace mc
} // namespace simgrid
