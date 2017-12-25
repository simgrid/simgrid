/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file callbacks.h */

#include "src/kernel/lmm/maxmin.hpp"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "xbt/sysdep.h"
#include "xbt/utility.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cxxabi.h>
#include <limits>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_maxmin, surf, "Logging specific to SURF (maxmin)");

double sg_maxmin_precision = 0.00001; /* Change this with --cfg=maxmin/precision:VALUE */
double sg_surf_precision   = 0.00001; /* Change this with --cfg=surf/precision:VALUE */
int sg_concurrency_limit   = -1;      /* Change this with --cfg=maxmin/concurrency-limit:VALUE */

namespace simgrid {
namespace kernel {
namespace lmm {

typedef std::vector<int> dyn_light_t;

int Variable::Global_debug_id   = 1;
int Constraint::Global_debug_id = 1;

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
  xbt_assert(constraint->concurrency_current >= get_concurrency());
  constraint->concurrency_current -= get_concurrency();
}

void Element::increase_concurrency()
{
  constraint->concurrency_current += get_concurrency();

  if (constraint->concurrency_current > constraint->concurrency_maximum)
    constraint->concurrency_maximum = constraint->concurrency_current;

  xbt_assert(constraint->get_concurrency_limit() < 0 ||
                 constraint->concurrency_current <= constraint->get_concurrency_limit(),
             "Concurrency limit overflow!");
}

void System::check_concurrency() const
{
  // These checks are very expensive, so do them only if we want to debug SURF LMM
  if (not XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug))
    return;

  for (Constraint const& cnst : constraint_set) {
    int concurrency       = 0;
    for (Element const& elem : cnst.enabled_element_set) {
      xbt_assert(elem.variable->sharing_weight > 0);
      concurrency += elem.get_concurrency();
    }

    for (Element const& elem : cnst.disabled_element_set) {
      // We should have staged variables only if concurrency is reached in some constraint
      xbt_assert(cnst.get_concurrency_limit() < 0 || elem.variable->staged_weight == 0 ||
                     elem.variable->get_min_concurrency_slack() < elem.variable->concurrency_share,
                 "should not have staged variable!");
    }

    xbt_assert(cnst.get_concurrency_limit() < 0 || cnst.get_concurrency_limit() >= concurrency,
               "concurrency check failed!");
    xbt_assert(cnst.concurrency_current == concurrency, "concurrency_current is out-of-date!");
  }

  // Check that for each variable, all corresponding elements are in the same state (i.e. same element sets)
  for (Variable const& var : variable_set) {
    if (var.cnsts.empty())
      continue;

    const Element& elem    = var.cnsts[0];
    int belong_to_enabled  = elem.enabled_element_set_hook.is_linked();
    int belong_to_disabled = elem.disabled_element_set_hook.is_linked();
    int belong_to_active   = elem.active_element_set_hook.is_linked();

    for (Element const& elem2 : var.cnsts) {
      xbt_assert(belong_to_enabled == elem2.enabled_element_set_hook.is_linked(),
                 "Variable inconsistency (1): enabled_element_set");
      xbt_assert(belong_to_disabled == elem2.disabled_element_set_hook.is_linked(),
                 "Variable inconsistency (2): disabled_element_set");
      xbt_assert(belong_to_active == elem2.active_element_set_hook.is_linked(),
                 "Variable inconsistency (3): active_element_set");
    }
  }
}

void System::var_free(lmm_variable_t var)
{
  XBT_IN("(sys=%p, var=%p)", this, var);
  modified = true;

  // TODOLATER Can do better than that by leaving only the variable in only one enabled_element_set, call
  // update_modified_set, and then remove it..
  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);

  for (Element& elem : var->cnsts) {
    if (var->sharing_weight > 0)
      elem.decrease_concurrency();
    if (elem.enabled_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->enabled_element_set, elem);
    if (elem.disabled_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->disabled_element_set, elem);
    if (elem.active_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->active_element_set, elem);
    int nelements = elem.constraint->enabled_element_set.size() + elem.constraint->disabled_element_set.size();
    if (nelements == 0)
      make_constraint_inactive(elem.constraint);
    else
      on_disabled_var(elem.constraint);
  }

