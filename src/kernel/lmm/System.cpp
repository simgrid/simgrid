/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/lmm/fair_bottleneck.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#if SIMGRID_HAVE_EIGEN3
#include "src/kernel/lmm/bmf.hpp"
#endif
#include <boost/core/demangle.hpp>
#include <typeinfo>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_lmm, kernel, "Kernel Linear Max-Min solver");

double sg_maxmin_precision = 1E-5; /* Change this with --cfg=maxmin/precision:VALUE */
double sg_surf_precision   = 1E-9; /* Change this with --cfg=surf/precision:VALUE */
int sg_concurrency_limit   = -1;      /* Change this with --cfg=maxmin/concurrency-limit:VALUE */

namespace simgrid {
namespace kernel {
namespace lmm {

int Variable::next_rank_   = 1;
int Constraint::next_rank_ = 1;

Element::Element(Constraint* constraint, Variable* variable, double cweight)
    : constraint(constraint), variable(variable), consumption_weight(cweight), max_consumption_weight(cweight)
{
}

int Element::get_concurrency() const
{
  // Ignore element with weight less than one (e.g. cross-traffic)
  return (consumption_weight >= 1) ? 1 : 0;
  // There are other alternatives, but they will change the behavior of the model..
  // So do not use it unless you want to make a new model.
  // If you do, remember to change the variables concurrency share to reflect it.
  // Potential examples are:
  // return (elem->weight>0)?1:0;//Include element as soon  as weight is non-zero
  // return (int)ceil(elem->weight);//Include element as the rounded-up integer value of the element weight
}

void Element::decrease_concurrency()
{
  xbt_assert(constraint->concurrency_current_ >= get_concurrency());
  constraint->concurrency_current_ -= get_concurrency();
}

void Element::increase_concurrency()
{
  constraint->concurrency_current_ += get_concurrency();

  if (constraint->concurrency_current_ > constraint->concurrency_maximum_)
    constraint->concurrency_maximum_ = constraint->concurrency_current_;

  xbt_assert(constraint->get_concurrency_limit() < 0 ||
                 constraint->concurrency_current_ <= constraint->get_concurrency_limit(),
             "Concurrency limit overflow!");
}

System* System::build(const std::string& solver_name, bool selective_update)
{
  System* system = nullptr;
  if (solver_name == "bmf") {
#if SIMGRID_HAVE_EIGEN3
    system = new BmfSystem(selective_update);
#endif
  } else if (solver_name == "fairbottleneck") {
    system = new FairBottleneck(selective_update);
  } else {
    system = new MaxMin(selective_update);
  }
  return system;
}

void System::validate_solver(const std::string& solver_name)
{
  static const std::vector<std::string> opts{"bmf", "maxmin", "fairbottleneck"};
  if (solver_name == "bmf") {
#if !SIMGRID_HAVE_EIGEN3
    xbt_die("Cannot use the BMF solver without installing Eigen3.");
#endif
  }
  if (std::find(opts.begin(), opts.end(), solver_name) == std::end(opts)) {
    xbt_die("Invalid system solver, it should be one of: \"maxmin\", \"fairbottleneck\" or \"bmf\"");
  }
}

void System::check_concurrency() const
{
  // These checks are very expensive, so do them only if we want to debug SURF LMM
  if (not XBT_LOG_ISENABLED(ker_lmm, xbt_log_priority_debug))
    return;

  for (Constraint const& cnst : constraint_set) {
    int concurrency       = 0;
    for (Element const& elem : cnst.enabled_element_set_) {
      xbt_assert(elem.variable->sharing_penalty_ > 0);
      concurrency += elem.get_concurrency();
    }

    for (Element const& elem : cnst.disabled_element_set_) {
      // We should have staged variables only if concurrency is reached in some constraint
      xbt_assert(cnst.get_concurrency_limit() < 0 || elem.variable->staged_penalty_ == 0 ||
                     elem.variable->get_min_concurrency_slack() < elem.variable->concurrency_share_,
                 "should not have staged variable!");
    }

    xbt_assert(cnst.get_concurrency_limit() < 0 || cnst.get_concurrency_limit() >= concurrency,
               "concurrency check failed!");
    xbt_assert(cnst.concurrency_current_ == concurrency, "concurrency_current is out-of-date!");
  }

  // Check that for each variable, all corresponding elements are in the same state (i.e. same element sets)
  for (Variable const& var : variable_set) {
    if (var.cnsts_.empty())
      continue;

    const Element& elem    = var.cnsts_[0];
    bool belong_to_enabled  = elem.enabled_element_set_hook.is_linked();
    bool belong_to_disabled = elem.disabled_element_set_hook.is_linked();
    bool belong_to_active   = elem.active_element_set_hook.is_linked();

    for (Element const& elem2 : var.cnsts_) {
      xbt_assert(belong_to_enabled == elem2.enabled_element_set_hook.is_linked(),
                 "Variable inconsistency (1): enabled_element_set");
      xbt_assert(belong_to_disabled == elem2.disabled_element_set_hook.is_linked(),
                 "Variable inconsistency (2): disabled_element_set");
      xbt_assert(belong_to_active == elem2.active_element_set_hook.is_linked(),
                 "Variable inconsistency (3): active_element_set");
    }
  }
}

void System::var_free(Variable* var)
{
  XBT_IN("(sys=%p, var=%p)", this, var);
  modified_ = true;

  // TODOLATER Can do better than that by leaving only the variable in only one enabled_element_set, call
  // update_modified_set, and then remove it..
  update_modified_cnst_set_from_variable(var);

  for (Element& elem : var->cnsts_) {
    if (var->sharing_penalty_ > 0)
      elem.decrease_concurrency();
    if (elem.enabled_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->enabled_element_set_, elem);
    if (elem.disabled_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->disabled_element_set_, elem);
    if (elem.active_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->active_element_set_, elem);
    if (elem.constraint->enabled_element_set_.empty() && elem.constraint->disabled_element_set_.empty())
      make_constraint_inactive(elem.constraint);
    else
      on_disabled_var(elem.constraint);
  }

  var->cnsts_.clear();

  check_concurrency();

  xbt_mallocator_release(variable_mallocator_, var);
  XBT_OUT();
}

System::System(bool selective_update) : selective_update_active(selective_update)
{
  XBT_DEBUG("Setting selective_update_active flag to %d", selective_update_active);

  if (selective_update)
    modified_set_ = std::make_unique<kernel::resource::Action::ModifiedSet>();
}

System::~System()
{
  while (Variable* var = extract_variable()) {
    std::string demangled = boost::core::demangle(var->id_ ? typeid(*var->id_).name() : "(unidentified)");
    XBT_WARN("Probable bug: a %s variable (#%d) not removed before the LMM system destruction.", demangled.c_str(),
             var->rank_);
    var_free(var);
  }
  while (Constraint* cnst = extract_constraint())
    cnst_free(cnst);

  xbt_mallocator_free(variable_mallocator_);
}

void System::cnst_free(Constraint* cnst)
{
  make_constraint_inactive(cnst);
  delete cnst;
}

Constraint::Constraint(resource::Resource* id_value, double bound_value) : bound_(bound_value), id_(id_value)
{
  rank_ = next_rank_++;
}

Constraint* System::constraint_new(resource::Resource* id, double bound_value)
{
  auto* cnst = new Constraint(id, bound_value);
  insert_constraint(cnst);
  return cnst;
}

void* System::variable_mallocator_new_f()
{
  return new Variable;
}

void System::variable_mallocator_free_f(void* var)
{
  delete static_cast<Variable*>(var);
}

Variable* System::variable_new(resource::Action* id, double sharing_penalty, double bound, size_t number_of_constraints)
{
  XBT_IN("(sys=%p, id=%p, penalty=%f, bound=%f, num_cons =%zu)", this, id, sharing_penalty, bound,
         number_of_constraints);

  auto* var = static_cast<Variable*>(xbt_mallocator_get(variable_mallocator_));
  var->initialize(id, sharing_penalty, bound, number_of_constraints, visited_counter_ - 1);
  if (sharing_penalty > 0)
    variable_set.push_front(*var);
  else
    variable_set.push_back(*var);

  XBT_OUT(" returns %p", var);
  return var;
}

void System::variable_free(Variable* var)
{
  remove_variable(var);
  var_free(var);
}

void System::variable_free_all()
{
  while (Variable* var = extract_variable())
    variable_free(var);
}

void System::expand(Constraint* cnst, Variable* var, double consumption_weight)
{
  modified_ = true;

  // Check if this variable already has an active element in this constraint
  // If it does, subtract it from the required slack
  int current_share = 0;
  if (var->concurrency_share_ > 1) {
    for (const Element& elem : var->cnsts_) {
      if (elem.constraint == cnst && elem.enabled_element_set_hook.is_linked())
        current_share += elem.get_concurrency();
    }
  }

  // Check if we need to disable the variable
  if (var->sharing_penalty_ > 0 && var->concurrency_share_ - current_share > cnst->get_concurrency_slack()) {
    double penalty = var->sharing_penalty_;
    disable_var(var);
    for (Element const& elem : var->cnsts_)
      on_disabled_var(elem.constraint);
    consumption_weight = 0;
    var->staged_penalty_ = penalty;
    xbt_assert(not var->sharing_penalty_);
  }

  xbt_assert(var->cnsts_.size() < var->cnsts_.capacity(), "Too much constraints");

  var->cnsts_.emplace_back(cnst, var, consumption_weight);
  Element& elem = var->cnsts_.back();

  if (var->sharing_penalty_ != 0.0) {
    elem.constraint->enabled_element_set_.push_front(elem);
    elem.increase_concurrency();
  } else
    elem.constraint->disabled_element_set_.push_back(elem);

  if (not selective_update_active) {
    make_constraint_active(cnst);
  } else if (elem.consumption_weight > 0 || var->sharing_penalty_ > 0) {
    make_constraint_active(cnst);
    update_modified_cnst_set(cnst);
    // TODOLATER: Why do we need this second call?
    if (var->cnsts_.size() > 1)
      update_modified_cnst_set(var->cnsts_[0].constraint);
  }

  check_concurrency();
}

void System::expand_add(Constraint* cnst, Variable* var, double value)
{
  modified_ = true;

  check_concurrency();

  // BEWARE: In case you have multiple elements in one constraint, this will always add value to the first element.
  auto elem_it =
      std::find_if(begin(var->cnsts_), end(var->cnsts_), [&cnst](Element const& x) { return x.constraint == cnst; });
  if (elem_it != end(var->cnsts_)) {
    Element& elem = *elem_it;
    if (var->sharing_penalty_ != 0.0)
      elem.decrease_concurrency();

    elem.max_consumption_weight = std::max(elem.max_consumption_weight, value);
    if (cnst->sharing_policy_ != Constraint::SharingPolicy::FATPIPE)
      elem.consumption_weight += value;
    else
      elem.consumption_weight = std::max(elem.consumption_weight, value);

    // We need to check that increasing value of the element does not cross the concurrency limit
    if (var->sharing_penalty_ != 0.0) {
      if (cnst->get_concurrency_slack() < elem.get_concurrency()) {
        double penalty = var->sharing_penalty_;
        disable_var(var);
        for (Element const& elem2 : var->cnsts_)
          on_disabled_var(elem2.constraint);
        var->staged_penalty_ = penalty;
        xbt_assert(not var->sharing_penalty_);
      }
      elem.increase_concurrency();
    }
    update_modified_cnst_set(cnst);
  } else
    expand(cnst, var, value);

  check_concurrency();
}

Variable* Constraint::get_variable(const Element** elem) const
{
  if (*elem == nullptr) {
    // That is the first call, pick the first element among enabled_element_set (or disabled_element_set if
    // enabled_element_set is empty)
    if (not enabled_element_set_.empty())
      *elem = &enabled_element_set_.front();
    else if (not disabled_element_set_.empty())
      *elem = &disabled_element_set_.front();
    else
      *elem = nullptr;
  } else {
    // elem is not null, so we carry on
    if ((*elem)->enabled_element_set_hook.is_linked()) {
      // Look at enabled_element_set, and jump to disabled_element_set when finished
      auto iter = std::next(enabled_element_set_.iterator_to(**elem));
      if (iter != std::end(enabled_element_set_))
        *elem = &*iter;
      else if (not disabled_element_set_.empty())
        *elem = &disabled_element_set_.front();
      else
        *elem = nullptr;
    } else {
      auto iter = std::next(disabled_element_set_.iterator_to(**elem));
      *elem     = iter != std::end(disabled_element_set_) ? &*iter : nullptr;
    }
  }
  if (*elem)
    return (*elem)->variable;
  else
    return nullptr;
}

// if we modify the list between calls, normal version may loop forever
// this safe version ensures that we browse the list elements only once
Variable* Constraint::get_variable_safe(const Element** elem, const Element** nextelem, size_t* numelem) const
{
  if (*elem == nullptr) {
    *numelem = enabled_element_set_.size() + disabled_element_set_.size() - 1;
    if (not enabled_element_set_.empty())
      *elem = &enabled_element_set_.front();
    else if (not disabled_element_set_.empty())
      *elem = &disabled_element_set_.front();
    else
      *elem = nullptr;
  } else {
    *elem = *nextelem;
    if (*numelem > 0) {
      (*numelem)--;
    } else
      return nullptr;
  }
  if (*elem) {
    // elem is not null, so we carry on
    if ((*elem)->enabled_element_set_hook.is_linked()) {
      // Look at enabled_element_set, and jump to disabled_element_set when finished
      auto iter = std::next(enabled_element_set_.iterator_to(**elem));
      if (iter != std::end(enabled_element_set_))
        *nextelem = &*iter;
      else if (not disabled_element_set_.empty())
        *nextelem = &disabled_element_set_.front();
      else
        *nextelem = nullptr;
    } else {
      auto iter = std::next(disabled_element_set_.iterator_to(**elem));
      *nextelem = iter != std::end(disabled_element_set_) ? &*iter : nullptr;
    }
    return (*elem)->variable;
  } else
    return nullptr;
}

template <class ElemList>
static void format_element_list(const ElemList& elem_list, Constraint::SharingPolicy sharing_policy, double& sum,
                                std::string& buf)
{
  for (Element const& elem : elem_list) {
    buf += std::to_string(elem.consumption_weight) + ".'" + std::to_string(elem.variable->rank_) + "'(" +
           std::to_string(elem.variable->value_) + ")" +
           (sharing_policy != Constraint::SharingPolicy::FATPIPE ? " + " : " , ");
    if (sharing_policy != Constraint::SharingPolicy::FATPIPE)
      sum += elem.consumption_weight * elem.variable->value_;
    else
      sum = std::max(sum, elem.consumption_weight * elem.variable->value_);
  }
}

void System::print() const
{
  std::string buf = "MAX-MIN ( ";

  /* Printing Objective */
  for (Variable const& var : variable_set)
    buf += "'" + std::to_string(var.rank_) + "'(" + std::to_string(var.sharing_penalty_) + ") ";
  buf += ")";
  XBT_DEBUG("%20s", buf.c_str());
  buf.clear();

  XBT_DEBUG("Constraints");
  /* Printing Constraints */
  for (Constraint const& cnst : active_constraint_set) {
    double sum            = 0.0;
    // Show  the enabled variables
    buf += "\t";
    buf += cnst.sharing_policy_ != Constraint::SharingPolicy::FATPIPE ? "(" : "max(";
    format_element_list(cnst.enabled_element_set_, cnst.sharing_policy_, sum, buf);
    // TODO: Adding disabled elements only for test compatibility, but do we really want them to be printed?
    format_element_list(cnst.disabled_element_set_, cnst.sharing_policy_, sum, buf);

    buf += "0) <= " + std::to_string(cnst.bound_) + " ('" + std::to_string(cnst.rank_) + "')";

    if (cnst.sharing_policy_ == Constraint::SharingPolicy::FATPIPE) {
      buf += " [MAX-Constraint]";
    }
    XBT_DEBUG("%s", buf.c_str());
    buf.clear();
    xbt_assert(not double_positive(sum - cnst.bound_, cnst.bound_ * sg_maxmin_precision),
               "Incorrect value (%f is not smaller than %f): %g", sum, cnst.bound_, sum - cnst.bound_);
  }

  XBT_DEBUG("Variables");
  /* Printing Result */
  for (Variable const& var : variable_set) {
    if (var.bound_ > 0) {
      XBT_DEBUG("'%d'(%f) : %f (<=%f)", var.rank_, var.sharing_penalty_, var.value_, var.bound_);
      xbt_assert(not double_positive(var.value_ - var.bound_, var.bound_ * sg_maxmin_precision),
                 "Incorrect value (%f is not smaller than %f", var.value_, var.bound_);
    } else {
      XBT_DEBUG("'%d'(%f) : %f", var.rank_, var.sharing_penalty_, var.value_);
    }
  }
}

resource::Action::ModifiedSet* System::get_modified_action_set() const
{
  return modified_set_.get();
}

void System::solve()
{
  if (not modified_)
    return;

  do_solve();

  modified_ = false;
  if (selective_update_active) {
    /* update list of modified variables */
    for (const Constraint& cnst : modified_constraint_set) {
      for (const Element& elem : cnst.enabled_element_set_) {
        if (elem.consumption_weight > 0) {
          resource::Action* action = elem.variable->id_;
          if (not action->is_within_modified_set())
            modified_set_->push_back(*action);
        }
      }
    }
    /* clear list of modified constraint */
    remove_all_modified_cnst_set();
  }

  if (XBT_LOG_ISENABLED(ker_lmm, xbt_log_priority_debug)) {
    print();
  }

  check_concurrency();
}

/** @brief Attribute the value bound to var->bound.
 *
 *  @param var the Variable*
 *  @param bound the new bound to associate with var
 *
 *  Makes var->bound equal to bound. Whenever this function is called a change is  signed in the system. To
 *  avoid false system changing detection it is a good idea to test (bound != 0) before calling it.
 */
void System::update_variable_bound(Variable* var, double bound)
{
  modified_  = true;
  var->bound_ = bound;

  if (not var->cnsts_.empty()) {
    for (Element const& elem : var->cnsts_) {
      update_modified_cnst_set(elem.constraint);
    }
  }
}

void Variable::initialize(resource::Action* id_value, double sharing_penalty, double bound_value,
                          size_t number_of_constraints, unsigned visited_value)
{
  id_     = id_value;
  rank_   = next_rank_++;
  cnsts_.reserve(number_of_constraints);
  sharing_penalty_   = sharing_penalty;
  staged_penalty_    = 0.0;
  bound_             = bound_value;
  concurrency_share_ = 1;
  value_             = 0.0;
  visited_           = visited_value;
  mu_                = 0.0;

  xbt_assert(not variable_set_hook_.is_linked());
  xbt_assert(not saturated_variable_set_hook_.is_linked());
}

int Variable::get_min_concurrency_slack() const
{
  int minslack = std::numeric_limits<int>::max();
  for (Element const& elem : cnsts_) {
    int slack = elem.constraint->get_concurrency_slack();
    if (slack < minslack) {
      // This is only an optimization, to avoid looking at more constraints when slack is already zero
      if (slack == 0)
        return 0;
      minslack = slack;
    }
  }
  return minslack;
}

// Small remark: In this implementation of System::enable_var() and System::disable_var(), we will meet multiple times
// with var when running System::update_modified_cnst_set().
// A priori not a big performance issue, but we might do better by calling System::update_modified_cnst_set() within the
// for loops (after doing the first for enabling==1, and before doing the last for disabling==1)
void System::enable_var(Variable* var)
{
  xbt_assert(not XBT_LOG_ISENABLED(ker_lmm, xbt_log_priority_debug) || var->can_enable());

  var->sharing_penalty_ = var->staged_penalty_;
  var->staged_penalty_  = 0;

  // Enabling the variable, move var to list head. Subtlety is: here, we need to call update_modified_cnst_set AFTER
  // moving at least one element of var.

  simgrid::xbt::intrusive_erase(variable_set, *var);
  variable_set.push_front(*var);
  for (Element& elem : var->cnsts_) {
    simgrid::xbt::intrusive_erase(elem.constraint->disabled_element_set_, elem);
    elem.constraint->enabled_element_set_.push_front(elem);
    elem.increase_concurrency();
  }
  update_modified_cnst_set_from_variable(var);

  // When used within on_disabled_var, we would get an assertion fail, because transiently there can be variables
  // that are staged and could be activated.
  // Anyway, caller functions all call check_concurrency() in the end.
}

void System::disable_var(Variable* var)
{
  xbt_assert(not var->staged_penalty_, "Staged penalty should have been cleared");
  // Disabling the variable, move to var to list tail. Subtlety is: here, we need to call update_modified_cnst_set
  // BEFORE moving the last element of var.
  simgrid::xbt::intrusive_erase(variable_set, *var);
  variable_set.push_back(*var);
  update_modified_cnst_set_from_variable(var);
  for (Element& elem : var->cnsts_) {
    simgrid::xbt::intrusive_erase(elem.constraint->enabled_element_set_, elem);
    elem.constraint->disabled_element_set_.push_back(elem);
    if (elem.active_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->active_element_set_, elem);
    elem.decrease_concurrency();
  }

  var->sharing_penalty_ = 0.0;
  var->staged_penalty_  = 0.0;
  var->value_          = 0.0;
  check_concurrency();
}

/* /brief Find variables that can be enabled and enable them.
 *
 * Assuming that the variable has already been removed from non-zero penalties
 * Can we find a staged variable to add?
 * If yes, check that none of the constraints that this variable is involved in is at the limit of its concurrency
 * And then add it to enabled variables
 */
void System::on_disabled_var(Constraint* cnstr)
{
  if (cnstr->get_concurrency_limit() < 0)
    return;

  size_t numelem = cnstr->disabled_element_set_.size();
  if (numelem == 0)
    return;

  Element* elem = &cnstr->disabled_element_set_.front();

  // Cannot use foreach loop, because System::enable_var() will modify disabled_element_set.. within the loop
  while (numelem-- && elem) {
    Element* nextelem;
    if (elem->disabled_element_set_hook.is_linked()) {
      auto iter = std::next(cnstr->disabled_element_set_.iterator_to(*elem));
      nextelem  = iter != std::end(cnstr->disabled_element_set_) ? &*iter : nullptr;
    } else {
      nextelem = nullptr;
    }

    if (elem->variable->staged_penalty_ > 0 && elem->variable->can_enable()) {
      // Found a staged variable
      // TODOLATER: Add random timing function to model reservation protocol fuzziness? Then how to make sure that
      // staged variables will eventually be called?
      enable_var(elem->variable);
    }

    xbt_assert(cnstr->concurrency_current_ <= cnstr->get_concurrency_limit(), "Concurrency overflow!");
    if (cnstr->concurrency_current_ == cnstr->get_concurrency_limit())
      break;

    elem = nextelem;
  }

  // We could get an assertion fail, because transiently there can be variables that are staged and could be activated.
  // And we need to go through all constraints of the disabled var before getting back a coherent state.
  // Anyway, caller functions all call check_concurrency() in the end.
}

/** @brief update the penalty of a variable (disable it by passing 0 as a penalty) */
void System::update_variable_penalty(Variable* var, double penalty)
{
  xbt_assert(penalty >= 0, "Variable penalty should not be negative!");
  if (penalty == var->sharing_penalty_)
    return;

  bool enabling_var  = (penalty > 0 && var->sharing_penalty_ <= 0);
  bool disabling_var = (penalty <= 0 && var->sharing_penalty_ > 0);

  XBT_IN("(sys=%p, var=%p, var->sharing_penalty = %f, penalty=%f)", this, var, var->sharing_penalty_, penalty);

  modified_ = true;

  // Are we enabling this variable?
  if (enabling_var) {
    var->staged_penalty_ = penalty;
    int minslack       = var->get_min_concurrency_slack();
    if (minslack < var->concurrency_share_) {
      XBT_DEBUG("Staging var (instead of enabling) because min concurrency slack %i, with penalty %f and concurrency"
                " share %i",
                minslack, penalty, var->concurrency_share_);
      return;
    }
    XBT_DEBUG("Enabling var with min concurrency slack %i", minslack);
    enable_var(var);
  } else if (disabling_var) {
    disable_var(var);
  } else {
    var->sharing_penalty_ = penalty;
    update_modified_cnst_set_from_variable(var);
  }

  check_concurrency();

  XBT_OUT();
}

void System::update_constraint_bound(Constraint* cnst, double bound)
{
  modified_ = true;
  update_modified_cnst_set(cnst);
  cnst->bound_ = bound;
}

/** @brief Update the constraint set propagating recursively to other constraints so the system should not be entirely
 *  computed.
 *
 *  @param cnst the Constraint* affected by the change
 *
 *  A recursive algorithm to optimize the system recalculation selecting only constraints that have changed. Each
 *  constraint change is propagated to the list of constraints for each variable.
 */
void System::update_modified_cnst_set_rec(const Constraint* cnst)
{
  for (Element const& elem : cnst->enabled_element_set_) {
    Variable* var = elem.variable;
    for (Element const& elem2 : var->cnsts_) {
      if (var->visited_ == visited_counter_)
        break;
      if (elem2.constraint != cnst && not elem2.constraint->modified_constraint_set_hook_.is_linked()) {
        modified_constraint_set.push_back(*elem2.constraint);
        update_modified_cnst_set_rec(elem2.constraint);
      }
    }
    // var will be ignored in later visits as long as sys->visited_counter does not move
    var->visited_ = visited_counter_;
  }
}

void System::update_modified_cnst_set_from_variable(const Variable* var)
{
  /* nothing to update in these cases:
   * - selective update not active, all variables are active
   * - variable doesn't use any constraint
   * - variable is disabled (sharing penalty <= 0): we iterate only through the enabled_variables in
   * update_modified_cnst_set_rec */
  if (not selective_update_active || var->cnsts_.empty() || var->sharing_penalty_ <= 0)
    return;

  /* Normally, if the conditions above are true, specially variable is enabled, we can call
   * modified_set over the first contraint only, since the recursion in update_modified_cnst_set_rec
   * will iterate over the other constraints of this variable */
  update_modified_cnst_set(var->cnsts_[0].constraint);
}

void System::update_modified_cnst_set(Constraint* cnst)
{
  /* nothing to do if selective update isn't active */
  if (selective_update_active && not cnst->modified_constraint_set_hook_.is_linked()) {
    modified_constraint_set.push_back(*cnst);
    update_modified_cnst_set_rec(cnst);
  }
}

void System::remove_all_modified_cnst_set()
{
  // We cleverly un-flag all variables just by incrementing visited_counter
  // In effect, the var->visited value will no more be equal to visited counter
  // To be clean, when visited counter has wrapped around, we force these var->visited values so that variables that
  // were in the modified a long long time ago are not wrongly skipped here, which would lead to very nasty bugs
  // (i.e. not readily reproducible, and requiring a lot of run time before happening).
  if (++visited_counter_ == 1) {
    /* the counter wrapped around, reset each variable->visited */
    for (Variable& var : variable_set)
      var.visited_ = 0;
  }
  modified_constraint_set.clear();
}

/**
 * Returns resource load (in flop per second, or byte per second, or similar)
 *
 * If the resource is shared (the default case), the load is sum of resource usage made by
 * every variables located on this resource.
 *
 * If the resource is not shared (ie in FATPIPE mode), then the load is the max (not the sum)
 * of all resource usages located on this resource.
 */
double Constraint::get_usage() const
{
  double result              = 0.0;
  if (sharing_policy_ != SharingPolicy::FATPIPE) {
    for (Element const& elem : enabled_element_set_)
      if (elem.consumption_weight > 0)
        result += elem.consumption_weight * elem.variable->value_;
  } else {
    for (Element const& elem : enabled_element_set_)
      if (elem.consumption_weight > 0)
        result = std::max(result, elem.consumption_weight * elem.variable->value_);
  }
  return result;
}

int Constraint::get_variable_amount() const
{
  return static_cast<int>(std::count_if(std::begin(enabled_element_set_), std::end(enabled_element_set_),
                                        [](const Element& elem) { return elem.consumption_weight > 0; }));
}

void Constraint::set_sharing_policy(SharingPolicy policy, const s4u::NonLinearResourceCb& cb)
{
  xbt_assert(policy == SharingPolicy::NONLINEAR || not cb,
             "Invalid sharing policy for constraint. Callback should be used with NONLINEAR sharing policy");
  sharing_policy_    = policy;
  dyn_constraint_cb_ = cb;
}

} // namespace lmm
} // namespace kernel
} // namespace simgrid