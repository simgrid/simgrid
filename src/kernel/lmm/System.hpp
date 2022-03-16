/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_LMM_SYSTEM_HPP
#define SIMGRID_KERNEL_LMM_SYSTEM_HPP

#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/mallocator.h"

#include <boost/intrusive/list.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace simgrid {
namespace kernel {
namespace lmm {

/** @addtogroup SURF_lmm
 * @details
 * A linear maxmin solver to resolve inequations systems.
 *
 * Most SimGrid model rely on a "fluid/steady-state" modeling that simulate the sharing of resources between actions at
 * relatively coarse-grain.  Such sharing is generally done by solving a set of linear inequations. Let's take an
 * example and assume we have the variables \f$x_1\f$, \f$x_2\f$, \f$x_3\f$, and \f$x_4\f$ . Let's say that \f$x_1\f$
 * and \f$x_2\f$ correspond to activities running and the same CPU \f$A\f$ whose capacity is \f$C_A\f$. In such a
 * case, we need to enforce:
 *
 *   \f[ x_1 + x_2 \leq C_A \f]
 *
 * Likewise, if \f$x_3\f$ (resp. \f$x_4\f$) corresponds to a network flow \f$F_3\f$ (resp. \f$F_4\f$) that goes through
 * a set of links \f$L_1\f$ and \f$L_2\f$ (resp. \f$L_2\f$ and \f$L_3\f$), then we need to enforce:
 *
 *   \f[ x_3  \leq C_{L_1} \f]
 *   \f[ x_3 + x_4 \leq C_{L_2} \f]
 *   \f[ x_4 \leq C_{L_3} \f]
 *
 * One could set every variable to 0 to make sure the constraints are satisfied but this would obviously not be very
 * realistic. A possible objective is to try to maximize the minimum of the \f$x_i\f$ . This ensures that all the
 * \f$x_i\f$ are positive and "as large as possible".
 *
 * This is called *max-min fairness* and is the most commonly used objective in SimGrid. Another possibility is to
 * maximize \f$\sum_if(x_i)\f$, where \f$f\f$ is a strictly increasing concave function.
 *
 * Constraint:
 *  - bound (set)
 *  - shared (set)
 *  - usage (computed)
 *
 * Variable:
 *  - weight (set)
 *  - bound (set)
 *  - value (computed)
 *
 * Element:
 *  - value (set)
 *
 * A possible system could be:
 * - three variables: `var1`, `var2`, `var3`
 * - two constraints: `cons1`, `cons2`
 * - four elements linking:
 *  - `elem1` linking `var1` and `cons1`
 *  - `elem2` linking `var2` and `cons1`
 *  - `elem3` linking `var2` and `cons2`
 *  - `elem4` linking `var3` and `cons2`
 *
 * And the corresponding inequations will be:
 *
 *     var1.value <= var1.bound
 *     var2.value <= var2.bound
 *     var3.value <= var3.bound
 *     var1.weight * var1.value * elem1.value + var2.weight * var2.value * elem2.value <= cons1.bound
 *     var2.weight * var2.value * elem3.value + var3.weight * var3.value * elem4.value <= cons2.bound
 *
 * where `var1.value`, `var2.value` and `var3.value` are the unknown values.
 *
 * If a constraint is not shared, the sum is replaced by a max.
 * For example, a third non-shared constraint `cons3` and the associated elements `elem5` and `elem6` could write as:
 *
 *     max( var1.weight * var1.value * elem5.value  ,  var3.weight * var3.value * elem6.value ) <= cons3.bound
 *
 * This is useful for the sharing of resources for various models.
 * For instance, for the network model, each link is associated to a constraint and each communication to a variable.
 *
 * Implementation details
 *
 * For implementation reasons, we are interested in distinguishing variables that actually participate to the
 * computation of constraints, and those who are part of the equations but are stuck to zero.
 * We call enabled variables, those which var.weight is strictly positive. Zero-weight variables are called disabled
 * variables.
 * Unfortunately this concept of enabled/disabled variables intersects with active/inactive variable.
 * Semantically, the intent is similar, but the conditions under which a variable is active is slightly more strict
 * than the conditions for it to be enabled.
 * A variable is active only if its var.value is non-zero (and, by construction, its var.weight is non-zero).
 * In general, variables remain disabled after their creation, which often models an initialization phase (e.g. first
 * packet propagating in the network). Then, it is enabled by the corresponding model. Afterwards, the max-min solver
 * (lmm_solve()) activates it when appropriate. It is possible that the variable is again disabled, e.g. to model the
 * pausing of an action.
 *
 * Concurrency limit and maximum
 *
 * We call concurrency, the number of variables that can be enabled at any time for each constraint.
 * From a model perspective, this "concurrency" often represents the number of actions that actually compete for one
 * constraint.
 * The LMM solver is able to limit the concurrency for each constraint, and to monitor its maximum value.
 *
 * One may want to limit the concurrency of constraints for essentially three reasons:
 *  - Keep LMM system in a size that can be solved (it does not react very well with tens of thousands of variables per
 *    constraint)
 *  - Stay within parameters where the fluid model is accurate enough.
 *  - Model serialization effects
 *
 * The concurrency limit can also be set to a negative value to disable concurrency limit. This can improve performance
 * slightly.
 *
 * Overall, each constraint contains three fields related to concurrency:
 *  - concurrency_limit which is the limit enforced by the solver
 *  - concurrency_current which is the current concurrency
 *  - concurrency_maximum which is the observed maximum concurrency
 *
 * Variables also have one field related to concurrency: concurrency_share.
 * In effect, in some cases, one variable is involved multiple times (i.e. two elements) in a constraint.
 * For example, cross-traffic is modeled using 2 elements per constraint.
 * concurrency_share formally corresponds to the maximum number of elements that associate the variable and any given
 * constraint.
 */

/** @{ @ingroup SURF_lmm */

/**
 * @brief LMM element
 * Elements can be seen as glue between constraint objects and variable objects.
 * Basically, each variable will have a set of elements, one for each constraint where it is involved.
 * Then, it is used to list all variables involved in constraint through constraint's xxx_element_set lists, or
 * vice-versa list all constraints for a given variable.
 */
class XBT_PUBLIC Element {
public:
  // Use rule-of-three, and implicitely disable the move constructor which should be 'noexcept' according to C++ Core
  // Guidelines.
  Element(Constraint* constraint, Variable* variable, double cweight);
  Element(const Element&) = default;
  ~Element()              = default;

  int get_concurrency() const;
  void decrease_concurrency();
  void increase_concurrency();

  void make_active();
  void make_inactive();

  /* hookup to constraint */
  boost::intrusive::list_member_hook<> enabled_element_set_hook;
  boost::intrusive::list_member_hook<> disabled_element_set_hook;
  boost::intrusive::list_member_hook<> active_element_set_hook;

  Constraint* constraint;
  Variable* variable;

  // consumption_weight: impact of 1 byte or flop of your application onto the resource (in byte or flop)
  //   - if CPU, then probably 1.
  //   - If network, then 1 in forward direction and 0.05 backward for the ACKs
  double consumption_weight;
  // maximum consumption weight (can be different from consumption_weight with subflows/ptasks)
  double max_consumption_weight;
};

class ConstraintLight {
public:
  double remaining_over_usage;
  Constraint* cnst;
};

/**
 * @brief LMM constraint
 * Each constraint contains several partially overlapping logical sets of elements:
 * \li Disabled elements which variable's weight is zero. This variables are not at all processed by LMM, but eventually
 *     the corresponding action will enable it (at least this is the idea).
 * \li Enabled elements which variable's weight is non-zero. They are utilized in some LMM functions.
 * \li Active elements which variable's weight is non-zero (i.e. it is enabled) AND its element value is non-zero.
 *     LMM_solve iterates over active elements during resolution, dynamically making them active or inactive.
 */
class XBT_PUBLIC Constraint {
public:
  enum class SharingPolicy { NONLINEAR = 2, SHARED = 1, FATPIPE = 0 };

  Constraint(resource::Resource* id_value, double bound_value);

  /** @brief Unshare a constraint. */
  void unshare() { sharing_policy_ = SharingPolicy::FATPIPE; }

  /** @brief Set how a constraint is shared  */
  void set_sharing_policy(SharingPolicy policy, const s4u::NonLinearResourceCb& cb);
  /** @brief Check how a constraint is shared  */
  SharingPolicy get_sharing_policy() const { return sharing_policy_; }

  /** @brief Get the usage of the constraint after the last lmm solve */
  double get_usage() const;
  int get_variable_amount() const;

  /** @brief Sets the concurrency limit for this constraint */
  void set_concurrency_limit(int limit)
  {
    xbt_assert(limit < 0 || concurrency_maximum_ <= limit,
               "New concurrency limit should be larger than observed concurrency maximum. Maybe you want to call"
               " concurrency_maximum_reset() to reset the maximum?");
    concurrency_limit_ = limit;
  }

  /** @brief Gets the concurrency limit for this constraint */
  int get_concurrency_limit() const { return concurrency_limit_; }

  /**
   * @brief Reset the concurrency maximum for a given variable (we will update the maximum to reflect constraint
   * evolution).
   */
  void reset_concurrency_maximum() { concurrency_maximum_ = 0; }

  /** @brief Get the concurrency maximum for a given constraint (which reflects constraint evolution). */
  int get_concurrency_maximum() const
  {
    xbt_assert(concurrency_limit_ < 0 || concurrency_maximum_ <= concurrency_limit_,
               "Very bad: maximum observed concurrency is higher than limit. This is a bug of SURF, please report it.");
    return concurrency_maximum_;
  }

  int get_concurrency_slack() const
  {
    return concurrency_limit_ < 0 ? std::numeric_limits<int>::max() : concurrency_limit_ - concurrency_current_;
  }

  /**
   * @brief Get a var associated to a constraint
   * @details Get the first variable of the next variable of elem if elem is not NULL
   * @param elem A element of constraint of the constraint or NULL
   * @return A variable associated to a constraint
   */
  Variable* get_variable(const Element** elem) const;

  /**
   * @brief Get a var associated to a constraint
   * @details Get the first variable of the next variable of elem if elem is not NULL
   * @param elem A element of constraint of the constraint or NULL
   * @param nextelem A element of constraint of the constraint or NULL, the one after elem
   * @param numelem parameter representing the number of elements to go
   * @return A variable associated to a constraint
   */
  Variable* get_variable_safe(const Element** elem, const Element** nextelem, size_t* numelem) const;

  /**
   * @brief Get the data associated to a constraint
   * @return The data associated to the constraint
   */
  resource::Resource* get_id() const { return id_; }

  /* hookup to system */
  boost::intrusive::list_member_hook<> constraint_set_hook_;
  boost::intrusive::list_member_hook<> active_constraint_set_hook_;
  boost::intrusive::list_member_hook<> modified_constraint_set_hook_;
  boost::intrusive::list_member_hook<> saturated_constraint_set_hook_;
  boost::intrusive::list<Element, boost::intrusive::member_hook<Element, boost::intrusive::list_member_hook<>,
                                                                &Element::enabled_element_set_hook>>
      enabled_element_set_;
  boost::intrusive::list<Element, boost::intrusive::member_hook<Element, boost::intrusive::list_member_hook<>,
                                                                &Element::disabled_element_set_hook>>
      disabled_element_set_;
  boost::intrusive::list<Element, boost::intrusive::member_hook<Element, boost::intrusive::list_member_hook<>,
                                                                &Element::active_element_set_hook>>
      active_element_set_;
  double remaining_ = 0.0;
  double usage_     = 0.0;
  double bound_;
  double dynamic_bound_ = 0.0; //!< dynamic bound for this constraint, defined by user's callback
  // TODO MARTIN Check maximum value across resources at the end of simulation and give a warning is more than e.g. 500
  int concurrency_current_ = 0; /* The current concurrency */
  int concurrency_maximum_ = 0; /* The maximum number of (enabled and disabled) variables associated to the constraint
                                 * at any given time (essentially for tracing)*/

  SharingPolicy sharing_policy_ = SharingPolicy::SHARED;
  int rank_; // Only used in debug messages to identify the constraint
  double lambda_               = 0.0;
  double new_lambda_           = 0.0;
  ConstraintLight* cnst_light_ = nullptr;
  s4u::NonLinearResourceCb dyn_constraint_cb_;

private:
  static int next_rank_;  // To give a separate rank_ to each constraint
  int concurrency_limit_ = sg_concurrency_limit; /* The maximum number of variables that may be enabled at any time
                                                  * (stage variables if necessary) */
  resource::Resource* id_;
};

/**
 * @brief LMM variable
 *
 * When something prevents us from enabling a variable, we "stage" the weight that we would have like to set, so that as
 * soon as possible we enable the variable with desired weight
 */
class XBT_PUBLIC Variable {
public:
  void initialize(resource::Action* id_value, double sharing_penalty, double bound_value, size_t number_of_constraints,
                  unsigned visited_value);

  /** @brief Get the value of the variable after the last lmm solve */
  double get_value() const { return value_; }

  /** @brief Get the maximum value of the variable (-1.0 if no specified maximum) */
  double get_bound() const { return bound_; }

  /**
   * @brief Set the concurrent share of the variable
   * @param value The new concurrency share
   */
  void set_concurrency_share(short int value) { concurrency_share_ = value; }

  /**
   * @brief Get the numth constraint associated to the variable
   * @param num The rank of constraint we want to get
   * @return The numth constraint
   */
  Constraint* get_constraint(unsigned num) const { return num < cnsts_.size() ? cnsts_[num].constraint : nullptr; }