  var->cnsts.clear();

  check_concurrency();

  xbt_mallocator_release(variable_mallocator, var);
  XBT_OUT();
}

System::System(bool selective_update) : selective_update_active(selective_update)
{
  modified        = false;
  visited_counter = 1;

  XBT_DEBUG("Setting selective_update_active flag to %d", selective_update_active);

  keep_track          = nullptr;
  variable_mallocator =
      xbt_mallocator_new(65536, System::variable_mallocator_new_f, System::variable_mallocator_free_f, nullptr);
  solve_fun = &lmm_solve;
}

System::~System()
{
  lmm_variable_t var;
  lmm_constraint_t cnst;

  while ((var = extract_variable())) {
    auto demangled = simgrid::xbt::demangle(typeid(*var->id).name());
    XBT_WARN("Probable bug: a %s variable (#%d) not removed before the LMM system destruction.", demangled.get(),
             var->id_int);
    var_free(var);
  }
  while ((cnst = extract_constraint()))
    cnst_free(cnst);

  xbt_mallocator_free(variable_mallocator);
}

void System::cnst_free(lmm_constraint_t cnst)
{
  make_constraint_inactive(cnst);
  delete cnst;
}

Constraint::Constraint(void* id_value, double bound_value) : bound(bound_value), id(id_value)
{
  id_int = Global_debug_id++;

  remaining           = 0.0;
  usage               = 0.0;
  concurrency_limit   = sg_concurrency_limit;
  concurrency_current = 0;
  concurrency_maximum = 0;
  sharing_policy      = 1; /* FIXME: don't hardcode the value */

  lambda     = 0.0;
  new_lambda = 0.0;
  cnst_light = nullptr;
}

lmm_constraint_t System::constraint_new(void* id, double bound_value)
{
  lmm_constraint_t cnst = new Constraint(id, bound_value);
  insert_constraint(cnst);
  return cnst;
}

void* System::variable_mallocator_new_f()
{
  return new Variable;
}

void System::variable_mallocator_free_f(void* var)
{
  delete static_cast<lmm_variable_t>(var);
}

lmm_variable_t System::variable_new(simgrid::surf::Action* id, double sharing_weight, double bound,
                                    int number_of_constraints)
{
  XBT_IN("(sys=%p, id=%p, weight=%f, bound=%f, num_cons =%d)", this, id, sharing_weight, bound, number_of_constraints);

  lmm_variable_t var = static_cast<lmm_variable_t>(xbt_mallocator_get(variable_mallocator));
  var->initialize(id, sharing_weight, bound, number_of_constraints, visited_counter - 1);
  if (sharing_weight)
    variable_set.push_front(*var);
  else
    variable_set.push_back(*var);

  XBT_OUT(" returns %p", var);
  return var;
}

void System::variable_free(lmm_variable_t var)
{
  remove_variable(var);
  var_free(var);
}

void System::expand(lmm_constraint_t cnst, lmm_variable_t var, double consumption_weight)
{
  modified = true;

  // Check if this variable already has an active element in this constraint
  // If it does, substract it from the required slack
  int current_share = 0;
  if (var->concurrency_share > 1) {
    for (Element& elem : var->cnsts) {
      if (elem.constraint == cnst && elem.enabled_element_set_hook.is_linked())
        current_share += elem.get_concurrency();
    }
  }

  // Check if we need to disable the variable
  if (var->sharing_weight > 0 && var->concurrency_share - current_share > cnst->get_concurrency_slack()) {
    double weight = var->sharing_weight;
    disable_var(var);
    for (Element const& elem : var->cnsts)
      on_disabled_var(elem.constraint);
    consumption_weight = 0;
    var->staged_weight = weight;
    xbt_assert(not var->sharing_weight);
  }

  xbt_assert(var->cnsts.size() < var->cnsts.capacity(), "Too much constraints");

  var->cnsts.resize(var->cnsts.size() + 1);
  Element& elem = var->cnsts.back();

  elem.consumption_weight = consumption_weight;
  elem.constraint         = cnst;
  elem.variable           = var;

  if (var->sharing_weight) {
    elem.constraint->enabled_element_set.push_front(elem);
    elem.increase_concurrency();
  } else
    elem.constraint->disabled_element_set.push_back(elem);

  if (not selective_update_active) {
    make_constraint_active(cnst);
  } else if (elem.consumption_weight > 0 || var->sharing_weight > 0) {
    make_constraint_active(cnst);
    update_modified_set(cnst);
    // TODOLATER: Why do we need this second call?
    if (var->cnsts.size() > 1)
      update_modified_set(var->cnsts[0].constraint);
  }

  check_concurrency();
}

void System::expand_add(lmm_constraint_t cnst, lmm_variable_t var, double value)
{
  modified = true;

  check_concurrency();

  // BEWARE: In case you have multiple elements in one constraint, this will always add value to the first element.
  auto elem_it =
      std::find_if(begin(var->cnsts), end(var->cnsts), [&cnst](Element const& x) { return x.constraint == cnst; });
  if (elem_it != end(var->cnsts)) {
    Element& elem = *elem_it;
    if (var->sharing_weight)
      elem.decrease_concurrency();

    if (cnst->sharing_policy)
      elem.consumption_weight += value;
    else
      elem.consumption_weight = std::max(elem.consumption_weight, value);

    // We need to check that increasing value of the element does not cross the concurrency limit
    if (var->sharing_weight) {
      if (cnst->get_concurrency_slack() < elem.get_concurrency()) {
        double weight = var->sharing_weight;
        disable_var(var);
        for (Element const& elem2 : var->cnsts)
          on_disabled_var(elem2.constraint);
        var->staged_weight = weight;
        xbt_assert(not var->sharing_weight);
      }
      elem.increase_concurrency();
    }
    update_modified_set(cnst);
  } else
    expand(cnst, var, value);

  check_concurrency();
}

lmm_variable_t Constraint::get_variable(const_lmm_element_t* elem) const
{
  if (*elem == nullptr) {
    // That is the first call, pick the first element among enabled_element_set (or disabled_element_set if
    // enabled_element_set is empty)
    if (not enabled_element_set.empty())
      *elem = &enabled_element_set.front();
    else if (not disabled_element_set.empty())
      *elem = &disabled_element_set.front();
    else
      *elem = nullptr;
  } else {
    // elem is not null, so we carry on
    if ((*elem)->enabled_element_set_hook.is_linked()) {
      // Look at enabled_element_set, and jump to disabled_element_set when finished
      auto iter = std::next(enabled_element_set.iterator_to(**elem));
      if (iter != std::end(enabled_element_set))
        *elem = &*iter;
      else if (not disabled_element_set.empty())
        *elem = &disabled_element_set.front();
      else
        *elem = nullptr;
    } else {
      auto iter = std::next(disabled_element_set.iterator_to(**elem));
      *elem     = iter != std::end(disabled_element_set) ? &*iter : nullptr;
    }
  }
  if (*elem)
    return (*elem)->variable;
  else
    return nullptr;
}

// if we modify the list between calls, normal version may loop forever
// this safe version ensures that we browse the list elements only once
lmm_variable_t Constraint::get_variable_safe(const_lmm_element_t* elem, const_lmm_element_t* nextelem,
                                             int* numelem) const
{
  if (*elem == nullptr) {
    *numelem = enabled_element_set.size() + disabled_element_set.size() - 1;
    if (not enabled_element_set.empty())
      *elem = &enabled_element_set.front();
    else if (not disabled_element_set.empty())
      *elem = &disabled_element_set.front();
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
      auto iter = std::next(enabled_element_set.iterator_to(**elem));
      if (iter != std::end(enabled_element_set))
        *nextelem = &*iter;
      else if (not disabled_element_set.empty())
        *nextelem = &disabled_element_set.front();
      else
        *nextelem = nullptr;
    } else {
      auto iter = std::next(disabled_element_set.iterator_to(**elem));
      *nextelem = iter != std::end(disabled_element_set) ? &*iter : nullptr;
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
                                                 const dyn_light_t& saturated_constraints, lmm_system_t sys)
{
  /* Add active variables (i.e. variables that need to be set) from the set of constraints to saturate
   * (cnst_light_tab)*/
  for (int const& saturated_cnst : saturated_constraints) {
    ConstraintLight& cnst = cnst_light_tab[saturated_cnst];
    for (Element const& elem : cnst.cnst->active_element_set) {
      // Visiting active_element_set, so, by construction, should never get a zero weight, correct?
      xbt_assert(elem.variable->sharing_weight > 0);
      if (elem.consumption_weight > 0 && not elem.variable->saturated_variable_set_hook.is_linked())
        sys->saturated_variable_set.push_back(*elem.variable);
    }
  }
}

template <class ElemList>
static void format_element_list(const ElemList& elem_list, int sharing_policy, double& sum, std::string& buf)
{
  for (Element const& elem : elem_list) {
    buf += std::to_string(elem.consumption_weight) + ".'" + std::to_string(elem.variable->id_int) + "'(" +
           std::to_string(elem.variable->value) + ")" + (sharing_policy ? " + " : " , ");
    if (sharing_policy)
      sum += elem.consumption_weight * elem.variable->value;
    else
      sum = std::max(sum, elem.consumption_weight * elem.variable->value);
  }
}

void System::print() const
{
  std::string buf = std::string("MAX-MIN ( ");

  /* Printing Objective */
  for (Variable const& var : variable_set)
    buf += "'" + std::to_string(var.id_int) + "'(" + std::to_string(var.sharing_weight) + ") ";
  buf += ")";
  XBT_DEBUG("%20s", buf.c_str());
  buf.clear();

  XBT_DEBUG("Constraints");
  /* Printing Constraints */
  for (Constraint const& cnst : active_constraint_set) {
    double sum            = 0.0;
    // Show  the enabled variables
    buf += "\t";
    buf += cnst.sharing_policy ? "(" : "max(";
    format_element_list(cnst.enabled_element_set, cnst.sharing_policy, sum, buf);
    // TODO: Adding disabled elements only for test compatibility, but do we really want them to be printed?
    format_element_list(cnst.disabled_element_set, cnst.sharing_policy, sum, buf);

    buf += "0) <= " + std::to_string(cnst.bound) + " ('" + std::to_string(cnst.id_int) + "')";

    if (not cnst.sharing_policy) {
      buf += " [MAX-Constraint]";
    }
    XBT_DEBUG("%s", buf.c_str());
    buf.clear();
    xbt_assert(not double_positive(sum - cnst.bound, cnst.bound * sg_maxmin_precision),
               "Incorrect value (%f is not smaller than %f): %g", sum, cnst.bound, sum - cnst.bound);
  }

  XBT_DEBUG("Variables");
  /* Printing Result */
  for (Variable const& var : variable_set) {
    if (var.bound > 0) {
      XBT_DEBUG("'%d'(%f) : %f (<=%f)", var.id_int, var.sharing_weight, var.value, var.bound);
      xbt_assert(not double_positive(var.value - var.bound, var.bound * sg_maxmin_precision),
                 "Incorrect value (%f is not smaller than %f", var.value, var.bound);
    } else {
      XBT_DEBUG("'%d'(%f) : %f", var.id_int, var.sharing_weight, var.value);
    }
  }
}

void System::solve()
{
  if (modified) {
    XBT_IN("(sys=%p)", this);
    /* Compute Usage and store the variables that reach the maximum. If selective_update_active is true, only
     * constraints that changed are considered. Otherwise all constraints with active actions are considered.
     */
    if (selective_update_active)
      solve(modified_constraint_set);
    else
      solve(active_constraint_set);
    XBT_OUT();
  }
}

template <class CnstList> void System::solve(CnstList& cnst_list)
{
  double min_usage = -1;
  double min_bound = -1;

  XBT_DEBUG("Active constraints : %zu", cnst_list.size());
  /* Init: Only modified code portions: reset the value of active variables */
  for (Constraint const& cnst : cnst_list) {
    for (Element const& elem : cnst.enabled_element_set) {
      xbt_assert(elem.variable->sharing_weight > 0.0);
      elem.variable->value = 0.0;
    }
  }

  ConstraintLight* cnst_light_tab = new ConstraintLight[cnst_list.size()]();
  int cnst_light_num              = 0;
  dyn_light_t saturated_constraints;

  for (Constraint& cnst : cnst_list) {
    /* INIT: Collect constraints that actually need to be saturated (i.e remaining  and usage are strictly positive)
     * into cnst_light_tab. */
    cnst.remaining = cnst.bound;
    if (not double_positive(cnst.remaining, cnst.bound * sg_maxmin_precision))
      continue;
    cnst.usage = 0;
    for (Element& elem : cnst.enabled_element_set) {
      xbt_assert(elem.variable->sharing_weight > 0);
      if (elem.consumption_weight > 0) {
        if (cnst.sharing_policy)
          cnst.usage += elem.consumption_weight / elem.variable->sharing_weight;
        else if (cnst.usage < elem.consumption_weight / elem.variable->sharing_weight)
          cnst.usage = elem.consumption_weight / elem.variable->sharing_weight;

        elem.make_active();
        simgrid::surf::Action* action = static_cast<simgrid::surf::Action*>(elem.variable->id);
        if (keep_track && not action->is_linked())
          keep_track->push_back(*action);
      }
    }
    XBT_DEBUG("Constraint '%d' usage: %f remaining: %f concurrency: %i<=%i<=%i", cnst.id_int, cnst.usage,
              cnst.remaining, cnst.concurrency_current, cnst.concurrency_maximum, cnst.get_concurrency_limit());
    /* Saturated constraints update */

    if (cnst.usage > 0) {
      cnst_light_tab[cnst_light_num].cnst                 = &cnst;
      cnst.cnst_light                                     = &cnst_light_tab[cnst_light_num];
      cnst_light_tab[cnst_light_num].remaining_over_usage = cnst.remaining / cnst.usage;
      saturated_constraints_update(cnst_light_tab[cnst_light_num].remaining_over_usage, cnst_light_num,
                                   saturated_constraints, &min_usage);
      xbt_assert(not cnst.active_element_set.empty(),
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
      if (var.sharing_weight <= 0.0)
        DIE_IMPOSSIBLE;
      /* First check if some of these variables could reach their upper bound and update min_bound accordingly. */
      XBT_DEBUG("var=%d, var.bound=%f, var.weight=%f, min_usage=%f, var.bound*var.weight=%f", var.id_int, var.bound,
                var.sharing_weight, min_usage, var.bound * var.sharing_weight);
      if ((var.bound > 0) && (var.bound * var.sharing_weight < min_usage)) {
        if (min_bound < 0)
          min_bound = var.bound * var.sharing_weight;
        else
          min_bound = std::min(min_bound, (var.bound * var.sharing_weight));
        XBT_DEBUG("Updated min_bound=%f", min_bound);
      }
    }

    while (not var_list.empty()) {
      Variable& var = var_list.front();
      if (min_bound < 0) {
        // If no variable could reach its bound, deal iteratively the constraints usage ( at worst one constraint is
        // saturated at each cycle)
        var.value = min_usage / var.sharing_weight;
        XBT_DEBUG("Setting var (%d) value to %f\n", var.id_int, var.value);
      } else {
        // If there exist a variable that can reach its bound, only update it (and other with the same bound) for now.
        if (double_equals(min_bound, var.bound * var.sharing_weight, sg_maxmin_precision)) {
          var.value = var.bound;
          XBT_DEBUG("Setting %p (%d) value to %f\n", &var, var.id_int, var.value);
        } else {
          // Variables which bound is different are not considered for this cycle, but they will be afterwards.
          XBT_DEBUG("Do not consider %p (%d) \n", &var, var.id_int);
          var_list.pop_front();
          continue;
        }
      }
      XBT_DEBUG("Min usage: %f, Var(%d).weight: %f, Var(%d).value: %f ", min_usage, var.id_int, var.sharing_weight,
                var.id_int, var.value);

      /* Update the usage of contraints where this variable is involved */
      for (Element& elem : var.cnsts) {
        lmm_constraint_t cnst = elem.constraint;
        if (cnst->sharing_policy) {
          // Remember: shared constraints require that sum(elem.value * var.value) < cnst->bound
          double_update(&(cnst->remaining), elem.consumption_weight * var.value, cnst->bound * sg_maxmin_precision);
          double_update(&(cnst->usage), elem.consumption_weight / var.sharing_weight, sg_maxmin_precision);
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage, sg_maxmin_precision) ||
              not double_positive(cnst->remaining, cnst->bound * sg_maxmin_precision)) {
            if (cnst->cnst_light) {
              int index = (cnst->cnst_light - cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || usage: %f remaining: %f bound: %f  ", index,
                        cnst_light_num, cnst->usage, cnst->remaining, cnst->bound);
              cnst_light_tab[index]                  = cnst_light_tab[cnst_light_num - 1];
              cnst_light_tab[index].cnst->cnst_light = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light = nullptr;
            }
          } else {
            cnst->cnst_light->remaining_over_usage = cnst->remaining / cnst->usage;
          }
          elem.make_inactive();
        } else {
          // Remember: non-shared constraints only require that max(elem.value * var.value) < cnst->bound
          cnst->usage = 0.0;
          elem.make_inactive();
          for (Element& elem2 : cnst->enabled_element_set) {
            xbt_assert(elem2.variable->sharing_weight > 0);
            if (elem2.variable->value > 0)
              continue;
            if (elem2.consumption_weight > 0)
              cnst->usage = std::max(cnst->usage, elem2.consumption_weight / elem2.variable->sharing_weight);
          }
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage, sg_maxmin_precision) ||
              not double_positive(cnst->remaining, cnst->bound * sg_maxmin_precision)) {
            if (cnst->cnst_light) {
              int index = (cnst->cnst_light - cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p "
                        "\t cnst_light_tab: %p usage: %f remaining: %f bound: %f  ",
                        index, cnst_light_num, cnst, cnst->cnst_light, cnst_light_tab, cnst->usage, cnst->remaining,
                        cnst->bound);
              cnst_light_tab[index]                  = cnst_light_tab[cnst_light_num - 1];
              cnst_light_tab[index].cnst->cnst_light = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light = nullptr;
            }
          } else {
            cnst->cnst_light->remaining_over_usage = cnst->remaining / cnst->usage;
            xbt_assert(not cnst->active_element_set.empty(),
                       "Should not keep a maximum constraint that has no active"
                       " element! You want to check the maxmin precision and possible rounding effects.");
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
      xbt_assert(not cnst_light_tab[pos].cnst->active_element_set.empty(),
                 "Cannot saturate more a constraint that has"
                 " no active element! You may want to change the maxmin precision (--cfg=maxmin/precision:<new_value>)"
                 " because of possible rounding effects.\n\tFor the record, the usage of this constraint is %g while "
                 "the maxmin precision to which it is compared is %g.\n\tThe usage of the previous constraint is %g.",
                 cnst_light_tab[pos].cnst->usage, sg_maxmin_precision, cnst_light_tab[pos - 1].cnst->usage);
      saturated_constraints_update(cnst_light_tab[pos].remaining_over_usage, pos, saturated_constraints, &min_usage);
    }

    saturated_variable_set_update(cnst_light_tab, saturated_constraints, this);

  } while (cnst_light_num > 0);

  modified = false;
  if (selective_update_active)
    remove_all_modified_set();

  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    print();
  }

  check_concurrency();

  delete[] cnst_light_tab;
}

void lmm_solve(lmm_system_t sys)
{
  sys->solve();
}

/** \brief Attribute the value bound to var->bound.
 *
 *  \param var the lmm_variable_t
 *  \param bound the new bound to associate with var
 *
 *  Makes var->bound equal to bound. Whenever this function is called a change is  signed in the system. To
 *  avoid false system changing detection it is a good idea to test (bound != 0) before calling it.
 */
void System::update_variable_bound(lmm_variable_t var, double bound)
{
  modified   = true;
  var->bound = bound;

  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);
}

void Variable::initialize(simgrid::surf::Action* id_value, double sharing_weight_value, double bound_value,
                          int number_of_constraints, unsigned visited_value)
{
  id     = id_value;
  id_int = Global_debug_id++;
  cnsts.reserve(number_of_constraints);
  sharing_weight    = sharing_weight_value;
  staged_weight     = 0.0;
  bound             = bound_value;
  concurrency_share = 1;
  value             = 0.0;
  visited           = visited_value;
  mu                = 0.0;
  new_mu            = 0.0;
  func_f            = func_f_def;
  func_fp           = func_fp_def;
  func_fpi          = func_fpi_def;

  xbt_assert(not variable_set_hook.is_linked());
  xbt_assert(not saturated_variable_set_hook.is_linked());
}

int Variable::get_min_concurrency_slack() const
{
  int minslack = std::numeric_limits<int>::max();
  for (Element const& elem : cnsts) {
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
void System::enable_var(lmm_variable_t var)
{
  xbt_assert(not XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug) || var->can_enable());

  var->sharing_weight = var->staged_weight;
  var->staged_weight  = 0;

  // Enabling the variable, move var to list head. Subtlety is: here, we need to call update_modified_set AFTER
  // moving at least one element of var.

  simgrid::xbt::intrusive_erase(variable_set, *var);
  variable_set.push_front(*var);
  for (Element& elem : var->cnsts) {
    simgrid::xbt::intrusive_erase(elem.constraint->disabled_element_set, elem);
    elem.constraint->enabled_element_set.push_front(elem);
    elem.increase_concurrency();
  }
  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);

  // When used within on_disabled_var, we would get an assertion fail, because transiently there can be variables
  // that are staged and could be activated.
  // Anyway, caller functions all call check_concurrency() in the end.
}

void System::disable_var(lmm_variable_t var)
{
  xbt_assert(not var->staged_weight, "Staged weight should have been cleared");
  // Disabling the variable, move to var to list tail. Subtlety is: here, we need to call update_modified_set
  // BEFORE moving the last element of var.
  simgrid::xbt::intrusive_erase(variable_set, *var);
  variable_set.push_back(*var);
  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);
  for (Element& elem : var->cnsts) {
    simgrid::xbt::intrusive_erase(elem.constraint->enabled_element_set, elem);
    elem.constraint->disabled_element_set.push_back(elem);
    if (elem.active_element_set_hook.is_linked())
      simgrid::xbt::intrusive_erase(elem.constraint->active_element_set, elem);
    elem.decrease_concurrency();
  }

  var->sharing_weight = 0.0;
  var->staged_weight  = 0.0;
  var->value          = 0.0;
  check_concurrency();
}

