/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Model.hpp"
#include "src/kernel/lmm/maxmin.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(resource);

namespace simgrid {
namespace kernel {
namespace resource {

Model::Model() : maxminSystem_(nullptr)
{
  readyActionSet_   = new ActionList();
  runningActionSet_ = new ActionList();
  failedActionSet_  = new ActionList();
  doneActionSet_    = new ActionList();

  modifiedSet_     = nullptr;
  updateMechanism_ = UM_UNDEFINED;
  selectiveUpdate_ = 0;
}

Model::~Model()
{
  delete readyActionSet_;
  delete runningActionSet_;
  delete failedActionSet_;
  delete doneActionSet_;
  delete modifiedSet_;
  delete maxminSystem_;
}

Action* Model::actionHeapPop()
{
  Action* action = actionHeap_.top().second;
  actionHeap_.pop();
  action->clearHeapHandle();
  return action;
}

double Model::nextOccuringEvent(double now)
{
  // FIXME: set the good function once and for all
  if (updateMechanism_ == UM_LAZY)
    return nextOccuringEventLazy(now);
  else if (updateMechanism_ == UM_FULL)
    return nextOccuringEventFull(now);
  else
    xbt_die("Invalid cpu update mechanism!");
}

double Model::nextOccuringEventLazy(double now)
{
  XBT_DEBUG("Before share resources, the size of modified actions set is %zu", modifiedSet_->size());
  lmm_solve(maxminSystem_);
  XBT_DEBUG("After share resources, The size of modified actions set is %zu", modifiedSet_->size());

  while (not modifiedSet_->empty()) {
    Action* action = &(modifiedSet_->front());
    modifiedSet_->pop_front();
    bool max_dur_flag = false;

    if (action->getStateSet() != runningActionSet_)
      continue;

    /* bogus priority, skip it */
    if (action->getPriority() <= 0 || action->getType() == Action::Type::LATENCY)
      continue;

    action->updateRemainingLazy(now);

    double min   = -1;
    double share = action->getVariable()->get_value();

    if (share > 0) {
      double time_to_completion;
      if (action->getRemains() > 0) {
        time_to_completion = action->getRemainsNoUpdate() / share;
      } else {
        time_to_completion = 0.0;
      }
      min = now + time_to_completion; // when the task will complete if nothing changes
    }

    if ((action->getMaxDuration() > NO_MAX_DURATION) &&
        (min <= -1 || action->getStartTime() + action->getMaxDuration() < min)) {
      // when the task will complete anyway because of the deadline if any
      min          = action->getStartTime() + action->getMaxDuration();
      max_dur_flag = true;
    }

    XBT_DEBUG("Action(%p) corresponds to variable %d", action, action->getVariable()->id_int);

    XBT_DEBUG("Action(%p) Start %f. May finish at %f (got a share of %f). Max_duration %f", action,
              action->getStartTime(), min, share, action->getMaxDuration());

    if (min > -1) {
      action->heapUpdate(actionHeap_, min, max_dur_flag ? Action::Type::MAX_DURATION : Action::Type::NORMAL);
      XBT_DEBUG("Insert at heap action(%p) min %f now %f", action, min, now);
    } else
      DIE_IMPOSSIBLE;
  }

  // hereafter must have already the min value for this resource model
  if (not actionHeapIsEmpty()) {
    double min = actionHeapTopDate() - now;
    XBT_DEBUG("minimum with the HEAP %f", min);
    return min;
  } else {
    XBT_DEBUG("The HEAP is empty, thus returning -1");
    return -1;
  }
}

double Model::nextOccuringEventFull(double /*now*/)
{
  maxminSystem_->solve_fun(maxminSystem_);

  double min = -1;

  for (Action& action : *getRunningActionSet()) {
    double value = action.getVariable()->get_value();
    if (value > 0) {
      if (action.getRemains() > 0)
        value = action.getRemainsNoUpdate() / value;
      else
        value = 0.0;
      if (min < 0 || value < min) {
        min = value;
        XBT_DEBUG("Updating min (value) with %p: %f", &action, min);
      }
    }
    if ((action.getMaxDuration() >= 0) && (min < 0 || action.getMaxDuration() < min)) {
      min = action.getMaxDuration();
      XBT_DEBUG("Updating min (duration) with %p: %f", &action, min);
    }
  }
  XBT_DEBUG("min value : %f", min);

  return min;
}

void Model::updateActionsState(double now, double delta)
{
  if (updateMechanism_ == UM_FULL)
    updateActionsStateFull(now, delta);
  else if (updateMechanism_ == UM_LAZY)
    updateActionsStateLazy(now, delta);
  else
    xbt_die("Invalid cpu update mechanism!");
}

void Model::updateActionsStateLazy(double /*now*/, double /*delta*/)
{
  THROW_UNIMPLEMENTED;
}

void Model::updateActionsStateFull(double /*now*/, double /*delta*/)
{
  THROW_UNIMPLEMENTED;
}

} // namespace surf
} // namespace simgrid
} // namespace simgrid