  /**
   * @brief Get the weight of the numth constraint associated to the variable
   * @param num The rank of constraint we want to get
   * @return The numth constraint
   */
  double get_constraint_weight(unsigned num) const
  {
    return num < cnsts_.size() ? cnsts_[num].consumption_weight : 0.0;
  }

  /** @brief Get the number of constraint associated to a variable */
  size_t get_number_of_constraint() const { return cnsts_.size(); }

  /** @brief Get the data associated to a variable */
  resource::Action* get_id() const { return id_; }

  /** @brief Get the penalty of a variable */
  double get_penalty() const { return sharing_penalty_; }

  /** @brief Measure the minimum concurrency slack across all constraints where the given var is involved */
  int get_min_concurrency_slack() const;

  /** @brief Check if a variable can be enabled
   * Make sure to set staged_penalty before, if your intent is only to check concurrency
   */
  bool can_enable() const { return staged_penalty_ > 0 && get_min_concurrency_slack() >= concurrency_share_; }

  /* hookup to system */
  boost::intrusive::list_member_hook<> variable_set_hook_;
  boost::intrusive::list_member_hook<> saturated_variable_set_hook_;

  std::vector<Element> cnsts_;

  // sharing_penalty: variable's impact on the resource during the sharing
  //   if == 0, the variable is not considered by LMM
  //   on CPU, actions with N threads have a sharing of N
  //   on network, the actions with higher latency have a lesser sharing_penalty
  double sharing_penalty_;

  double staged_penalty_; /* If non-zero, variable is staged for addition as soon as maxconcurrency constraints will be
                            met */
  double bound_;
  double value_;
  short int concurrency_share_; /* The maximum number of elements that variable will add to a constraint */
  resource::Action* id_;
  int rank_;         // Only used in debug messages to identify the variable
  unsigned visited_; /* used by System::update_modified_cnst_set() */
  double mu_;

private:
  static int next_rank_; // To give a separate rank_ to each variable
};

inline void Element::make_active()
{
  constraint->active_element_set_.push_front(*this);
}
inline void Element::make_inactive()
{
  if (active_element_set_hook.is_linked())
    xbt::intrusive_erase(constraint->active_element_set_, *this);
}

/**
 * @brief LMM system
 */
class XBT_PUBLIC System {
public:
  /**
   * @brief Creates a new System solver
   *
   * @param solver_name Name of the solver to be used
   * @param selective_update Enables lazy updates
   * @return pointer to System instance
   */
  static System* build(const std::string& solver_name, bool selective_update);
  /** @brief Validates solver configuration */
  static void validate_solver(const std::string& solver_name);

  /**
   * @brief Create a new Linear MaxMim system
   * @param selective_update whether we should do lazy updates
   */
  explicit System(bool selective_update);
  /** @brief Free an existing Linear MaxMin system */
  virtual ~System();

  /**
   * @brief Create a new Linear MaxMin constraint
   * @param id Data associated to the constraint (e.g.: a network link)
   * @param bound_value The bound value of the constraint
   */
  Constraint* constraint_new(resource::Resource* id, double bound_value);

  /**
   * @brief Create a new Linear MaxMin variable
   * @param id Data associated to the variable (e.g.: a network communication)
   * @param sharing_penalty The weight of the variable (0.0 if not used)
   * @param bound The maximum value of the variable (-1.0 if no maximum value)
   * @param number_of_constraints The maximum number of constraints to associate to the variable
   */
  Variable* variable_new(resource::Action* id, double sharing_penalty, double bound = -1.0,
                         size_t number_of_constraints = 1);

  /** @brief Get the list of modified actions since last solve() */
  resource::Action::ModifiedSet* get_modified_action_set() const;

  /**
   * @brief Free a variable
   * @param var The variable to free
   */
  void variable_free(Variable * var);

  /** @brief Free all variables */
  void variable_free_all();

  /**
   * @brief Associate a variable to a constraint with a coefficient
   * @param cnst A constraint
   * @param var A variable
   * @param value The coefficient associated to the variable in the constraint
   */
  void expand(Constraint * cnst, Variable * var, double value);

  /**
   * @brief Add value to the coefficient between a constraint and a variable or create one
   * @param cnst A constraint
   * @param var A variable
   * @param value The value to add to the coefficient associated to the variable in the constraint
   */
  void expand_add(Constraint * cnst, Variable * var, double value);

  /** @brief Update the bound of a variable */
  void update_variable_bound(Variable * var, double bound);

  /** @brief Update the sharing penalty of a variable */
  void update_variable_penalty(Variable* var, double penalty);

