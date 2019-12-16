/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_ACTION_HPP
#define SIMGRID_KERNEL_RESOURCE_ACTION_HPP

#include <simgrid/forward.h>
#include <xbt/signal.hpp>
#include <xbt/utility.hpp>

#include <boost/heap/pairing_heap.hpp>
#include <boost/optional.hpp>
#include <string>

static constexpr int NO_MAX_DURATION = -1.0;

namespace simgrid {
namespace kernel {
namespace resource {

typedef std::pair<double, Action*> heap_element_type;
typedef boost::heap::pairing_heap<heap_element_type, boost::heap::constant_time_size<false>, boost::heap::stable<true>,
                                  boost::heap::compare<simgrid::xbt::HeapComparator<heap_element_type>>>
    heap_type;

typedef std::pair<double, Action*> heap_element_type;
class XBT_PUBLIC ActionHeap : public heap_type {
  friend Action;

public:
  enum class Type {
    latency = 100, /* this is a heap entry to warn us when the latency is payed */
    max_duration,  /* this is a heap entry to warn us when the max_duration limit (timeout) is reached */
    normal,        /* this is a normal heap entry stating the date to finish transmitting */
    unset
  };

  double top_date() const;
  void insert(Action* action, double date, ActionHeap::Type type);
  void update(Action* action, double date, ActionHeap::Type type);
  void remove(Action* action);
  Action* pop();
};

/** @details An action is a consumption on a resource (e.g.: a communication for the network).
 *
 * It is related (but still different) from activities, that are the stuff on which an actor can be blocked.
 *
 * - A sequential execution activity encompasses 2 actions: one for the exec itself,
 *   and a time-limited sleep used as timeout detector.
 * - A point-to-point communication activity encompasses 3 actions: one for the comm itself
 *   (which spans on all links of the path), and one infinite sleep used as failure detector
 *   on both sender and receiver hosts.
 * - Synchronization activities may possibly be connected to no action.

 */
class XBT_PUBLIC Action {
  friend ActionHeap;

public:
  /* Lazy update needs this Set hook to maintain a list of the tracked actions */
  boost::intrusive::list_member_hook<> modified_set_hook_;
  bool is_within_modified_set() const { return modified_set_hook_.is_linked(); }
  typedef boost::intrusive::list<
      Action, boost::intrusive::member_hook<Action, boost::intrusive::list_member_hook<>, &Action::modified_set_hook_>>
      ModifiedSet;

  boost::intrusive::list_member_hook<> state_set_hook_;
  typedef boost::intrusive::list<
      Action, boost::intrusive::member_hook<Action, boost::intrusive::list_member_hook<>, &Action::state_set_hook_>>
      StateSet;

  enum class State {
    INITED,   /**< Created, but not started yet */
    STARTED,  /**< Currently running */
    FAILED,   /**< either the resource failed, or the action was canceled */
    FINISHED, /**< Successfully completed  */
    IGNORED   /**< e.g. failure detectors: infinite sleep actions that are put on resources which failure should get
                 noticed  */
  };

  enum class SuspendStates {
    RUNNING = 0, /**< Action currently not suspended **/
    SUSPENDED,
    SLEEPING
  };

  /**
   * @brief Action constructor
   *
   * @param model The Model associated to this Action
   * @param cost The cost of the Action
   * @param failed If the action is impossible (e.g.: execute something on a switched off host)
   */
  Action(Model* model, double cost, bool failed);

  /**
   * @brief Action constructor
   *
   * @param model The Model associated to this Action
   * @param cost The cost of the Action
   * @param failed If the action is impossible (e.g.: execute something on a switched off host)
   * @param var The lmm variable associated to this Action if it is part of a LMM component
   */
  Action(Model* model, double cost, bool failed, lmm::Variable* var);
  Action(const Action&) = delete;
  Action& operator=(const Action&) = delete;

  virtual ~Action();

  /**
   * @brief Mark that the action is now finished
   *
   * @param state the new [state](@ref simgrid::kernel::resource::Action::State) of the current Action
   */
  void finish(Action::State state);

  /** @brief Get the [state](@ref simgrid::kernel::resource::Action::State) of the current Action */
  Action::State get_state() const; /**< get the state*/
  /** @brief Set the [state](@ref simgrid::kernel::resource::Action::State) of the current Action */
  virtual void set_state(Action::State state);

  /** @brief Get the bound of the current Action */
  double get_bound() const;
  /** @brief Set the bound of the current Action */
  void set_bound(double bound);

  /** @brief Get the start time of the current action */
  double get_start_time() const { return start_time_; }
  /** @brief Get the finish time of the current action */
  double get_finish_time() const { return finish_time_; }

  /** @brief Get the user data associated to the current action */
  void* get_data() const { return data_; }
  /** @brief Set the user data associated to the current action */
  void set_data(void* data) { data_ = data; }

  /** @brief Get the user data associated to the current action */
  activity::ActivityImpl* get_activity() const { return activity_; }
  /** @brief Set the user data associated to the current action */
  void set_activity(activity::ActivityImpl* activity) { activity_ = activity; }

  /** @brief Get the cost of the current action */
  double get_cost() const { return cost_; }
  /** @brief Set the cost of the current action */
  void set_cost(double cost) { cost_ = cost; }

  /** @brief Update the maximum duration of the current action
   *  @param delta Amount to remove from the MaxDuration */
  void update_max_duration(double delta);

  /** @brief Update the remaining time of the current action
   *  @param delta Amount to remove from the remaining time */
  void update_remains(double delta);

  virtual void update_remains_lazy(double now) = 0;

  /** @brief Set the remaining time of the current action */
  void set_remains(double value) { remains_ = value; }

  /** @brief Get the remaining time of the current action after updating the resource */
  virtual double get_remains();
  /** @brief Get the remaining time of the current action without updating the resource */
  double get_remains_no_update() const { return remains_; }

  /** @brief Set the finish time of the current action */
  void set_finish_time(double value) { finish_time_ = value; }

  /**@brief Add a reference to the current action (refcounting) */
  void ref();
  /** @brief Unref that action (and destroy it if refcount reaches 0)
   *  @return true if the action was destroyed and false if someone still has references on it */
  bool unref();

  /** @brief Cancel the current Action if running */
  virtual void cancel();

  /** @brief Suspend the current Action */
  virtual void suspend();

  /** @brief Resume the current Action */
  virtual void resume();

  /** @brief Returns true if the current action is suspended */
  bool is_suspended() const { return suspended_ == SuspendStates::SUSPENDED; }
  /** @brief Returns true if the current action is running */
  bool is_running() const { return suspended_ == SuspendStates::RUNNING; }

  /** @brief Get the maximum duration of the current action */
  double get_max_duration() const { return max_duration_; }
  /** @brief Set the maximum duration of the current Action */
  virtual void set_max_duration(double duration);

  /** @brief Get the tracing category associated to the current action */
  const std::string& get_category() const { return category_; }
  /** @brief Set the tracing category of the current Action */
  void set_category(const std::string& category) { category_ = category; }

  /** @brief Get the sharing_penalty (RTT or 1/thread_count) of the current Action */
  double get_sharing_penalty() const { return sharing_penalty_; };
  /** @brief Set the sharing_penalty (RTT or 1/thread_count) of the current Action */
  virtual void set_sharing_penalty(double sharing_penalty);
  void set_sharing_penalty_no_update(double sharing_penalty) { sharing_penalty_ = sharing_penalty; }

  /** @brief Get the state set in which the action is */
  StateSet* get_state_set() const { return state_set_; };

  simgrid::kernel::resource::Model* get_model() const { return model_; }

private:
  StateSet* state_set_;
  Action::SuspendStates suspended_ = Action::SuspendStates::RUNNING;
  int refcount_            = 1;
  double sharing_penalty_          = 1.0;             /**< priority (1.0 by default) */
  double max_duration_   = NO_MAX_DURATION; /*< max_duration (may fluctuate until the task is completed) */
  double remains_;           /**< How much of that cost remains to be done in the currently running task */
  double start_time_;        /**< start time  */
  double finish_time_ = -1;  /**< finish time (may fluctuate until the task is completed) */
  std::string category_;     /**< tracing category for categorized resource utilization monitoring */

  double cost_;
  Model* model_;
  void* data_                       = nullptr; /**< for your convenience */
  activity::ActivityImpl* activity_ = nullptr;

  /* LMM */
  double last_update_      = 0;
  double last_value_       = 0;
  lmm::Variable* variable_ = nullptr;

  ActionHeap::Type type_                              = ActionHeap::Type::unset;
  boost::optional<ActionHeap::handle_type> heap_hook_ = boost::none;

public:
  ActionHeap::Type get_type() const { return type_; }

  lmm::Variable* get_variable() const { return variable_; }
  void set_variable(lmm::Variable* var) { variable_ = var; }

  double get_last_update() const { return last_update_; }
  void set_last_update();

  double get_last_value() const { return last_value_; }
  void set_last_value(double val) { last_value_ = val; }
  void set_suspend_state(Action::SuspendStates state) { suspended_ = state; }
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif
