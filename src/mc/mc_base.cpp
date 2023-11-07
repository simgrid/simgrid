/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_base.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

#include "src/mc/mc.h"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(mc, "All MC categories");
bool simgrid_mc_replay_show_backtraces = false;

namespace simgrid::mc {

void execute_actors()
{
  auto* engine = kernel::EngineImpl::get_instance();

  XBT_DEBUG("execute_actors: %lu of %zu to run (%s)", engine->get_actor_to_run_count(), engine->get_actor_count(),
            (MC_record_replay_is_active() ? "replay active" : "no replay"));
  while (engine->has_actors_to_run()) {
    engine->run_all_actors();
    for (auto const& actor : engine->get_actors_that_ran()) {
      const kernel::actor::Simcall* req = &actor->simcall_;
      if (req->call_ != kernel::actor::Simcall::Type::NONE && not simgrid::mc::request_is_visible(req))
        actor->simcall_handle(0);
    }
  }
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
bool actor_is_enabled(kernel::actor::ActorImpl* actor)
{
  xbt_assert(get_model_checking_mode() != ModelCheckingMode::CHECKER_SIDE,
             "This should be called from the client side");

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
  xbt_assert(get_model_checking_mode() != ModelCheckingMode::CHECKER_SIDE,
             "This should be called from the client side");

  if (req->observer_ == nullptr)
    return false;
  return req->observer_->is_visible();
}

} // namespace simgrid::mc
