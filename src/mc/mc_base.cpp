/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_base.hpp"
#include "src/kernel/EngineImpl.hpp"
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
  XBT_DEBUG("all actors are done!");
}

static const char* warning_message =
    "WARNING: the checker seems to be behaving as an application. To verify McSimGrid itself, you must:\n"
    " * Compile sthread with `enable_lib_in_sthread` in cmake, to ensure that the SimGrid shipped with sthread\n"
    "   does not interfere with the one living in the verified McSimGrid. This message shall disappear after that.\n"
    " * The verified McSimGrid must run with the thread factory since the other context factories are not\n"
    "   observed by McSimGrid, so the verifying McSimGrid will not see them.\n"
    " * The verifying McSimGrid needs `no-fork`, as threaded applications cannot be verified without it.\n"
    " * You will get a stack overflow if the verifying McSimGrid runs in DFS; use BeFS (and buy more memory).\n"
    " * The verified McSimGrid can be compiled with mcclang to track dataraces (buy several tons of memory).\n"
    " * Finally, the application verified by the verified McSimGrid shall be short and simple, but you want\n"
    "   it to NOT to lead to an assertion failure, to test more behaviors in the verified McSimGrid.\n"
    " * Make sure that the external application is not intercepted by sthread as follows\n"
    "   `export STHREAD_IGNORE_BINARY=s4u-mc-centralized-mutex`\n"
    "\nIn practice, you may want to run something such as:\n"
    " lib_in_sthread_and_clang_build/bin/simgrid-mc --sthread --cfg=model-check/exploration-algo:BeFS \\\n"
    "                                    --cfg=model-check/strategy:uniform --cfg=model-check/no-fork:on \\\n"
    "  -- \\\n"
    "  mcclang_build/bin/simgrid-mc --cfg=contexts/factory:thread --cfg=model-check/reduction:odpor \\\n"
    "                     --cfg=model-check/exploration-algo:parallel --cfg=model-check/parallel-thread:2 \\\n"
    "  -- \\\n"
    "  path/to/s4u-mc-centralized-mutex path/to/small_platform.xml --log=root.t:critical";

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
  xbt_assert(get_model_checking_mode() != ModelCheckingMode::CHECKER_SIDE, "%s", warning_message);

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
  xbt_assert(get_model_checking_mode() != ModelCheckingMode::CHECKER_SIDE, "%s", warning_message);

  if (req->observer_ == nullptr)
    return false;
  return req->observer_->is_visible();
}

} // namespace simgrid::mc
