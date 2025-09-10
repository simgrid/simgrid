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
#include "xbt/thread.hpp"

#include <algorithm>
#include <atomic>
#include <boost/current_function.hpp>
#include <boost/range/algorithm.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

namespace simgrid::mc {

long State::expended_states_  = 0;
std::atomic_ulong State::in_memory_states_ = 0;
size_t State::max_actor_encountered_ = 1;

State::~State()
{
  in_memory_states_--;
  if (_sg_mc_output_lts)
    XBT_CRITICAL("Closing state n°%ld! There are %ld remaining states", this->get_num(), get_in_memory_states());
}

State::State(const RemoteApp& remote_app, bool set_actor_status) : num_(++expended_states_)
{
  in_memory_states_++;
  if (set_actor_status) {
    remote_app.get_actors_status(actors_to_run_);
    actor_status_set_      = true;
    max_actor_encountered_ = std::max(max_actor_encountered_, actors_to_run_.size());
  } else {
    // If the actor_status is not to be set yet, then at least, allocate a overestimated vector
    actors_to_run_.resize(max_actor_encountered_);
  }

  opened_.reserve(max_actor_encountered_);

  // UDPOR create state that have no parentship links at all and manage everything
  // its own way so let it cook
  if (get_num() == 1 and get_model_checking_reduction() != ReductionMode::udpor) {
    traversal_ = std::make_shared<PostFixTraversal>(this);

    is_leftmost_ = true; // The first state is the only one at that depth, so the leftmost one.
  }

  being_explored.test_and_set();
}

State::State(const RemoteApp& remote_app, StatePtr parent_state, std::shared_ptr<Transition> incoming_transition,
             bool set_actor_status)
    : State(remote_app, set_actor_status)
{
  xbt_assert(incoming_transition != nullptr);
  parent_state_        = std::move(parent_state);
  incoming_transition_ = std::move(incoming_transition);
  depth_               = parent_state_->depth_ + 1;

  parent_state_->update_opened(get_transition_in());
  // This is leftmost iff parent is leftmost, and this is the leftest opened child of the parent
  is_leftmost_ =
      parent_state_->is_leftmost_ and parent_state_->opened_[parent_state_->closed_.size()] == get_transition_in();
  parent_state_->record_child_state(this);

  traversal_ = std::make_shared<PostFixTraversal>(this);

  if (_sg_mc_output_lts)
    XBT_CRITICAL("State %ld ==> Actor %ld: %.60s ==> State %ld", parent_state_->num_, incoming_transition_->aid_,
                 incoming_transition_->to_string().c_str(), num_);
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(actors_to_run_, [](auto& opt) { return opt.has_value() and opt->is_todo(); });
}

bool State::has_more_to_be_explored() const
{
  size_t count = 0;
  for (auto const& actor : actors_to_run_)
    if (actor.has_value() and actor.value().is_todo())
      count += actor.value().get_times_not_considered();

  return count > 0;
}

aid_t State::next_transition() const
{
  XBT_DEBUG("Search for an actor to run. %zu actors to consider", actors_to_run_.size());
  for (auto const& actor : actors_to_run_) {
    if (not actor.has_value())
      continue;
    aid_t aid = actor.value().get_aid();
    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
    if (not actor.value().is_todo() || not actor.value().is_enabled() || actor.value().is_done()) {
      if (not actor.value().is_todo())
        XBT_DEBUG("Can't run actor %ld because it is not todo", aid);

      if (not actor.value().is_enabled())
        XBT_DEBUG("Can't run actor %ld because it is not enabled", aid);

      if (actor.value().is_done())
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
  const auto actor_state                         = get_actor_at(next);
  const unsigned times_considered                = actors_to_run_[next]->do_consider();
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
    actors_to_run_[next]->set_transition(outgoing_transition_, times_considered);
  app.wait_for_requests();

  return outgoing_transition_;
}

std::unordered_set<aid_t> State::get_backtrack_set() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& state : get_actors_list()) {
    if (not state.has_value())
      continue;
    if (state.value().is_todo() || state.value().is_done()) {
      actors.insert(state.value().get_aid());
    }
  }
  return actors;
}

std::unordered_set<aid_t> State::get_enabled_actors() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& state : get_actors_list()) {
    if (not state.has_value())
      continue;
    if (state.value().is_enabled()) {
      actors.insert(state.value().get_aid());
    }
  }
  return actors;
}

