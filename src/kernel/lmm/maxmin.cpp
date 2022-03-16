/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/lmm/maxmin.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_lmm);

namespace simgrid {
namespace kernel {
namespace lmm {

using dyn_light_t = std::vector<int>;

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

static inline void saturated_variable_set_update(const ConstraintLight* cnst_light_tab,
                                                 const dyn_light_t& saturated_constraints, System* sys)
{
  /* Add active variables (i.e. variables that need to be set) from the set of constraints to saturate
   * (cnst_light_tab)*/
  for (int const& saturated_cnst : saturated_constraints) {
    const ConstraintLight& cnst = cnst_light_tab[saturated_cnst];
    for (Element const& elem : cnst.cnst->active_element_set_) {
      xbt_assert(elem.variable->sharing_penalty_ > 0); // All elements of active_element_set should be active
      if (elem.consumption_weight > 0 && not elem.variable->saturated_variable_set_hook_.is_linked())
        sys->saturated_variable_set.push_back(*elem.variable);
    }
  }
}

void MaxMin::do_solve()
{
  XBT_IN("(sys=%p)", this);
  /* Compute Usage and store the variables that reach the maximum. If selective_update_active is true, only
   * constraints that changed are considered. Otherwise all constraints with active actions are considered.
   */
  if (selective_update_active)
    maxmin_solve(modified_constraint_set);
  else
    maxmin_solve(active_constraint_set);
  XBT_OUT();
}

template <class CnstList> void MaxMin::maxmin_solve(CnstList& cnst_list)
{
  double min_usage = -1;
  double min_bound = -1;

  XBT_DEBUG("Active constraints : %zu", cnst_list.size());
  cnst_light_vec.reserve(cnst_list.size());
  ConstraintLight* cnst_light_tab = cnst_light_vec.data();
  int cnst_light_num              = 0;

  for (Constraint& cnst : cnst_list) {
    /* INIT: Collect constraints that actually need to be saturated (i.e remaining  and usage are strictly positive)
     * into cnst_light_tab. */
    cnst.dynamic_bound_ = cnst.bound_;
    if (cnst.get_sharing_policy() == Constraint::SharingPolicy::NONLINEAR && cnst.dyn_constraint_cb_) {
      cnst.dynamic_bound_ = cnst.dyn_constraint_cb_(cnst.bound_, cnst.concurrency_current_);
    }
    cnst.remaining_ = cnst.dynamic_bound_;
    if (not double_positive(cnst.remaining_, cnst.dynamic_bound_ * sg_maxmin_precision))
      continue;
    cnst.usage_ = 0;
    for (Element& elem : cnst.enabled_element_set_) {
      xbt_assert(elem.variable->sharing_penalty_ > 0.0);
      elem.variable->value_ = 0.0;
      if (elem.consumption_weight > 0) {
        if (cnst.sharing_policy_ != Constraint::SharingPolicy::FATPIPE)
          cnst.usage_ += elem.consumption_weight / elem.variable->sharing_penalty_;
        else if (cnst.usage_ < elem.consumption_weight / elem.variable->sharing_penalty_)
          cnst.usage_ = elem.consumption_weight / elem.variable->sharing_penalty_;

        elem.make_active();
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
          XBT_DEBUG("Do not consider %p (%d)\n", &var, var.rank_);
          var_list.pop_front();
          continue;
        }
      }
      XBT_DEBUG("Min usage: %f, Var(%d).penalty: %f, Var(%d).value: %f", min_usage, var.rank_, var.sharing_penalty_,
                var.rank_, var.value_);

      /* Update the usage of constraints where this variable is involved */
      for (Element& elem : var.cnsts_) {
        Constraint* cnst = elem.constraint;
        if (cnst->sharing_policy_ != Constraint::SharingPolicy::FATPIPE) {
          // Remember: shared constraints require that sum(elem.value * var.value) < cnst->bound
          double_update(&(cnst->remaining_), elem.consumption_weight * var.value_,
                        cnst->dynamic_bound_ * sg_maxmin_precision);
          double_update(&(cnst->usage_), elem.consumption_weight / var.sharing_penalty_, sg_maxmin_precision);
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage_, sg_maxmin_precision) ||
              not double_positive(cnst->remaining_, cnst->dynamic_bound_ * sg_maxmin_precision)) {
            if (cnst->cnst_light_) {
              size_t index = (cnst->cnst_light_ - cnst_light_tab);
              XBT_DEBUG("index: %zu \t cnst_light_num: %d \t || usage: %f remaining: %f bound: %f", index,
                        cnst_light_num, cnst->usage_, cnst->remaining_, cnst->dynamic_bound_);
              cnst_light_tab[index]                   = cnst_light_tab[cnst_light_num - 1];
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
          for (const Element& elem2 : cnst->enabled_element_set_) {
            xbt_assert(elem2.variable->sharing_penalty_ > 0);
            if (elem2.variable->value_ > 0)
              continue;
            if (elem2.consumption_weight > 0)
              cnst->usage_ = std::max(cnst->usage_, elem2.consumption_weight / elem2.variable->sharing_penalty_);
          }
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage_, sg_maxmin_precision) ||
              not double_positive(cnst->remaining_, cnst->dynamic_bound_ * sg_maxmin_precision)) {
            if (cnst->cnst_light_) {
              size_t index = (cnst->cnst_light_ - cnst_light_tab);
              XBT_DEBUG("index: %zu \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p "
                        "\t cnst_light_tab: %p usage: %f remaining: %f bound: %f",
                        index, cnst_light_num, cnst, cnst->cnst_light_, cnst_light_tab, cnst->usage_, cnst->remaining_,
                        cnst->dynamic_bound_);
              cnst_light_tab[index]                   = cnst_light_tab[cnst_light_num - 1];
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
    for (int pos = 0; pos < cnst_light_num; pos++) {
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
}

} // namespace lmm
} // namespace kernel
} // namespace simgrid