  /** @brief Update a constraint bound */
  void update_constraint_bound(Constraint * cnst, double bound);

  int constraint_used(const Constraint* cnst) const { return cnst->active_constraint_set_hook_.is_linked(); }

  /** @brief Print the lmm system */
  void print() const;

  /** @brief Solve the lmm system. May be specialized in subclasses. */
  void solve();

private:
  static void* variable_mallocator_new_f();
  static void variable_mallocator_free_f(void* var);
  /** @brief Implements the solver. Must be specialized in subclasses. */
  virtual void do_solve() = 0;

  void var_free(Variable * var);
  void cnst_free(Constraint * cnst);
  Variable* extract_variable()
  {
    if (variable_set.empty())
      return nullptr;
    Variable* res = &variable_set.front();
    variable_set.pop_front();
    return res;
  }
  Constraint* extract_constraint()
  {
    if (constraint_set.empty())
      return nullptr;
    Constraint* res = &constraint_set.front();
    constraint_set.pop_front();
    return res;
  }
  void insert_constraint(Constraint * cnst) { constraint_set.push_back(*cnst); }
  void remove_variable(Variable * var)
  {
    if (var->variable_set_hook_.is_linked())
      xbt::intrusive_erase(variable_set, *var);
    if (var->saturated_variable_set_hook_.is_linked())
      xbt::intrusive_erase(saturated_variable_set, *var);
  }
  void make_constraint_active(Constraint * cnst)
  {
    if (not cnst->active_constraint_set_hook_.is_linked())
      active_constraint_set.push_back(*cnst);
  }
  void make_constraint_inactive(Constraint * cnst)
  {
    if (cnst->active_constraint_set_hook_.is_linked())
      xbt::intrusive_erase(active_constraint_set, *cnst);
    if (cnst->modified_constraint_set_hook_.is_linked())
      xbt::intrusive_erase(modified_constraint_set, *cnst);
  }

  void enable_var(Variable * var);
  void disable_var(Variable * var);
  void on_disabled_var(Constraint * cnstr);

  /**
   * @brief Update the value of element linking the constraint and the variable
   * @param cnst A constraint
   * @param var A variable
   * @param value The new value
   */
  void update(Constraint * cnst, Variable * var, double value);

  /** @brief Given a variable, update modified_constraint_set_ */
  void update_modified_cnst_set_from_variable(const Variable* var);
  void update_modified_cnst_set(Constraint* cnst);
  void update_modified_cnst_set_rec(const Constraint* cnst);

public:
  bool modified_ = false;
  boost::intrusive::list<Variable, boost::intrusive::member_hook<Variable, boost::intrusive::list_member_hook<>,
                                                                 &Variable::variable_set_hook_>>
      variable_set;
  boost::intrusive::list<Constraint, boost::intrusive::member_hook<Constraint, boost::intrusive::list_member_hook<>,
                                                                   &Constraint::active_constraint_set_hook_>>
      active_constraint_set;
  boost::intrusive::list<Variable, boost::intrusive::member_hook<Variable, boost::intrusive::list_member_hook<>,
                                                                 &Variable::saturated_variable_set_hook_>>
      saturated_variable_set;
  boost::intrusive::list<Constraint, boost::intrusive::member_hook<Constraint, boost::intrusive::list_member_hook<>,
                                                                   &Constraint::saturated_constraint_set_hook_>>
      saturated_constraint_set;

protected:
  bool selective_update_active; /* flag to update partially the system only selecting changed portions */
  boost::intrusive::list<Constraint, boost::intrusive::member_hook<Constraint, boost::intrusive::list_member_hook<>,
                                                                   &Constraint::modified_constraint_set_hook_>>
      modified_constraint_set;

  /** @brief Remove all constraints of the modified_constraint_set. */
  void remove_all_modified_cnst_set();
  void check_concurrency() const;

private:
  unsigned visited_counter_ =
      1; /* used by System::update_modified_cnst_set() and System::remove_all_modified_cnst_set() to
          * cleverly (un-)flag the constraints (more details in these functions) */
  boost::intrusive::list<Constraint, boost::intrusive::member_hook<Constraint, boost::intrusive::list_member_hook<>,
                                                                   &Constraint::constraint_set_hook_>>
      constraint_set;
  xbt_mallocator_t variable_mallocator_ =
      xbt_mallocator_new(65536, System::variable_mallocator_new_f, System::variable_mallocator_free_f, nullptr);

  std::unique_ptr<resource::Action::ModifiedSet> modified_set_ = nullptr;
};

/** @} */
} // namespace lmm
} // namespace kernel
} // namespace simgrid

#endif