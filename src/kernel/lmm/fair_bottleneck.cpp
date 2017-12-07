/* Copyright (c) 2007-2011, 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/lmm/maxmin.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <xbt/utility.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_maxmin);
#define SHOW_EXPR_G(expr) XBT_DEBUG(#expr " = %g", expr);
#define SHOW_EXPR_D(expr) XBT_DEBUG(#expr " = %d", expr);
#define SHOW_EXPR_P(expr) XBT_DEBUG(#expr " = %p", expr);

void simgrid::kernel::lmm::bottleneck_solve(lmm_system_t sys)
{
  if (not sys->modified)
    return;

  XBT_DEBUG("Variable set : %zu", sys->variable_set.size());
  for (Variable& var : sys->variable_set) {
    var.value = 0.0;
    XBT_DEBUG("Handling variable %p", &var);
    if (var.sharing_weight > 0.0 && std::find_if(begin(var.cnsts), end(var.cnsts), [](Element const& x) {
                                      return x.consumption_weight != 0.0;
                                    }) != end(var.cnsts)) {
      sys->saturated_variable_set.push_back(var);
    } else {
      XBT_DEBUG("Err, finally, there is no need to take care of variable %p", &var);
      if (var.sharing_weight > 0.0)
        var.value = 1.0;
    }
  }

  XBT_DEBUG("Active constraints : %zu", sys->active_constraint_set.size());
  for (Constraint& cnst : sys->active_constraint_set) {
    sys->saturated_constraint_set.push_back(cnst);
  }
  for (Constraint& cnst : sys->saturated_constraint_set) {
    cnst.remaining = cnst.bound;
    cnst.usage     = 0.0;
  }

  XBT_DEBUG("Fair bottleneck Initialized");

  /*
   * Compute Usage and store the variables that reach the maximum.
   */
  auto& var_list  = sys->saturated_variable_set;
  auto& cnst_list = sys->saturated_constraint_set;
  do {
    if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
      XBT_DEBUG("Fair bottleneck done");
      sys->print();
    }
    XBT_DEBUG("******* Constraints to process: %zu *******", cnst_list.size());
    for (auto iter = std::begin(cnst_list); iter != std::end(cnst_list);) {
      Constraint& cnst = *iter;
      int nb = 0;
      XBT_DEBUG("Processing cnst %p ", &cnst);
      cnst.usage = 0.0;
      for (Element& elem : cnst.enabled_element_set) {
        xbt_assert(elem.variable->sharing_weight > 0);
        if (elem.consumption_weight > 0 && elem.variable->saturated_variable_set_hook.is_linked())
          nb++;
      }
      XBT_DEBUG("\tThere are %d variables", nb);
      if (nb > 0 && not cnst.sharing_policy)
        nb = 1;
      if (nb == 0) {
        cnst.remaining = 0.0;
        cnst.usage     = 0.0;
        iter           = cnst_list.erase(iter);
      } else {
        cnst.usage = cnst.remaining / nb;
        XBT_DEBUG("\tConstraint Usage %p : %f with %d variables", &cnst, cnst.usage, nb);
        iter++;
      }
    }

    for (auto iter = std::begin(var_list); iter != std::end(var_list);) {
      Variable& var  = *iter;
      double min_inc = DBL_MAX;
      for (Element const& elm : var.cnsts) {
        if (elm.consumption_weight > 0)
          min_inc = std::min(min_inc, elm.constraint->usage / elm.consumption_weight);
      }
      if (var.bound > 0)
        min_inc = std::min(min_inc, var.bound - var.value);
      var.mu    = min_inc;
      XBT_DEBUG("Updating variable %p maximum increment: %g", &var, var.mu);
      var.value += var.mu;
      if (var.value == var.bound)
        iter = var_list.erase(iter);
      else
        iter++;
    }

    for (auto iter = std::begin(cnst_list); iter != std::end(cnst_list);) {
      Constraint& cnst = *iter;
      XBT_DEBUG("Updating cnst %p ", &cnst);
      if (cnst.sharing_policy) {
        for (Element& elem : cnst.enabled_element_set) {
          xbt_assert(elem.variable->sharing_weight > 0);
          XBT_DEBUG("\tUpdate constraint %p (%g) with variable %p by %g", &cnst, cnst.remaining, elem.variable,
                    elem.variable->mu);
          double_update(&cnst.remaining, elem.consumption_weight * elem.variable->mu, sg_maxmin_precision);
        }
      } else {
        for (Element& elem : cnst.enabled_element_set) {
          xbt_assert(elem.variable->sharing_weight > 0);
          XBT_DEBUG("\tNon-Shared variable. Update constraint usage of %p (%g) with variable %p by %g", &cnst,
                    cnst.usage, elem.variable, elem.variable->mu);
          cnst.usage = std::min(cnst.usage, elem.consumption_weight * elem.variable->mu);
        }
        XBT_DEBUG("\tUpdate constraint %p (%g) by %g", &cnst, cnst.remaining, cnst.usage);
        double_update(&cnst.remaining, cnst.usage, sg_maxmin_precision);
      }

      XBT_DEBUG("\tRemaining for %p : %g", &cnst, cnst.remaining);
      if (cnst.remaining <= 0.0) {
        XBT_DEBUG("\tGet rid of constraint %p", &cnst);

        iter = cnst_list.erase(iter);
        for (Element& elem : cnst.enabled_element_set) {
          if (elem.variable->sharing_weight <= 0)
            break;
          if (elem.consumption_weight > 0 && elem.variable->saturated_variable_set_hook.is_linked()) {
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
  sys->modified = true;
  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    XBT_DEBUG("Fair bottleneck done");
    sys->print();
  }
}