std::vector<aid_t> State::get_batrack_minus_done() const
{
  std::vector<aid_t> actors;
  for (const auto& state : get_actors_list()) {
    if (not state.has_value())
      continue;
    if (state.value().is_todo()) {
      actors.insert(actors.begin(), state.value().get_aid());
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
  is_a_leaf                                     = false;
}

void State::signal_on_backtrack()
{
  to_be_deleted_ = true;
  State::garbage_collect();
  return;
}

void State::reset_parent_state()
{
  if (_sg_mc_output_lts)
    XBT_CRITICAL("Cleaning the parent state of state #%ld", get_num());
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
  XBT_DEBUG("[tid : %s] Removing a ref to state #%ld, %d ref remaining", xbt::gettid().c_str(), state->get_num(),
            static_cast<int>(state->refcount_.load()));
  if (state->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete state;
  }
}

void State::initialize(const RemoteApp& remote_app)
{
  if (_sg_mc_explore_algo != "parallel")
    xbt_assert(not actor_status_set_, "State #%ld is already initialized!", get_num());
  remote_app.get_actors_status(actors_to_run_); // We tell the remote app to get the transitions this time
  actor_status_set_ = true;

  for (auto const& t : opened_) {
    xbt_assert(t != nullptr);
    actors_to_run_.at(t->aid_)->mark_todo();
  }

  // Tell the parent we are being done (and are not "todo" anymore)
  parent_state_->actors_to_run_[incoming_transition_->aid_]->mark_done();
}

void State::update_incoming_transition_with_remote_app(const RemoteApp& remote_app, aid_t aid, int times_considered)
{
  incoming_transition_ = std::shared_ptr<Transition>(remote_app.handle_simcall(aid, times_considered, true));
  if (_sg_mc_output_lts)
    XBT_CRITICAL("State %ld ==> Actor %ld: %.60s ==> State %ld", parent_state_->num_, incoming_transition_->aid_,
                 incoming_transition_->to_string().c_str(), num_);
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
void State::consider_one(aid_t aid)
{
  auto actor = get_actor_at(aid);
  xbt_assert(actor.is_enabled(), "Tried to mark as TODO actor %ld in state #%ld but it is not enabled", aid, get_num());
  xbt_assert(not actor.is_done(), "Tried to mark as TODO actor %ld in state #%ld but it is already done", aid,
             get_num());
  actors_to_run_[aid]->mark_todo();
  opened_.emplace_back(std::make_shared<Transition>(Transition::Type::UNKNOWN, aid, actor.get_times_considered()));
  XBT_DEBUG("Considered actor %hd at state %ld", actor.get_aid(), get_num());
}
State::PostFixTraversal::PostFixTraversal(StatePtr state)
{
  prev_ = nullptr;
  next_ = nullptr;
  self_ = state;

  if (state->parent_state_ == nullptr)
    return;

  // Insert the new node just at the left of his parent
  PostFixTraversal* parent_traversal = state->parent_state_->traversal_.get();
  xbt_assert(parent_traversal != nullptr, "Why does this state parent have no traversal?");

  next_ = parent_traversal;

  // Wait until we can lock the node before where we are inserting, if any
  // This is mandatory when we try to insert at the beginning of the list
  while (parent_traversal->prev_ != nullptr and parent_traversal->prev_->lock_.try_lock()) {
  }

  if (parent_traversal->prev_ != nullptr)
    parent_traversal->prev_->next_ = this;

  prev_                   = parent_traversal->prev_;
  parent_traversal->prev_ = this;

  if (first_ == nullptr or first_ == parent_traversal)
    first_ = this;

  if (prev_ != nullptr)
    prev_->lock_.unlock();
}

void State::remove_ref_in_parent()
{

  parent_state_->children_states_[get_transition_in()->aid_][get_transition_in()->times_considered_] = nullptr;

  auto findme = std::find(parent_state_->closed_.begin(), parent_state_->closed_.end(), get_transition_in()->aid_);
  xbt_assert(findme == parent_state_->closed_.end(), "I'm already in the closed_ of my parent");
}

State::PostFixTraversal* simgrid::mc::State::PostFixTraversal::first_ = nullptr;

void State::garbage_collect()
{
  StatePtr first_state = PostFixTraversal::get_first();
  while (first_state != nullptr and first_state->to_be_deleted_) {
    PostFixTraversal::remove_first();

    if (first_state->parent_state_ != nullptr) {
      first_state->parent_state_->to_be_deleted_ = true;
      first_state->remove_ref_in_parent();
      first_state->reset_parent_state();
    }

    first_state = PostFixTraversal::get_first();
  }
}
void State::PostFixTraversal::remove_first()
{
  // Waiting until the first is accessible
  // and lock it so no one modify what's coming next
  while (not first_->lock_.try_lock()) {
  }

  if (first_->next_ != nullptr)
    // try to remove the second entry predecessor
    first_->next_->prev_ = nullptr;

  first_->self_ = nullptr;
  first_        = first_->next_;
}
StatePtr State::PostFixTraversal::get_first()
{
  if (first_ != nullptr)
    return first_->self_;
  else
    return nullptr;
}
std::string State::PostFixTraversal::get_traversal_as_ids()
{
  std::string res = "";

  auto curr = first_;
  while (curr != nullptr) {
    res += std::to_string(curr->self_->num_) + ';';
    curr = curr->next_;
  }

  return res;
};
unsigned long State::get_actor_count() const
{
  return std::count_if(actors_to_run_.begin(), actors_to_run_.end(), [](auto actor) { return actor.has_value(); });
}
unsigned long State::consider_all()
{
  unsigned long count = 0;
  for (auto& actor : actors_to_run_) {
    if (not actor.has_value())
      continue;
    if (actor.value().is_enabled() && not actor.value().is_done()) {
      actor.value().mark_todo();
      count++;
      opened_.emplace_back(std::make_shared<Transition>(Transition::Type::UNKNOWN, actor.value().get_aid(),
                                                        actor.value().get_times_considered()));
      XBT_DEBUG("Marked actor %hd at state %ld", actor->get_aid(), get_num());
    }
  }
  return count;
}
void State::PostFixTraversal::update_leftness()
{
  PostFixTraversal* current_state = PostFixTraversal::get_first()->traversal_.get();
  unsigned long count             = 0;
  while (current_state != nullptr) {

    current_state->self_->leftness_ = count;

    current_state = current_state->next_;
    count++;
  }
}
} // namespace simgrid::mc
