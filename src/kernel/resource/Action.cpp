/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_CATEGORY(kernel, "SimGrid internals");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_resource, kernel, "Resources, modeling the platform performance");

namespace simgrid {
namespace kernel {
namespace resource {

Action::Action(Model* model, double cost, bool failed) : Action(model, cost, failed, nullptr) {}

Action::Action(Model* model, double cost, bool failed, lmm::Variable* var)
    : remains_(cost), start_time_(EngineImpl::get_clock()), cost_(cost), model_(model), variable_(var)
{
  if (failed)
    state_set_ = model_->get_failed_action_set();
  else
    state_set_ = model_->get_started_action_set();

  state_set_->push_back(*this);
}

Action::~Action()
{
  if (state_set_hook_.is_linked())
    xbt::intrusive_erase(*state_set_, *this);
  if (get_variable())
    model_->get_maxmin_system()->variable_free(get_variable());

  /* remove from heap on need (ie, if selective update) */
  model_->get_action_heap().remove(this);
  if (modified_set_hook_.is_linked())
    xbt::intrusive_erase(*model_->get_modified_set(), *this);
}

void Action::finish(Action::State state)
{
  finish_time_ = EngineImpl::get_clock();
  set_remains(0);
  set_state(state);
}

Action::State Action::get_state() const
{
  if (state_set_ == model_->get_inited_action_set())
    return Action::State::INITED;
  if (state_set_ == model_->get_started_action_set())
    return Action::State::STARTED;
  if (state_set_ == model_->get_failed_action_set())
    return Action::State::FAILED;
  if (state_set_ == model_->get_finished_action_set())
    return Action::State::FINISHED;
  if (state_set_ == model_->get_ignored_action_set())
    return Action::State::IGNORED;
  THROW_IMPOSSIBLE;
}

void Action::set_state(Action::State state)
{
  xbt::intrusive_erase(*state_set_, *this);
  switch (state) {
    case Action::State::INITED:
      state_set_ = model_->get_inited_action_set();
      break;
    case Action::State::STARTED:
      state_set_ = model_->get_started_action_set();
      break;
    case Action::State::FAILED:
      state_set_ = model_->get_failed_action_set();
      break;
    case Action::State::FINISHED:
      state_set_ = model_->get_finished_action_set();
      break;
    case Action::State::IGNORED:
      state_set_ = model_->get_ignored_action_set();
      break;
    default:
      state_set_ = nullptr;
      break;
  }
  if (state_set_)
    state_set_->push_back(*this);
}

double Action::get_bound() const
{
  return variable_ ? variable_->get_bound() : 0;
}

double Action::get_rate() const
{
  return variable_ ? variable_->get_value() * factor_ : 0;
}

void Action::set_bound(double bound)
{
  XBT_IN("(%p,%g)", this, bound);
  if (variable_)
    model_->get_maxmin_system()->update_variable_bound(variable_, bound);

  if (model_->is_update_lazy() && get_last_update() != EngineImpl::get_clock())
    model_->get_action_heap().remove(this);
  XBT_OUT();
}

void Action::ref()
{
  refcount_++;
}

void Action::set_max_duration(double duration)
{
  max_duration_ = duration;
  if (model_->is_update_lazy()) // remove action from the heap
    model_->get_action_heap().remove(this);
}

void Action::set_sharing_penalty(double sharing_penalty)
{
  XBT_IN("(%p,%g)", this, sharing_penalty);
  sharing_penalty_ = sharing_penalty;
  model_->get_maxmin_system()->update_variable_penalty(get_variable(), sharing_penalty);
  if (model_->is_update_lazy())
    model_->get_action_heap().remove(this);
  XBT_OUT();
}

void Action::cancel()
{
  set_state(Action::State::FAILED);
  if (model_->is_update_lazy()) {
    if (modified_set_hook_.is_linked())
      xbt::intrusive_erase(*model_->get_modified_set(), *this);
    model_->get_action_heap().remove(this);
  }
}

bool Action::unref()
{
  refcount_--;
  if (not refcount_) {
    delete this;
    return true;
  }
  return false;
}

void Action::suspend()
{
  XBT_IN("(%p)", this);
  if (suspended_ != SuspendStates::SLEEPING) {
    model_->get_maxmin_system()->update_variable_penalty(get_variable(), 0.0);
    if (model_->is_update_lazy()) {
      model_->get_action_heap().remove(this);
      if (state_set_ == model_->get_started_action_set() && sharing_penalty_ > 0) {
        // If we have a lazy model, we need to update the remaining value accordingly
        update_remains_lazy(EngineImpl::get_clock());
      }
    }
    suspended_ = SuspendStates::SUSPENDED;
  }
  XBT_OUT();
}

void Action::resume()
{
  XBT_IN("(%p)", this);
  if (suspended_ != SuspendStates::SLEEPING) {
    model_->get_maxmin_system()->update_variable_penalty(get_variable(), get_sharing_penalty());
    suspended_ = SuspendStates::RUNNING;
    if (model_->is_update_lazy())
      model_->get_action_heap().remove(this);
  }
  XBT_OUT();
}

double Action::get_remains()
{
  XBT_IN("(%p)", this);
  /* update remains before returning it */
  if (model_->is_update_lazy()) /* update remains before return it */
    update_remains_lazy(EngineImpl::get_clock());
  XBT_OUT();
  return remains_;
}

void Action::update_max_duration(double delta)
{
  if (max_duration_ != NO_MAX_DURATION)
    double_update(&max_duration_, delta, sg_surf_precision);
}

void Action::update_remains(double delta)
{
  double_update(&remains_, delta, sg_maxmin_precision * sg_surf_precision);
}

void Action::set_last_update()
{
  last_update_ = EngineImpl::get_clock();
}

double ActionHeap::top_date() const
{
  return top().first;
}

void ActionHeap::insert(Action* action, double date, ActionHeap::Type type)
{
  action->type_      = type;
  action->heap_hook_ = emplace(std::make_pair(date, action));
}

void ActionHeap::remove(Action* action)
{
  action->type_ = ActionHeap::Type::unset;
  if (action->heap_hook_) {
    erase(*action->heap_hook_);
    action->heap_hook_ = boost::none;
  }
}

void ActionHeap::update(Action* action, double date, ActionHeap::Type type)
{
  action->type_ = type;
  if (action->heap_hook_) {
    heap_type::update(*action->heap_hook_, std::make_pair(date, action));
  } else {
    action->heap_hook_ = emplace(std::make_pair(date, action));
  }
}

Action* ActionHeap::pop()
{
  Action* action = top().second;
  heap_type::pop();
  action->heap_hook_ = boost::none;
  return action;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
