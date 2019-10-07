/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/backtrace.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_maxmin, surf, "Logging specific to SURF (maxmin)");

double sg_maxmin_precision = 0.00001; /* Change this with --cfg=maxmin/precision:VALUE */
double sg_surf_precision   = 0.00001; /* Change this with --cfg=surf/precision:VALUE */
int sg_concurrency_limit   = -1;      /* Change this with --cfg=maxmin/concurrency-limit:VALUE */

namespace simgrid {
namespace kernel {
namespace lmm {

typedef std::vector<int> dyn_light_t;

int Variable::next_rank_   = 1;
int Constraint::next_rank_ = 1;

System* make_new_maxmin_system(bool selective_update)
{
  return new System(selective_update);
}

int Element::get_concurrency() const
{
  // Ignore element with weight less than one (e.g. cross-traffic)
  return (consumption_weight >= 1) ? 1 : 0;
  // There are other alternatives, but they will change the behaviour of the model..
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

void System::check_concurrency() const
{
  // These checks are very expensive, so do them only if we want to debug SURF LMM
  if (not XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug))
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
    int belong_to_enabled  = elem.enabled_element_set_hook.is_linked();
    int belong_to_disabled = elem.disabled_element_set_hook.is_linked();
    int belong_to_active   = elem.active_element_set_hook.is_linked();

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
  if (not var->cnsts_.empty())
    update_modified_set(var->cnsts_[0].constraint);

  for (Element& elem : var->cnsts_) {
    if (var->sharing_penalty_ > 0)
      elem.decrease_concurrency();
    if (elem.enabled_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->enabled_element_set_, elem);
    if (elem.disabled_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->disabled_element_set_, elem);
    if (elem.active_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->active_element_set_, elem);
    int nelements = elem.constraint->enabled_element_set_.size() + elem.constraint->disabled_element_set_.size();
    if (nelements == 0)
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
    modified_set_ = new kernel::resource::Action::ModifiedSet();
}

System::~System()
{
  Variable* var;
  Constraint* cnst;

  while ((var = extract_variable())) {
    auto demangled = simgrid::xbt::demangle(var->id_ ? typeid(*var->id_).name() : "(unidentified)");
    XBT_WARN("Probable bug: a %s variable (#%d) not removed before the LMM system destruction.", demangled.get(),
             var->rank_);
    var_free(var);
  }
  while ((cnst = extract_constraint()))
    cnst_free(cnst);

  xbt_mallocator_free(variable_mallocator_);
  delete modified_set_;
}

void System::cnst_free(Constraint* cnst)
{
  make_constraint_inactive(cnst);
  delete cnst;
}

Constraint::Constraint(resource::Resource* id_value, double bound_value) : bound_(bound_value), id_(id_value)
{
  rank_ = next_rank_++;

  remaining_           = 0.0;
  usage_               = 0.0;
  concurrency_limit_   = sg_concurrency_limit;
  concurrency_current_ = 0;
  concurrency_maximum_ = 0;
  sharing_policy_      = s4u::Link::SharingPolicy::SHARED;

  lambda_     = 0.0;
  new_lambda_ = 0.0;
  cnst_light_ = nullptr;
}

Constraint* System::constraint_new(resource::Resource* id, double bound_value)
{
  Constraint* cnst = new Constraint(id, bound_value);
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

  Variable* var = static_cast<Variable*>(xbt_mallocator_get(variable_mallocator_));
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
  Variable* var;
  while ((var = extract_variable()))
    variable_free(var);
}

void System::expand(Constraint* cnst, Variable* var, double consumption_weight)
{
  modified_ = true;

  // Check if this variable already has an active element in this constraint
  // If it does, substract it from the required slack
  int current_share = 0;
  if (var->concurrency_share_ > 1) {
    for (Element& elem : var->cnsts_) {
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

  var->cnsts_.resize(var->cnsts_.size() + 1);
  Element& elem = var->cnsts_.back();

  elem.consumption_weight = consumption_weight;
  elem.constraint         = cnst;
  elem.variable           = var;

  if (var->sharing_penalty_) {
    elem.constraint->enabled_element_set_.push_front(elem);
    elem.increase_concurrency();
  } else
    elem.constraint->disabled_element_set_.push_back(elem);

  if (not selective_update_active) {
    make_constraint_active(cnst);
  } else if (elem.consumption_weight > 0 || var->sharing_penalty_ > 0) {
    make_constraint_active(cnst);
    update_modified_set(cnst);
    // TODOLATER: Why do we need this second call?
    if (var->cnsts_.size() > 1)
      update_modified_set(var->cnsts_[0].constraint);
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
    if (var->sharing_penalty_)
      elem.decrease_concurrency();

    if (cnst->sharing_policy_ != s4u::Link::SharingPolicy::FATPIPE)
      elem.consumption_weight += value;
    else
      elem.consumption_weight = std::max(elem.consumption_weight, value);

    // We need to check that increasing value of the element does not cross the concurrency limit
    if (var->sharing_penalty_) {
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
    update_modified_set(cnst);
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
Variable* Constraint::get_variable_safe(const Element** elem, const Element** nextelem, int* numelem) const
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

static inline void saturated_constraints_update(double usage, int cnst_light_num, dyn_light_t& saturated_constraints,
                                                double* min_usage)
{
  xbt_assert(usage > 0, "Impossible");

  if (*min_usage < 0 || *min_usage > usage) {
    *min_usage = usage;
    XBT_HERE(" min_usage=%f (cnst->remaining / cnst->usage =%f)", *min_usage, usage);
    saturated_constraints.assign(1, cnst_light_num);
  } else if (*min_usage == usage) {
    saturated_constraints.emplace_back(cnst_light_num);
  }
}

static inline void saturated_variable_set_update(ConstraintLight* cnst_light_tab,
                                                 const dyn_light_t& saturated_constraints, System* sys)
{
  /* Add active variables (i.e. variables that need to be set) from the set of constraints to saturate
   * (cnst_light_tab)*/
  for (int const& saturated_cnst : saturated_constraints) {
    ConstraintLight& cnst = cnst_light_tab[saturated_cnst];
    for (Element const& elem : cnst.cnst->active_element_set_) {
      xbt_assert(elem.variable->sharing_penalty_ > 0); // All elements of active_element_set should be active
      if (elem.consumption_weight > 0 && not elem.variable->saturated_variable_set_hook_.is_linked())
        sys->saturated_variable_set.push_back(*elem.variable);
    }
  }
}

template <class ElemList>
static void format_element_list(const ElemList& elem_list, s4u::Link::SharingPolicy sharing_policy, double& sum,
                                std::string& buf)
{
  for (Element const& elem : elem_list) {
    buf += std::to_string(elem.consumption_weight) + ".'" + std::to_string(elem.variable->rank_) + "'(" +
           std::to_string(elem.variable->value_) + ")" +
           (sharing_policy != s4u::Link::SharingPolicy::FATPIPE ? " + " : " , ");
    if (sharing_policy != s4u::Link::SharingPolicy::FATPIPE)
      sum += elem.consumption_weight * elem.variable->value_;
    else
      sum = std::max(sum, elem.consumption_weight * elem.variable->value_);
  }
}

void System::print() const
{
  std::string buf = std::string("MAX-MIN ( ");

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
    buf += cnst.sharing_policy_ != s4u::Link::SharingPolicy::FATPIPE ? "(" : "max(";
    format_element_list(cnst.enabled_element_set_, cnst.sharing_policy_, sum, buf);
    // TODO: Adding disabled elements only for test compatibility, but do we really want them to be printed?
    format_element_list(cnst.disabled_element_set_, cnst.sharing_policy_, sum, buf);

    buf += "0) <= " + std::to_string(cnst.bound_) + " ('" + std::to_string(cnst.rank_) + "')";

    if (cnst.sharing_policy_ == s4u::Link::SharingPolicy::FATPIPE) {
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

void System::lmm_solve()
{
  if (modified_) {
    XBT_IN("(sys=%p)", this);
    /* Compute Usage and store the variables that reach the maximum. If selective_update_active is true, only
     * constraints that changed are considered. Otherwise all constraints with active actions are considered.
     */
    if (selective_update_active)
      lmm_solve(modified_constraint_set);
    else
      lmm_solve(active_constraint_set);
    XBT_OUT();
  }
}

template <class CnstList> void System::lmm_solve(CnstList& cnst_list)
{
  double min_usage = -1;
  double min_bound = -1;

  XBT_DEBUG("Active constraints : %zu", cnst_list.size());
  /* Init: Only modified code portions: reset the value of active variables */
  for (Constraint const& cnst : cnst_list) {
    for (Element const& elem : cnst.enabled_element_set_) {
      xbt_assert(elem.variable->sharing_penalty_ > 0.0);
      elem.variable->value_ = 0.0;
    }
  }

  ConstraintLight* cnst_light_tab = new ConstraintLight[cnst_list.size()]();
  int cnst_light_num              = 0;
  dyn_light_t saturated_constraints;

  for (Constraint& cnst : cnst_list) {
    /* INIT: Collect constraints that actually need to be saturated (i.e remaining  and usage are strictly positive)
     * into cnst_light_tab. */
    cnst.remaining_ = cnst.bound_;
    if (not double_positive(cnst.remaining_, cnst.bound_ * sg_maxmin_precision))
      continue;
    cnst.usage_ = 0;
    for (Element& elem : cnst.enabled_element_set_) {
      xbt_assert(elem.variable->sharing_penalty_ > 0);
      if (elem.consumption_weight > 0) {
        if (cnst.sharing_policy_ != s4u::Link::SharingPolicy::FATPIPE)
          cnst.usage_ += elem.consumption_weight / elem.variable->sharing_penalty_;
        else if (cnst.usage_ < elem.consumption_weight / elem.variable->sharing_penalty_)
          cnst.usage_ = elem.consumption_weight / elem.variable->sharing_penalty_;

        elem.make_active();
        resource::Action* action = static_cast<resource::Action*>(elem.variable->id_);
        if (modified_set_ && not action->is_within_modified_set())
          modified_set_->push_back(*action);
      }
    }
    XBT_DEBUG("Constraint '%d' usage: %f remaining: %f concurrency: %i<=%i<=%i", cnst.rank_, cnst.usage_,
              cnst.remaining_, cnst.concurrency_current_, cnst.concurrency_maximum_, cnst.get_concurrency_limit());
    /* Saturated constraints update */

    if (cnst.usage_ > 0) {
      cnst_light_tab[cnst_light_num].cnst                 = &cnst;
      cnst.cnst_light_                                    = &cnst_light_tab[cnst_light_num];
      cnst_light_tab[cnst_light_num].remaining_over_usage = cnst.remaining_ / cnst.usage_;
      saturated_constraints_update(cnst_light_tab[cnst_light_num].remaining_over_usage, cnst_light_num,
                                   saturated_constraints, &min_usage);
      xbt_assert(not cnst.active_element_set_.empty(),
                 "There is no sense adding a constraint that has no active element!");
      cnst_light_num++;
    }
  }

  saturated_variable_set_update(cnst_light_tab, saturated_constraints, this);

  /* Saturated variables update */
  do {
    /* Fix the variables that have to be */
    auto& var_list = saturated_variable_set;
    for (Variable const& var : var_list) {
      if (var.sharing_penalty_ <= 0.0)
        DIE_IMPOSSIBLE;
      /* First check if some of these variables could reach their upper bound and update min_bound accordingly. */
      XBT_DEBUG("var=%d, var.bound=%f, var.penalty=%f, min_usage=%f, var.bound*var.penalty=%f", var.rank_, var.bound_,
                var.sharing_penalty_, min_usage, var.bound_ * var.sharing_penalty_);
      if ((var.bound_ > 0) && (var.bound_ * var.sharing_penalty_ < min_usage)) {
        if (min_bound < 0)
          min_bound = var.bound_ * var.sharing_penalty_;
        else
          min_bound = std::min(min_bound, (var.bound_ * var.sharing_penalty_));
        XBT_DEBUG("Updated min_bound=%f", min_bound);
      }
    }

    while (not var_list.empty()) {
      Variable& var = var_list.front();
      if (min_bound < 0) {
        // If no variable could reach its bound, deal iteratively the constraints usage ( at worst one constraint is
        // saturated at each cycle)
        var.value_ = min_usage / var.sharing_penalty_;
        XBT_DEBUG("Setting var (%d) value to %f\n", var.rank_, var.value_);
      } else {
        // If there exist a variable that can reach its bound, only update it (and other with the same bound) for now.
        if (double_equals(min_bound, var.bound_ * var.sharing_penalty_, sg_maxmin_precision)) {
          var.value_ = var.bound_;
          XBT_DEBUG("Setting %p (%d) value to %f\n", &var, var.rank_, var.value_);
        } else {
          // Variables which bound is different are not considered for this cycle, but they will be afterwards.
          XBT_DEBUG("Do not consider %p (%d) \n", &var, var.rank_);
          var_list.pop_front();
          continue;
        }
      }
      XBT_DEBUG("Min usage: %f, Var(%d).penalty: %f, Var(%d).value: %f ", min_usage, var.rank_, var.sharing_penalty_,
                var.rank_, var.value_);

      /* Update the usage of contraints where this variable is involved */
      for (Element& elem : var.cnsts_) {
        Constraint* cnst = elem.constraint;
        if (cnst->sharing_policy_ != s4u::Link::SharingPolicy::FATPIPE) {
          // Remember: shared constraints require that sum(elem.value * var.value) < cnst->bound
          double_update(&(cnst->remaining_), elem.consumption_weight * var.value_, cnst->bound_ * sg_maxmin_precision);
          double_update(&(cnst->usage_), elem.consumption_weight / var.sharing_penalty_, sg_maxmin_precision);
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage_, sg_maxmin_precision) ||
              not double_positive(cnst->remaining_, cnst->bound_ * sg_maxmin_precision)) {
            if (cnst->cnst_light_) {
              int index = (cnst->cnst_light_ - cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || usage: %f remaining: %f bound: %f  ", index,
                        cnst_light_num, cnst->usage_, cnst->remaining_, cnst->bound_);
              cnst_light_tab[index]                  = cnst_light_tab[cnst_light_num - 1];
              cnst_light_tab[index].cnst->cnst_light_ = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light_ = nullptr;
            }
          } else {
            if (cnst->cnst_light_) {
              cnst->cnst_light_->remaining_over_usage = cnst->remaining_ / cnst->usage_;
            }
          }
          elem.make_inactive();
        } else {
          // Remember: non-shared constraints only require that max(elem.value * var.value) < cnst->bound
          cnst->usage_ = 0.0;
          elem.make_inactive();
          for (Element& elem2 : cnst->enabled_element_set_) {
            xbt_assert(elem2.variable->sharing_penalty_ > 0);
            if (elem2.variable->value_ > 0)
              continue;
            if (elem2.consumption_weight > 0)
              cnst->usage_ = std::max(cnst->usage_, elem2.consumption_weight / elem2.variable->sharing_penalty_);
          }
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage_, sg_maxmin_precision) ||
              not double_positive(cnst->remaining_, cnst->bound_ * sg_maxmin_precision)) {
            if (cnst->cnst_light_) {
              int index = (cnst->cnst_light_ - cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p "
                        "\t cnst_light_tab: %p usage: %f remaining: %f bound: %f  ",
                        index, cnst_light_num, cnst, cnst->cnst_light_, cnst_light_tab, cnst->usage_, cnst->remaining_,
                        cnst->bound_);
              cnst_light_tab[index]                  = cnst_light_tab[cnst_light_num - 1];
              cnst_light_tab[index].cnst->cnst_light_ = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light_ = nullptr;
            }
          } else {
            if (cnst->cnst_light_) {
              cnst->cnst_light_->remaining_over_usage = cnst->remaining_ / cnst->usage_;
              xbt_assert(not cnst->active_element_set_.empty(),
                         "Should not keep a maximum constraint that has no active"
                         " element! You want to check the maxmin precision and possible rounding effects.");
            }
          }
        }
      }
      var_list.pop_front();
    }

    /* Find out which variables reach the maximum */
    min_usage = -1;
    min_bound = -1;
    saturated_constraints.clear();
    int pos;
    for (pos = 0; pos < cnst_light_num; pos++) {
      xbt_assert(not cnst_light_tab[pos].cnst->active_element_set_.empty(),
                 "Cannot saturate more a constraint that has"
                 " no active element! You may want to change the maxmin precision (--cfg=maxmin/precision:<new_value>)"
                 " because of possible rounding effects.\n\tFor the record, the usage of this constraint is %g while "
                 "the maxmin precision to which it is compared is %g.\n\tThe usage of the previous constraint is %g.",
                 cnst_light_tab[pos].cnst->usage_, sg_maxmin_precision, cnst_light_tab[pos - 1].cnst->usage_);
      saturated_constraints_update(cnst_light_tab[pos].remaining_over_usage, pos, saturated_constraints, &min_usage);
    }

    saturated_variable_set_update(cnst_light_tab, saturated_constraints, this);

  } while (cnst_light_num > 0);

  modified_ = false;
  if (selective_update_active)
    remove_all_modified_set();

  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    print();
  }

  check_concurrency();

  delete[] cnst_light_tab;
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

  if (not var->cnsts_.empty())
    update_modified_set(var->cnsts_[0].constraint);
}

void Variable::initialize(resource::Action* id_value, double sharing_penalty, double bound_value,
                          int number_of_constraints, unsigned visited_value)
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
// with var when running System::update_modified_set().
// A priori not a big performance issue, but we might do better by calling System::update_modified_set() within the for
// loops (after doing the first for enabling==1, and before doing the last for disabling==1)
void System::enable_var(Variable* var)
{
  xbt_assert(not XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug) || var->can_enable());

  var->sharing_penalty_ = var->staged_penalty_;
  var->staged_penalty_  = 0;

  // Enabling the variable, move var to list head. Subtlety is: here, we need to call update_modified_set AFTER
  // moving at least one element of var.

  simgrid::xbt::intrusive_erase(variable_set, *var);
  variable_set.push_front(*var);
  for (Element& elem : var->cnsts_) {
    simgrid::xbt::intrusive_erase(elem.constraint->disabled_element_set_, elem);
    elem.constraint->enabled_element_set_.push_front(elem);
    elem.increase_concurrency();
  }
  if (not var->cnsts_.empty())
    update_modified_set(var->cnsts_[0].constraint);

  // When used within on_disabled_var, we would get an assertion fail, because transiently there can be variables
  // that are staged and could be activated.
  // Anyway, caller functions all call check_concurrency() in the end.
}

void System::disable_var(Variable* var)
{
  xbt_assert(not var->staged_penalty_, "Staged penalty should have been cleared");
  // Disabling the variable, move to var to list tail. Subtlety is: here, we need to call update_modified_set
  // BEFORE moving the last element of var.
  simgrid::xbt::intrusive_erase(variable_set, *var);
  variable_set.push_back(*var);
  if (not var->cnsts_.empty())
    update_modified_set(var->cnsts_[0].constraint);
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

  int numelem = cnstr->disabled_element_set_.size();
  if (not numelem)
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

  int enabling_var  = (penalty > 0 && var->sharing_penalty_ <= 0);
  int disabling_var = (penalty <= 0 && var->sharing_penalty_ > 0);

  XBT_IN("(sys=%p, var=%p, penalty=%f)", this, var, penalty);

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
  }

  check_concurrency();

  XBT_OUT();
}

void System::update_constraint_bound(Constraint* cnst, double bound)
{
  modified_ = true;
  update_modified_set(cnst);
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
void System::update_modified_set_rec(Constraint* cnst)
{
  for (Element const& elem : cnst->enabled_element_set_) {
    Variable* var = elem.variable;
    for (Element const& elem2 : var->cnsts_) {
      if (var->visited_ == visited_counter_)
        break;
      if (elem2.constraint != cnst && not elem2.constraint->modified_constraint_set_hook_.is_linked()) {
        modified_constraint_set.push_back(*elem2.constraint);
        update_modified_set_rec(elem2.constraint);
      }
    }
    // var will be ignored in later visits as long as sys->visited_counter does not move
    var->visited_ = visited_counter_;
  }
}

void System::update_modified_set(Constraint* cnst)
{
  /* nothing to do if selective update isn't active */
  if (selective_update_active && not cnst->modified_constraint_set_hook_.is_linked()) {
    modified_constraint_set.push_back(*cnst);
    update_modified_set_rec(cnst);
  }
}

void System::remove_all_modified_set()
{
  // We cleverly un-flag all variables just by incrementing visited_counter
  // In effect, the var->visited value will no more be equal to visited counter
  // To be clean, when visited counter has wrapped around, we force these var->visited values so that variables that
  // were in the modified a long long time ago are not wrongly skipped here, which would lead to very nasty bugs
  // (i.e. not readibily reproducible, and requiring a lot of run time before happening).
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
  if (sharing_policy_ != s4u::Link::SharingPolicy::FATPIPE) {
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
  return std::count_if(std::begin(enabled_element_set_), std::end(enabled_element_set_),
                       [](const Element& elem) { return elem.consumption_weight > 0; });
}
}
}
}