/* /brief Find variables that can be enabled and enable them.
 *
 * Assuming that the variable has already been removed from non-zero weights
 * Can we find a staged variable to add?
 * If yes, check that none of the constraints that this variable is involved in is at the limit of its concurrency
 * And then add it to enabled variables
 */
void System::on_disabled_var(lmm_constraint_t cnstr)
{
  if (cnstr->get_concurrency_limit() < 0)
    return;

  int numelem = cnstr->disabled_element_set.size();
  if (not numelem)
    return;

  lmm_element_t elem = &cnstr->disabled_element_set.front();

  // Cannot use foreach loop, because System::enable_var() will modify disabled_element_set.. within the loop
  while (numelem-- && elem) {

    lmm_element_t nextelem;
    if (elem->disabled_element_set_hook.is_linked()) {
      auto iter = std::next(cnstr->disabled_element_set.iterator_to(*elem));
      nextelem  = iter != std::end(cnstr->disabled_element_set) ? &*iter : nullptr;
    } else {
      nextelem = nullptr;
    }

    if (elem->variable->staged_weight > 0 && elem->variable->can_enable()) {
      // Found a staged variable
      // TODOLATER: Add random timing function to model reservation protocol fuzziness? Then how to make sure that
      // staged variables will eventually be called?
      enable_var(elem->variable);
    }

    xbt_assert(cnstr->concurrency_current <= cnstr->get_concurrency_limit(), "Concurrency overflow!");
    if (cnstr->concurrency_current == cnstr->get_concurrency_limit())
      break;

    elem = nextelem;
  }

  // We could get an assertion fail, because transiently there can be variables that are staged and could be activated.
  // And we need to go through all constraints of the disabled var before getting back a coherent state.
  // Anyway, caller functions all call check_concurrency() in the end.
}

/* \brief update the weight of a variable, and enable/disable it.
 * @return Returns whether a change was made
 */
void System::update_variable_weight(lmm_variable_t var, double weight)
{
  xbt_assert(weight >= 0, "Variable weight should not be negative!");

  if (weight == var->sharing_weight)
    return;

  int enabling_var  = (weight > 0 && var->sharing_weight <= 0);
  int disabling_var = (weight <= 0 && var->sharing_weight > 0);

  XBT_IN("(sys=%p, var=%p, weight=%f)", this, var, weight);

  modified = true;

  // Are we enabling this variable?
  if (enabling_var) {
    var->staged_weight = weight;
    int minslack       = var->get_min_concurrency_slack();
    if (minslack < var->concurrency_share) {
      XBT_DEBUG("Staging var (instead of enabling) because min concurrency slack %i, with weight %f and concurrency"
                " share %i",
                minslack, weight, var->concurrency_share);
      return;
    }
    XBT_DEBUG("Enabling var with min concurrency slack %i", minslack);
    enable_var(var);
  } else if (disabling_var) {
    // Are we disabling this variable?
    disable_var(var);
  } else {
    var->sharing_weight = weight;
  }

  check_concurrency();

  XBT_OUT();
}

void System::update_constraint_bound(lmm_constraint_t cnst, double bound)
{
  modified = true;
  update_modified_set(cnst);
  cnst->bound = bound;
}

/** \brief Update the constraint set propagating recursively to other constraints so the system should not be entirely
 *  computed.
 *
 *  \param cnst the lmm_constraint_t affected by the change
 *
 *  A recursive algorithm to optimize the system recalculation selecting only constraints that have changed. Each
 *  constraint change is propagated to the list of constraints for each variable.
 */
void System::update_modified_set_rec(lmm_constraint_t cnst)
{
  for (Element const& elem : cnst->enabled_element_set) {
    lmm_variable_t var = elem.variable;
    for (Element const& elem2 : var->cnsts) {
      if (var->visited == visited_counter)
        break;
      if (elem2.constraint != cnst && not elem2.constraint->modified_constraint_set_hook.is_linked()) {
        modified_constraint_set.push_back(*elem2.constraint);
        update_modified_set_rec(elem2.constraint);
      }
    }
    // var will be ignored in later visits as long as sys->visited_counter does not move
    var->visited = visited_counter;
  }
}

void System::update_modified_set(lmm_constraint_t cnst)
{
  /* nothing to do if selective update isn't active */
  if (selective_update_active && not cnst->modified_constraint_set_hook.is_linked()) {
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
  if (++visited_counter == 1) {
    /* the counter wrapped around, reset each variable->visited */
    for (Variable& var : variable_set)
      var.visited = 0;
  }
  modified_constraint_set.clear();
}

/**
 * Returns resource load (in flop per second, or byte per second, or similar)
 *
 * If the resource is shared (the default case), the load is sum of resource usage made by every variables located on
 * this resource.
 *
 * If the resource is not shared (ie in FATPIPE mode), then the load is the max (not the sum) of all resource usages
 * located on this resource.
 */
double Constraint::get_usage() const
{
  double result              = 0.0;
  if (sharing_policy) {
    for (Element const& elem : enabled_element_set)
      if (elem.consumption_weight > 0)
        result += elem.consumption_weight * elem.variable->value;
  } else {
    for (Element const& elem : enabled_element_set)
      if (elem.consumption_weight > 0)
        result = std::max(result, elem.consumption_weight * elem.variable->value);
  }
  return result;
}

int Constraint::get_variable_amount() const
{
  return std::count_if(std::begin(enabled_element_set), std::end(enabled_element_set),
                       [](const Element& elem) { return elem.consumption_weight > 0; });
}
}
}
}
