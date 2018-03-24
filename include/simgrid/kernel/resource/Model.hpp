/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_MODEL_HPP
#define SIMGRID_KERNEL_RESOURCE_MODEL_HPP

#include <simgrid/kernel/resource/Action.hpp>

extern "C" {

/** @brief Possible update mechanisms */
enum e_UM_t {
  UM_FULL,     /**< Full update mechanism: the remaining time of every action is recomputed at each step */
  UM_LAZY,     /**< Lazy update mechanism: only the modified actions get recomputed.
                    It may be slower than full if your system is tightly coupled to the point where every action
                    gets recomputed anyway. In that case, you'd better not try to be cleaver with lazy and go for
                    a simple full update.  */
  UM_UNDEFINED /**< Mechanism not defined */
};
}

namespace simgrid {
namespace kernel {
namespace resource {

/** @ingroup SURF_interface
 * @brief SURF model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class XBT_PUBLIC Model {
public:
  Model();
  virtual ~Model();

  /** @brief Get the set of [actions](@ref Action) in *ready* state */
  ActionList* getReadyActionSet() const { return ready_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *running* state */
  ActionList* getRunningActionSet() const { return running_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *failed* state */
  ActionList* getFailedActionSet() const { return failed_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *done* state */
  ActionList* getDoneActionSet() const { return done_action_set_; }

  /** @brief Get the set of modified [actions](@ref Action) */
  ActionLmmListPtr getModifiedSet() const;

  /** @brief Get the maxmin system of the current Model */
  lmm::System* getMaxminSystem() const { return maxmin_system_; }

  /**
   * @brief Get the update mechanism of the current Model
   * @see e_UM_t
   */
  e_UM_t getUpdateMechanism() const { return update_mechanism_; }
  void setUpdateMechanism(e_UM_t mechanism) { update_mechanism_ = mechanism; }

  /** @brief Get Action heap */
  heap_type& getActionHeap() { return action_heap_; }

  double actionHeapTopDate() const { return action_heap_.top().first; }
  Action* actionHeapPop();
  bool actionHeapIsEmpty() const { return action_heap_.empty(); }

  /**
   * @brief Share the resources between the actions
   *
   * @param now The current time of the simulation
   * @return The delta of time till the next action will finish
   */
  virtual double nextOccuringEvent(double now);
  virtual double nextOccuringEventLazy(double now);
  virtual double nextOccuringEventFull(double now);

  /**
   * @brief Update action to the current time
   *
   * @param now The current time of the simulation
   * @param delta The delta of time since the last update
   */
  virtual void updateActionsState(double now, double delta);
  virtual void updateActionsStateLazy(double now, double delta);
  virtual void updateActionsStateFull(double now, double delta);

  /** @brief Returns whether this model have an idempotent shareResource()
   *
   * The only model that is not is NS3: computing the next timestamp moves the model up to that point,
   * so we need to call it only when the next timestamp of other sources is computed.
   */
  virtual bool nextOccuringEventIsIdempotent() { return true; }

protected:
  lmm::System* maxmin_system_ = nullptr;

private:
  e_UM_t update_mechanism_        = UM_UNDEFINED;
  ActionList* ready_action_set_   = new ActionList(); /**< Actions in state SURF_ACTION_READY */
  ActionList* running_action_set_ = new ActionList(); /**< Actions in state SURF_ACTION_RUNNING */
  ActionList* failed_action_set_  = new ActionList(); /**< Actions in state SURF_ACTION_FAILED */
  ActionList* done_action_set_    = new ActionList(); /**< Actions in state SURF_ACTION_DONE */
  heap_type action_heap_;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif
