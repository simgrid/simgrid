/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_ACTION_HPP
#define SIMGRID_KERNEL_RESOURCE_ACTION_HPP

#include <simgrid/forward.h>
#include <xbt/signal.hpp>
#include <xbt/utility.hpp>

#include <boost/heap/pairing_heap.hpp>
#include <boost/optional.hpp>

const int NO_MAX_DURATION = -1.0;

namespace simgrid {
namespace kernel {
namespace resource {

typedef std::pair<double, simgrid::kernel::resource::Action*> heap_element_type;
typedef boost::heap::pairing_heap<heap_element_type, boost::heap::constant_time_size<false>, boost::heap::stable<true>,
                                  boost::heap::compare<simgrid::xbt::HeapComparator<heap_element_type>>>
    heap_type;

/** @details An action is a consumption on a resource (e.g.: a communication for the network) */
class XBT_PUBLIC Action {
public:
  /* Lazy update needs this Set hook to maintain a list of the tracked actions */
  boost::intrusive::list_member_hook<> modified_set_hook_;
  bool isLinkedModifiedSet() const { return modified_set_hook_.is_linked(); }
  typedef boost::intrusive::list<
      Action, boost::intrusive::member_hook<Action, boost::intrusive::list_member_hook<>, &Action::modified_set_hook_>>
      ModifiedSet;

  boost::intrusive::list_member_hook<> state_set_hook_;
  typedef boost::intrusive::list<
      Action, boost::intrusive::member_hook<Action, boost::intrusive::list_member_hook<>, &Action::state_set_hook_>>
      StateSet;

  enum class State {
    ready = 0,        /**< Ready        */
    running,          /**< Running      */
    failed,           /**< Task Failure */
    done,             /**< Completed    */
    to_free,          /**< Action to free in next cleanup */
    not_in_the_system /**< Not in the system anymore. Why did you ask ? */
  };

  enum class SuspendStates {
    not_suspended = 0, /**< Action currently not suspended **/
    suspended,
    sleeping
  };

  enum class Type { LATENCY = 100, MAX_DURATION, NORMAL, NOTSET };

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

  virtual ~Action();

  /**
   * @brief Mark that the action is now finished
   *
   * @param state the new [state](\ref simgrid::kernel::resource::Action::State) of the current Action
   */
  void finish(Action::State state);

  /** @brief Get the [state](\ref simgrid::kernel::resource::Action::State) of the current Action */
  Action::State get_state() const; /**< get the state*/
  /** @brief Set the [state](\ref simgrid::kernel::resource::Action::State) of the current Action */
  virtual void set_state(Action::State state);

  /** @brief Get the bound of the current Action */
  double get_bound() const;
  /** @brief Set the bound of the current Action */
  void set_bound(double bound);

  /** @brief Get the start time of the current action */
  double get_start_time() const { return start_time_; }
  /** @brief Get the finish time of the current action */
  double getFinishTime() const { return finish_time_; }

  /** @brief Get the user data associated to the current action */
  void* get_data() const { return data_; }
  /** @brief Set the user data associated to the current action */
  void set_data(void* data) { data_ = data; }

  /** @brief Get the cost of the current action */
  double getCost() const { return cost_; }
  /** @brief Set the cost of the current action */
  void setCost(double cost) { cost_ = cost; }

  /** @brief Update the maximum duration of the current action
   *  @param delta Amount to remove from the MaxDuration */
  void updateMaxDuration(double delta);

  /** @brief Update the remaining time of the current action
   *  @param delta Amount to remove from the remaining time */
  void updateRemains(double delta);

  /** @brief Set the remaining time of the current action */
  void setRemains(double value) { remains_ = value; }
  /** @brief Get the remaining time of the current action after updating the resource */
  virtual double getRemains();
  /** @brief Get the remaining time of the current action without updating the resource */
  double getRemainsNoUpdate() const { return remains_; }

  /** @brief Set the finish time of the current action */
  void setFinishTime(double value) { finish_time_ = value; }

  /**@brief Add a reference to the current action (refcounting) */
  void ref();
  /** @brief Unref that action (and destroy it if refcount reaches 0)
   *  @return true if the action was destroyed and false if someone still has references on it
   */
  virtual int unref();

  /** @brief Cancel the current Action if running */
  virtual void cancel();

  /** @brief Suspend the current Action */
  virtual void suspend();

  /** @brief Resume the current Action */
  virtual void resume();

  /** @brief Returns true if the current action is running */
  virtual bool isSuspended();

  /** @brief Get the maximum duration of the current action */
  double getMaxDuration() const { return max_duration_; }
  /** @brief Set the maximum duration of the current Action */
  virtual void setMaxDuration(double duration);

  /** @brief Get the tracing category associated to the current action */
  char* getCategory() const { return category_; }
  /** @brief Set the tracing category of the current Action */
  void setCategory(const char* category);

  /** @brief Get the priority of the current Action */
  double getPriority() const { return sharing_weight_; };
  /** @brief Set the priority of the current Action */
  virtual void setSharingWeight(double priority);
  void setSharingWeightNoUpdate(double weight) { sharing_weight_ = weight; }

  /** @brief Get the state set in which the action is */
  StateSet* getStateSet() const { return state_set_; };

  simgrid::kernel::resource::Model* getModel() const { return model_; }

protected:
  StateSet* state_set_;
  int refcount_ = 1;

private:
  double sharing_weight_ = 1.0;             /**< priority (1.0 by default) */
  double max_duration_   = NO_MAX_DURATION; /*< max_duration (may fluctuate until the task is completed) */
  double remains_;           /**< How much of that cost remains to be done in the currently running task */
  double start_time_;        /**< start time  */
  char* category_ = nullptr; /**< tracing category for categorized resource utilization monitoring */
  double finish_time_ =
      -1; /**< finish time : this is modified during the run and fluctuates until the task is completed */

  double cost_;
  simgrid::kernel::resource::Model* model_;
  void* data_ = nullptr; /**< for your convenience */

  /* LMM */
  double last_update_                                  = 0;
  double last_value_                                   = 0;
  kernel::lmm::Variable* variable_                    = nullptr;
  Action::Type type_                                  = Action::Type::NOTSET;
  boost::optional<heap_type::handle_type> heap_handle_ = boost::none;

public:
  virtual void updateRemainingLazy(double now) = 0;
  void heapInsert(heap_type& heap, double key, Action::Type hat);
  void heapRemove(heap_type& heap);
  void heapUpdate(heap_type& heap, double key, Action::Type hat);
  void clearHeapHandle() { heap_handle_ = boost::none; }
  kernel::lmm::Variable* getVariable() const { return variable_; }
  void setVariable(kernel::lmm::Variable* var) { variable_ = var; }
  double getLastUpdate() const { return last_update_; }
  void refreshLastUpdate();
  double getLastValue() const { return last_value_; }
  void setLastValue(double val) { last_value_ = val; }
  Action::Type getType() const { return type_; }

protected:
  Action::SuspendStates suspended_ = Action::SuspendStates::not_suspended;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif
