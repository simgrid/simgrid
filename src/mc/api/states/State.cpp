/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/State.hpp"
#include "src/mc/api/ActorState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"

#include <algorithm>
#include <boost/range/algorithm.hpp>
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

namespace simgrid::mc {

long State::expended_states_  = 0;
long State::in_memory_states_ = 0;

State::~State()
{
  in_memory_states_--;
  XBT_VERB("Closing state n°%ld! There are %ld remaining states", this->get_num(), get_in_memory_states());
}

State::State(const RemoteApp& remote_app, bool set_actor_status) : num_(++expended_states_)
{
  in_memory_states_++;
  if (set_actor_status) {
    actor_status_set_ = true;
    remote_app.get_actors_status(actors_to_run_);
  }
  if (get_num() == 1)
    is_leftmost_ = true; // The first state is the only one at that depth, so the leftmost one.
}

State::State(const RemoteApp& remote_app, StatePtr parent_state, std::shared_ptr<Transition> incoming_transition,
             bool set_actor_status)
    : State(remote_app, set_actor_status)
{
  xbt_assert(incoming_transition != nullptr);
  parent_state_        = std::move(parent_state);
  incoming_transition_ = std::move(incoming_transition);
  depth_               = parent_state_->depth_ + 1;

  is_leftmost_ = parent_state_->is_leftmost_ and parent_state_->opened_.size() == parent_state_->closed_.size();
  parent_state_->update_opened(get_transition_in());
  parent_state_->record_child_state(this);

  XBT_DEBUG("Creating %ld, son of %ld", get_num(), parent_state_->get_num());
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(actors_to_run_, [](auto& pair) { return pair.second.is_todo(); });
}

bool State::has_more_to_be_explored() const
{
  size_t count = 0;
  for (auto const& [_, actor] : actors_to_run_)
    if (actor.is_todo())
      count += actor.get_times_not_considered();

  return count > 0;
}

aid_t State::next_transition() const
{
  XBT_DEBUG("Search for an actor to run. %zu actors to consider", actors_to_run_.size());
  for (auto const& [aid, actor] : actors_to_run_) {
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

std::pair<aid_t, int> State::next_transition_guided() const
{
  return Exploration::get_strategy()->next_transition_in(this);
}

// This should be done in GuidedState, or at least interact with it
std::shared_ptr<Transition> State::execute_next(aid_t next, RemoteApp& app)
{
  // This actor is ready to be executed. Execution involves three phases:

  // 1. Identify the appropriate ActorState to prepare for execution
  // when simcall_handle will be called on it
  auto& actor_state                              = actors_to_run_.at(next);
  const unsigned times_considered                = actor_state.do_consider();
  const Transition* expected_executed_transition = nullptr;
  if (_sg_mc_debug) {
    expected_executed_transition = actor_state.get_transition(times_considered).get();
    xbt_assert(actor_state.is_enabled(), "Tried to execute a disabled actor");
    xbt_assert(expected_executed_transition != nullptr,
               "Expected a transition with %u times considered to be noted in actor %ld", times_considered, next);
  }
  XBT_DEBUG("Let's run actor %ld (times_considered = %u)", next, times_considered);

  // 2. Execute the actor according to the preparation above
  Transition::executed_transitions_++;
  auto* just_executed = app.handle_simcall(next, times_considered, true);
  if (_sg_mc_debug) {
    xbt_assert(
        just_executed->type_ == expected_executed_transition->type_,
        "The transition that was just executed by actor %ld, viz:\n"
        "%s\n"
        "is not what was purportedly scheduled to execute, which was:\n"
        "%s\n"
        "If adding the --cfg=model-check/cached-states-interval:1 parameter solves this problem, then your application "
        "is not purely data dependent (its outcome does not only depends on its input). McSimGrid may miss bugs in "
        "such "
        "applications, as they cannot be exhaustively explored with Mc SimGrid.\n\n"
        "If adding this parameter does not help, then it's probably a bug in Mc SimGrid itself. Please report it.\n",
        next, just_executed->to_string().c_str(), expected_executed_transition->to_string().c_str());
  }
  // 3. Update the state with the newest information. This means recording
  // both
  //  1. what action was last taken from this state (viz. `executed_transition`)
  //  2. what action actor `next` was able to take given `times_considered`
  // The latter update is important as *more* information is potentially available
  // about a transition AFTER it has executed.
  outgoing_transition_ = std::shared_ptr<Transition>(just_executed);

  if (Exploration::need_actor_status_transitions())
    actor_state.set_transition(outgoing_transition_, times_considered);
  app.wait_for_requests();

  return outgoing_transition_;
}

std::unordered_set<aid_t> State::get_backtrack_set() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, state] : get_actors_list()) {
    if (state.is_todo() || state.is_done()) {
      actors.insert(aid);
    }
  }
  return actors;
}

std::unordered_set<aid_t> State::get_enabled_actors() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, state] : get_actors_list()) {
    if (state.is_enabled()) {
      actors.insert(aid);
    }
  }
  return actors;
}

std::vector<aid_t> State::get_batrack_minus_done() const
{
  std::vector<aid_t> actors;
  for (const auto& [aid, state] : get_actors_list()) {
    if (state.is_todo()) {
      actors.insert(actors.begin(), aid);
    }
  }
  return actors;
}

void State::register_as_correct()
{
  if (not _sg_mc_search_critical_transition)
    return;
  has_correct_descendent_ = true;
  if (parent_state_ != nullptr)
    parent_state_->register_as_correct();
}

void State::record_child_state(StatePtr child)
{
  aid_t child_aid      = child->get_transition_in()->aid_;
  int times_considered = child->get_transition_in()->times_considered_;
  if (children_states_.size() < static_cast<long unsigned>(child_aid + 1))
    children_states_.resize(child_aid + 1);
  if (children_states_[child_aid].size() < static_cast<long unsigned>(times_considered + 1))
    children_states_[child_aid].resize(times_considered + 1);
  children_states_[child_aid][times_considered] = std::move(child);
}

void State::signal_on_backtrack()
{
  XBT_VERB("State n°%ld, child of %ld by actor %ld, being signaled to backtrack", get_num(),
           get_parent_state() != nullptr ? get_parent_state()->get_num() : -1, get_transition_in()->aid_);
  XBT_VERB("... %s",
           is_leftmost_ ? "This state is the leftmost at this depth" : "This state is NOT the leftmost at this depth");

  if (not is_leftmost_)
    return;

  if (closed_.size() < opened_.size()) {
    // if there are children states that are being visited, we may need to update the leftmost information
    auto const& leftmost_transition = opened_[closed_.size()];
    auto children_aid = children_states_[leftmost_transition->aid_][leftmost_transition->times_considered_];
    if (children_aid == nullptr)
      reference_holder_.tell("Assertion (children_aid == nullptr) failed!");
    xbt_assert(children_aid != nullptr, "Leftmost aid: %ld; state_num: %ld", leftmost_transition->aid_, get_num());
    children_aid->is_leftmost_ = true;
    children_aid->signal_on_backtrack();
    return;
  }

  XBT_VERB("... there's still at least one actor to execute (%ld)", next_transition());
  if (next_transition() == -1) {
    // This is the leftmost state, it doesn't have anymore open children and nothing left to do
    // Let's close this by marking it
    if (parent_state_ != nullptr) {
      XBT_VERB("\t... there are %lu recorded children StatePtr in its parent state n°%ld",
               parent_state_->opened_.size(), parent_state_->get_num());
      parent_state_->children_states_[get_transition_in()->aid_][get_transition_in()->times_considered_] = nullptr;

      auto findme = std::find(parent_state_->closed_.begin(), parent_state_->closed_.end(), get_transition_in()->aid_);
      xbt_assert(findme == parent_state_->closed_.end(), "I'm already in the closed_ of my parent");
      parent_state_->closed_.emplace_back(get_transition_in()->aid_);

      XBT_VERB("\t... The count of remaining intrusive_ptr on this node #%ld is %d", get_num(), get_ref_count());
      parent_state_->signal_on_backtrack();
    }
    reset_parent_state();
  }
}

void State::reset_parent_state()
{
  XBT_VERB("Cleaning the parent state of state #%ld", get_num());
  parent_state_ = nullptr;
}

// boost::intrusive_ptr<State> support:
void intrusive_ptr_add_ref(State* state)
{
  XBT_DEBUG("Adding a ref to state #%ld", state->get_num());
  state->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(State* state)
{
  XBT_DEBUG("Removing a ref to state #%ld", state->get_num());
  if (state->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete state;
  }
}

void State::initialize(const RemoteApp& remote_app)
{
  XBT_VERB("Initializing state #%ld", get_num());
  xbt_assert(not actor_status_set_);
  actor_status_set_ = true;
  remote_app.get_actors_status(actors_to_run_); // We tell the remote app to get the transitions this time

  for (auto const& t : opened_) {
    xbt_assert(t != nullptr);
    actors_to_run_.at(t->aid_).mark_todo();
  }
}

void State::update_incoming_transition_with_remote_app(const RemoteApp& remote_app, aid_t aid, int times_considered)
{

  incoming_transition_ = std::shared_ptr<Transition>(remote_app.handle_simcall(aid, times_considered, true));
}

void State::update_opened(std::shared_ptr<Transition> transition)
{
  xbt_assert(transition != nullptr);
  for (size_t i = 0; i < opened_.size(); i++) {
    if (opened_[i]->aid_ != transition->aid_)
      continue;
    if (opened_[i]->times_considered_ != transition->times_considered_)
      continue;

    opened_[i] = transition;
    return;
  }

  opened_.push_back(transition);
}
} // namespace simgrid::mc
