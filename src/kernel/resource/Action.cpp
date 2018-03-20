/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_CATEGORY(kernel, "Logging specific to the internals of SimGrid");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(resource, kernel, "Logging specific to the resources");

namespace simgrid {
namespace kernel {
namespace resource {

Action::Action(simgrid::kernel::resource::Model* model, double cost, bool failed) : Action(model, cost, failed, nullptr)
{
}

Action::Action(simgrid::kernel::resource::Model* model, double cost, bool failed, kernel::lmm::Variable* var)
    : remains_(cost), start_(surf_get_clock()), cost_(cost), model_(model), variable_(var)
{
  if (failed)
    stateSet_ = getModel()->getFailedActionSet();
  else
    stateSet_ = getModel()->getRunningActionSet();

  stateSet_->push_back(*this);
}

Action::~Action()
{
  xbt_free(category_);
}

void Action::finish(Action::State state)
{
  finishTime_ = surf_get_clock();
  setState(state);
}

Action::State Action::getState() const
{
  if (stateSet_ == model_->getReadyActionSet())
    return Action::State::ready;
  if (stateSet_ == model_->getRunningActionSet())
    return Action::State::running;
  if (stateSet_ == model_->getFailedActionSet())
    return Action::State::failed;
  if (stateSet_ == model_->getDoneActionSet())
    return Action::State::done;
  return Action::State::not_in_the_system;
}

void Action::setState(Action::State state)
{
  simgrid::xbt::intrusive_erase(*stateSet_, *this);
  switch (state) {
    case Action::State::ready:
      stateSet_ = model_->getReadyActionSet();
      break;
    case Action::State::running:
      stateSet_ = model_->getRunningActionSet();
      break;
    case Action::State::failed:
      stateSet_ = model_->getFailedActionSet();
      break;
    case Action::State::done:
      stateSet_ = model_->getDoneActionSet();
      break;
    default:
      stateSet_ = nullptr;
      break;
  }
  if (stateSet_)
    stateSet_->push_back(*this);
}

double Action::getBound() const
{
  return variable_ ? variable_->get_bound() : 0;
}

void Action::setBound(double bound)
{
  XBT_IN("(%p,%g)", this, bound);
  if (variable_)
    getModel()->getMaxminSystem()->update_variable_bound(variable_, bound);

  if (getModel()->getUpdateMechanism() == UM_LAZY && getLastUpdate() != surf_get_clock())
    heapRemove(getModel()->getActionHeap());
  XBT_OUT();
}

void Action::setCategory(const char* category)
{
  category_ = xbt_strdup(category);
}

void Action::ref()
{
  refcount_++;
}

void Action::setMaxDuration(double duration)
{
  maxDuration_ = duration;
  if (getModel()->getUpdateMechanism() == UM_LAZY) // remove action from the heap
    heapRemove(getModel()->getActionHeap());
}

void Action::setSharingWeight(double weight)
{
  XBT_IN("(%p,%g)", this, weight);
  sharingWeight_ = weight;
  getModel()->getMaxminSystem()->update_variable_weight(getVariable(), weight);

  if (getModel()->getUpdateMechanism() == UM_LAZY)
    heapRemove(getModel()->getActionHeap());
  XBT_OUT();
}

void Action::cancel()
{
  setState(Action::State::failed);
  if (getModel()->getUpdateMechanism() == UM_LAZY) {
    if (modifiedSetHook_.is_linked())
      simgrid::xbt::intrusive_erase(*getModel()->getModifiedSet(), *this);
    heapRemove(getModel()->getActionHeap());
  }
}

int Action::unref()
{
  refcount_--;
  if (not refcount_) {
    if (stateSetHook_.is_linked())
      simgrid::xbt::intrusive_erase(*stateSet_, *this);
    if (getVariable())
      getModel()->getMaxminSystem()->variable_free(getVariable());
    if (getModel()->getUpdateMechanism() == UM_LAZY) {
      /* remove from heap */
      heapRemove(getModel()->getActionHeap());
      if (modifiedSetHook_.is_linked())
        simgrid::xbt::intrusive_erase(*getModel()->getModifiedSet(), *this);
    }
    delete this;
    return 1;
  }
  return 0;
}

void Action::suspend()
{
  XBT_IN("(%p)", this);
  if (suspended_ != SuspendStates::sleeping) {
    getModel()->getMaxminSystem()->update_variable_weight(getVariable(), 0.0);
    if (getModel()->getUpdateMechanism() == UM_LAZY) {
      heapRemove(getModel()->getActionHeap());
      if (getModel()->getUpdateMechanism() == UM_LAZY && stateSet_ == getModel()->getRunningActionSet() &&
          sharingWeight_ > 0) {
        // If we have a lazy model, we need to update the remaining value accordingly
        updateRemainingLazy(surf_get_clock());
      }
    }
    suspended_ = SuspendStates::suspended;
  }
  XBT_OUT();
}

void Action::resume()
{
  XBT_IN("(%p)", this);
  if (suspended_ != SuspendStates::sleeping) {
    getModel()->getMaxminSystem()->update_variable_weight(getVariable(), getPriority());
    suspended_ = SuspendStates::not_suspended;
    if (getModel()->getUpdateMechanism() == UM_LAZY)
      heapRemove(getModel()->getActionHeap());
  }
  XBT_OUT();
}

bool Action::isSuspended()
{
  return suspended_ == SuspendStates::suspended;
}
/* insert action on heap using a given key and a hat (heap_action_type)
 * a hat can be of three types for communications:
 *
 * NORMAL = this is a normal heap entry stating the date to finish transmitting
 * LATENCY = this is a heap entry to warn us when the latency is payed
 * MAX_DURATION =this is a heap entry to warn us when the max_duration limit is reached
 */
void Action::heapInsert(heap_type& heap, double key, Action::Type hat)
{
  type_       = hat;
  heapHandle_ = heap.emplace(std::make_pair(key, this));
}

void Action::heapRemove(heap_type& heap)
{
  type_ = Action::Type::NOTSET;
  if (heapHandle_) {
    heap.erase(*heapHandle_);
    clearHeapHandle();
  }
}

void Action::heapUpdate(heap_type& heap, double key, Action::Type hat)
{
  type_ = hat;
  if (heapHandle_) {
    heap.update(*heapHandle_, std::make_pair(key, this));
  } else {
    heapHandle_ = heap.emplace(std::make_pair(key, this));
  }
}

double Action::getRemains()
{
  XBT_IN("(%p)", this);
  /* update remains before return it */
  if (getModel()->getUpdateMechanism() == UM_LAZY) /* update remains before return it */
    updateRemainingLazy(surf_get_clock());
  XBT_OUT();
  return remains_;
}

void Action::updateMaxDuration(double delta)
{
  double_update(&maxDuration_, delta, sg_surf_precision);
}
void Action::updateRemains(double delta)
{
  double_update(&remains_, delta, sg_maxmin_precision * sg_surf_precision);
}

void Action::refreshLastUpdate()
{
  lastUpdate_ = surf_get_clock();
}

} // namespace surf
} // namespace simgrid
} // namespace simgrid
