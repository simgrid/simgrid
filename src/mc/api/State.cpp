/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/State.hpp"
#include "src/mc/api/guide/BasicGuide.hpp"
//#include "src/mc/api/guide/WaitGuide.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

namespace simgrid::mc {

long State::expended_states_ = 0;

State::State(RemoteApp& remote_app) : num_(++expended_states_)
{
  remote_app.get_actors_status(guide_->actors_to_run_);
  if (_sg_mc_guided == "none")  
      guide_ = std::make_unique<BasicGuide>();
  if (_sg_mc_guided == "nb_wait")
      guide_ = std::make_unique<BasicGuide>();  
  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (num_ % _sg_mc_checkpoint == 0)) || _sg_mc_termination)
    system_state_ = std::make_shared<simgrid::mc::Snapshot>(num_, remote_app.get_page_store(),
                                                            remote_app.get_remote_process_memory());
}

State::State(RemoteApp& remote_app, const State* parent_state)
    : num_(++expended_states_), parent_state_(parent_state)
{
    
    remote_app.get_actors_status(guide_->actors_to_run_);
    if (_sg_mc_guided == "none")  
	guide_ = std::make_unique<BasicGuide>();
    if (_sg_mc_guided == "nb_wait")
	guide_ = std::make_unique<BasicGuide>();	
    
  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (num_ % _sg_mc_checkpoint == 0)) || _sg_mc_termination)
    system_state_ = std::make_shared<simgrid::mc::Snapshot>(num_, remote_app.get_page_store(),
                                                            remote_app.get_remote_process_memory());

  /* If we want sleep set reduction, copy the sleep set and eventually removes things from it */
  if (_sg_mc_sleep_set) {
    /* For each actor in the previous sleep set, keep it if it is not dependent with current transition.
     * And if we kept it and the actor is enabled in this state, mark the actor as already done, so that
     * it is not explored*/
    for (auto& [aid, transition] : parent_state_->get_sleep_set()) {

      if (not parent_state_->get_transition()->depends(&transition)) {

        sleep_set_.emplace(aid, transition);
        if (guide_->actors_to_run_.count(aid) != 0) {
          XBT_DEBUG("Actor %ld will not be explored, for it is in the sleep set", aid);

          guide_->actors_to_run_.at(aid).mark_done();
        }
      } else
        XBT_DEBUG("Transition >>%s<< removed from the sleep set because it was dependent with >>%s<<",
                  transition.to_string().c_str(), parent_state_->get_transition()->to_string().c_str());
    }
  }
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(this->guide_->actors_to_run_, [](auto& pair) { return pair.second.is_todo(); });
}

Transition* State::get_transition() const
{
  return transition_;
}

aid_t State::next_transition() const
{
  XBT_DEBUG("Search for an actor to run. %zu actors to consider", guide_->actors_to_run_.size());
  for (auto const& [aid, actor] : guide_->actors_to_run_) {
    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
    if (not actor.is_todo() || not actor.is_enabled() || actor.is_done()) {

      if (not actor.is_todo())
        XBT_DEBUG("Can't run actor %ld because it is not todo", aid);

      if (not actor.is_enabled())
        XBT_DEBUG("Can't run actor %ld because it is not enabled", aid);

      if (actor.is_done())
        XBT_DEBUG("Can't run actor %ld because it has already been done", aid);

      continue;
    }

    return aid;
  }
  return -1;
}

std::pair<aid_t, double> State::next_transition_guided() const
{
  return guide_->next_transition();
}

// This should be done in GuidedState, or at least interact with it
void State::execute_next(aid_t next, RemoteApp& app)
{
  // This actor is ready to be executed. Execution involves three phases:

  // 1. Identify the appropriate ActorState to prepare for execution
  // when simcall_handle will be called on it
  auto& actor_state                        = guide_->actors_to_run_.at(next);
  const unsigned times_considered          = actor_state.do_consider();
  const auto* expected_executed_transition = actor_state.get_transition(times_considered);
  xbt_assert(expected_executed_transition != nullptr,
             "Expected a transition with %u times considered to be noted in actor %ld", times_considered, next);

  XBT_DEBUG("Let's run actor %ld (times_considered = %u)", next, times_considered);

  // 2. Execute the actor according to the preparation above
  Transition::executed_transitions_++;
  auto* just_executed = app.handle_simcall(next, times_considered, true);
  xbt_assert(just_executed->type_ == expected_executed_transition->type_,
             "The transition that was just executed by actor %ld, viz:\n"
             "%s\n"
             "is not what was purportedly scheduled to execute, which was:\n"
             "%s\n",
             next, just_executed->to_string().c_str(), expected_executed_transition->to_string().c_str());

  // 3. Update the state with the newest information. This means recording
  // both
  //  1. what action was last taken from this state (viz. `executed_transition`)
  //  2. what action actor `next` was able to take given `times_considered`
  // The latter update is important as *more* information is potentially available
  // about a transition AFTER it has executed.
  transition_ = just_executed;

  auto executed_transition = std::unique_ptr<Transition>(just_executed);
  actor_state.set_transition(std::move(executed_transition), times_considered);

  app.wait_for_requests();
}
} // namespace simgrid::mc
