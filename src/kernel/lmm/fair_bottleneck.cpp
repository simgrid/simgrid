/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/lmm/fair_bottleneck.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/sysdep.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <xbt/utility.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_lmm);

void simgrid::kernel::lmm::FairBottleneck::do_solve()
{
  XBT_DEBUG("Variable set : %zu", variable_set.size());
  for (Variable& var : variable_set) {
    var.value_ = 0.0;
    XBT_DEBUG("Handling variable %p", &var);
    if (var.sharing_penalty_ > 0.0 && std::find_if(begin(var.cnsts_), end(var.cnsts_), [](Element const& x) {
                                        return x.consumption_weight != 0.0;
                                      }) != end(var.cnsts_)) {
      saturated_variable_set.push_back(var);
    } else {
      XBT_DEBUG("Err, finally, there is no need to take care of variable %p", &var);
      if (var.sharing_penalty_ > 0.0)
        var.value_ = 1.0;
    }
  }

  XBT_DEBUG("Active constraints : %zu", active_constraint_set.size());
  saturated_constraint_set.insert(saturated_constraint_set.end(), active_constraint_set.begin(),
                                  active_constraint_set.end());
  for (Constraint& cnst : saturated_constraint_set) {
    cnst.remaining_ = cnst.bound_;
    cnst.usage_     = 0.0;
  }

  XBT_DEBUG("Fair bottleneck Initialized");

  /*
   * Compute Usage and store the variables that reach the maximum.
   */
  auto& var_list  = saturated_variable_set;
  auto& cnst_list = saturated_constraint_set;
  do {
    if (XBT_LOG_ISENABLED(ker_lmm, xbt_log_priority_debug)) {
      XBT_DEBUG("Fair bottleneck done");
      print();
    }
    XBT_DEBUG("******* Constraints to process: %zu *******", cnst_list.size());
    for (auto iter = std::begin(cnst_list); iter != std::end(cnst_list);) {
      Constraint& cnst = *iter;
      int nb = 0;
      XBT_DEBUG("Processing cnst %p ", &cnst);
      cnst.usage_ = 0.0;
      for (const Element& elem : cnst.enabled_element_set_) {
        xbt_assert(elem.variable->sharing_penalty_ > 0);
        if (elem.consumption_weight > 0 && elem.variable->saturated_variable_set_hook_.is_linked())
          nb++;
      }
      XBT_DEBUG("\tThere are %d variables", nb);
      if (nb > 0 && cnst.sharing_policy_ == Constraint::SharingPolicy::FATPIPE)
        nb = 1;
      if (nb == 0) {
        cnst.remaining_ = 0.0;
        cnst.usage_     = 0.0;
        iter           = cnst_list.erase(iter);
      } else {
        cnst.usage_ = cnst.remaining_ / nb;
        XBT_DEBUG("\tConstraint Usage %p : %f with %d variables", &cnst, cnst.usage_, nb);
        iter++;
      }
    }

    for (auto iter = std::begin(var_list); iter != std::end(var_list);) {
      Variable& var  = *iter;
      double min_inc = DBL_MAX;
      for (Element const& elm : var.cnsts_) {
        if (elm.consumption_weight > 0)
          min_inc = std::min(min_inc, elm.constraint->usage_ / elm.consumption_weight);
      }
      if (var.bound_ > 0)
        min_inc = std::min(min_inc, var.bound_ - var.value_);
      var.mu_ = min_inc;
      XBT_DEBUG("Updating variable %p maximum increment: %g", &var, var.mu_);
      var.value_ += var.mu_;
      if (var.value_ == var.bound_)
        iter = var_list.erase(iter);
      else
        iter++;
    }

    for (auto iter = std::begin(cnst_list); iter != std::end(cnst_list);) {
      Constraint& cnst = *iter;
      XBT_DEBUG("Updating cnst %p ", &cnst);
      if (cnst.sharing_policy_ != Constraint::SharingPolicy::FATPIPE) {
        for (const Element& elem : cnst.enabled_element_set_) {
          xbt_assert(elem.variable->sharing_penalty_ > 0);
          XBT_DEBUG("\tUpdate constraint %p (%g) with variable %p by %g", &cnst, cnst.remaining_, elem.variable,
                    elem.variable->mu_);
          double_update(&cnst.remaining_, elem.consumption_weight * elem.variable->mu_, sg_maxmin_precision);
        }
      } else {
        for (const Element& elem : cnst.enabled_element_set_) {
          xbt_assert(elem.variable->sharing_penalty_ > 0);
          XBT_DEBUG("\tNon-Shared variable. Update constraint usage of %p (%g) with variable %p by %g", &cnst,
                    cnst.usage_, elem.variable, elem.variable->mu_);
          cnst.usage_ = std::min(cnst.usage_, elem.consumption_weight * elem.variable->mu_);
        }
        XBT_DEBUG("\tUpdate constraint %p (%g) by %g", &cnst, cnst.remaining_, cnst.usage_);
        double_update(&cnst.remaining_, cnst.usage_, sg_maxmin_precision);
      }

      XBT_DEBUG("\tRemaining for %p : %g", &cnst, cnst.remaining_);
      if (cnst.remaining_ <= 0.0) {
        XBT_DEBUG("\tGet rid of constraint %p", &cnst);

        iter = cnst_list.erase(iter);
        for (const Element& elem : cnst.enabled_element_set_) {
          if (elem.variable->sharing_penalty_ <= 0)
            break;
          if (elem.consumption_weight > 0 && elem.variable->saturated_variable_set_hook_.is_linked()) {
            XBT_DEBUG("\t\tGet rid of variable %p", elem.variable);
            simgrid::xbt::intrusive_erase(var_list, *elem.variable);
          }
        }
      } else {
        iter++;
      }
    }
  } while (not var_list.empty());

  cnst_list.clear();
}
