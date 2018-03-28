/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_MODEL_HPP
#define SIMGRID_KERNEL_RESOURCE_MODEL_HPP

#include <simgrid/kernel/resource/Action.hpp>

namespace simgrid {
namespace kernel {
namespace resource {

/** @ingroup SURF_interface
 * @brief SURF model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class XBT_PUBLIC Model {
public:
  /** @brief Possible update mechanisms */
  enum class UpdateAlgo {
    Full,     /**< Full update mechanism: the remaining time of every action is recomputed at each step */
    Lazy,     /**< Lazy update mechanism: only the modified actions get recomputed.
                      It may be slower than full if your system is tightly coupled to the point where every action
                      gets recomputed anyway. In that case, you'd better not try to be cleaver with lazy and go for
                      a simple full update.  */
    Undefined /**< Mechanism not defined */
  };

  Model();
  virtual ~Model();

  /** @brief Get the set of [actions](@ref Action) in *ready* state */
  Action::StateSet* get_ready_action_set() const { return ready_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *running* state */
  Action::StateSet* get_running_action_set() const { return running_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *failed* state */
  Action::StateSet* get_failed_action_set() const { return failed_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *done* state */
  Action::StateSet* get_done_action_set() const { return done_action_set_; }

  /** @brief Get the set of modified [actions](@ref Action) */
  Action::ModifiedSet* get_modified_set() const;

  /** @brief Get the maxmin system of the current Model */
  lmm::System* get_maxmin_system() const { return maxmin_system_; }

  /** @brief Set the maxmin system of the current Model */
  void set_maxmin_system(lmm::System* system) { maxmin_system_ = system; }

  /**
   * @brief Get the update mechanism of the current Model
   * @see e_UM_t
   */
  UpdateAlgo getUpdateMechanism() const { return update_mechanism_; }
  void setUpdateMechanism(UpdateAlgo mechanism) { update_mechanism_ = mechanism; }

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
  virtual double next_occuring_event(double now);
  virtual double next_occuring_event_lazy(double now);
  virtual double next_occuring_event_full(double now);

  /**
   * @brief Update action to the current time
   *
   * @param now The current time of the simulation
   * @param delta The delta of time since the last update
   */
  virtual void update_actions_state(double now, double delta);
  virtual void update_actions_state_lazy(double now, double delta);
  virtual void update_actions_state_full(double now, double delta);

  /** @brief Returns whether this model have an idempotent shareResource()
   *
   * The only model that is not is NS3: computing the next timestamp moves the model up to that point,
   * so we need to call it only when the next timestamp of other sources is computed.
   */
  virtual bool nextOccuringEventIsIdempotent() { return true; }

private:
  lmm::System* maxmin_system_           = nullptr;
  UpdateAlgo update_mechanism_          = UpdateAlgo::Undefined;
  Action::StateSet* ready_action_set_   = new Action::StateSet(); /**< Actions in state SURF_ACTION_READY */
  Action::StateSet* running_action_set_ = new Action::StateSet(); /**< Actions in state SURF_ACTION_RUNNING */
  Action::StateSet* failed_action_set_  = new Action::StateSet(); /**< Actions in state SURF_ACTION_FAILED */
  Action::StateSet* done_action_set_    = new Action::StateSet(); /**< Actions in state SURF_ACTION_DONE */
  heap_type action_heap_;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif
