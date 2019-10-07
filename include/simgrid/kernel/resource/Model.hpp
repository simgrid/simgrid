/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_MODEL_HPP
#define SIMGRID_KERNEL_RESOURCE_MODEL_HPP

#include <memory>
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
    FULL, /**< Full update mechanism: the remaining time of every action is recomputed at each step */
    LAZY  /**< Lazy update mechanism: only the modified actions get recomputed.
                   It may be slower than full if your system is tightly coupled to the point where every action
                   gets recomputed anyway. In that case, you'd better not try to be cleaver with lazy and go for
                   a simple full update.  */
  };

  explicit Model(Model::UpdateAlgo algo);
  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

  virtual ~Model();

  /** @brief Get the set of [actions](@ref Action) in *inited* state */
  Action::StateSet* get_inited_action_set() { return &inited_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *started* state */
  Action::StateSet* get_started_action_set() { return &started_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *failed* state */
  Action::StateSet* get_failed_action_set() { return &failed_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *finished* state */
  Action::StateSet* get_finished_action_set() { return &finished_action_set_; }

  /** @brief Get the set of [actions](@ref Action) in *ignored* state */
  Action::StateSet* get_ignored_action_set() { return &ignored_action_set_; }

  /** @brief Get the set of modified [actions](@ref Action) */
  Action::ModifiedSet* get_modified_set() const;

  /** @brief Get the maxmin system of the current Model */
  lmm::System* get_maxmin_system() const { return maxmin_system_.get(); }

  /** @brief Set the maxmin system of the current Model */
  void set_maxmin_system(lmm::System* system);

  /** @brief Get the update algorithm of the current Model */
  UpdateAlgo get_update_algorithm() const { return update_algorithm_; }

  /** @brief Get Action heap */
  ActionHeap& get_action_heap() { return action_heap_; }

  /**
   * @brief Share the resources between the actions
   *
   * @param now The current time of the simulation
   * @return The delta of time till the next action will finish
   */
  virtual double next_occuring_event(double now);
  virtual double next_occuring_event_lazy(double now);
  virtual double next_occuring_event_full(double now);

private:
  Action* extract_action(Action::StateSet* list);

public:
  Action* extract_done_action();
  Action* extract_failed_action();

  /**
   * @brief Update action to the current time
   *
   * @param now The current time of the simulation
   * @param delta The delta of time since the last update
   */
  virtual void update_actions_state(double now, double delta);
  virtual void update_actions_state_lazy(double now, double delta);
  virtual void update_actions_state_full(double now, double delta);

  /** @brief Returns whether this model have an idempotent share_resource()
   *
   * The only model that is not is ns-3: computing the next timestamp moves the model up to that point,
   * so we need to call it only when the next timestamp of other sources is computed.
   */
  virtual bool next_occuring_event_is_idempotent() { return true; }

private:
  std::unique_ptr<lmm::System> maxmin_system_;
  const UpdateAlgo update_algorithm_;
  Action::StateSet inited_action_set_;   /**< Created not started */
  Action::StateSet started_action_set_;  /**< Started not done */
  Action::StateSet failed_action_set_;   /**< Done with failure */
  Action::StateSet finished_action_set_; /**< Done successful */
  Action::StateSet ignored_action_set_;  /**< not considered (failure detectors?) */

  ActionHeap action_heap_;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

/** @ingroup SURF_models
 *  @brief List of initialized models
 */
XBT_PUBLIC_DATA std::vector<simgrid::kernel::resource::Model*> all_existing_models;

#endif
