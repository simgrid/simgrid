/* 	$Id$ */

/* Copyright (c) 2007 Arnaud Legrand. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "maxmin_private.h"
#include <stdlib.h>
#include <math.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_maxmin);
#define SHOW_EXPR_G(expr) DEBUG1(#expr " = %g",expr);
#define SHOW_EXPR_D(expr) DEBUG1(#expr " = %d",expr);
#define SHOW_EXPR_P(expr) DEBUG1(#expr " = %p",expr);

void bottleneck_solve(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_variable_t var_next = NULL;
  lmm_constraint_t cnst = NULL;
  s_lmm_constraint_t s_cnst;
  lmm_constraint_t cnst_next = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  double min_usage = -1;
  int i;

  static s_xbt_swag_t cnst_to_update;

  if (!(sys->modified))
    return;

  /* Init */
  xbt_swag_init(&(cnst_to_update),
                xbt_swag_offset(s_cnst, saturated_constraint_set_hookup));

  var_list = &(sys->variable_set);
  DEBUG1("Variable set : %d", xbt_swag_size(var_list));
  xbt_swag_foreach(var, var_list) {
    int nb = 0;
    var->value = 0.0;
    DEBUG1("Handling variable %p", var);
    xbt_swag_insert(var, &(sys->saturated_variable_set));
    for (i = 0; i < var->cnsts_number; i++) {
      if (var->cnsts[i].value == 0.0)
        nb++;
    }
    if ((nb == var->cnsts_number) && (var->weight > 0.0)) {
      DEBUG1("Err, finally, there is no need to take care of variable %p",
             var);
      xbt_swag_remove(var, &(sys->saturated_variable_set));
      var->value = 1.0;
    }
    if (var->weight <= 0.0) {
      DEBUG1("Err, finally, there is no need to take care of variable %p",
             var);
      xbt_swag_remove(var, &(sys->saturated_variable_set));
    }
  }
  var_list = &(sys->saturated_variable_set);

  cnst_list = &(sys->active_constraint_set);
  DEBUG1("Active constraints : %d", xbt_swag_size(cnst_list));
  xbt_swag_foreach(cnst, cnst_list) {
    xbt_swag_insert(cnst, &(sys->saturated_constraint_set));
  }
  cnst_list = &(sys->saturated_constraint_set);
  xbt_swag_foreach(cnst, cnst_list) {
    cnst->remaining = cnst->bound;
    cnst->usage = 0.0;
  }

  DEBUG0("Fair bottleneck Initialized");

  /* 
   * Compute Usage and store the variables that reach the maximum.
   */
  do {
    if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
      DEBUG0("Fair bottleneck done");
      lmm_print(sys);
    }
    DEBUG1("******* Constraints to process: %d *******",
           xbt_swag_size(cnst_list));
    min_usage = -1;
    xbt_swag_foreach_safe(cnst, cnst_next, cnst_list) {
      int nb = 0;
      DEBUG1("Processing cnst %p ", cnst);
      elem_list = &(cnst->element_set);
      cnst->usage = 0.0;
      xbt_swag_foreach(elem, elem_list) {
        if (elem->variable->weight <= 0)
          break;
        if ((elem->value > 0) && xbt_swag_belongs(elem->variable, var_list))
          nb++;
      }
      DEBUG1("\tThere are %d variables", nb);
      if (nb > 0 && !cnst->shared)
        nb = 1;
      if (!nb) {
        cnst->remaining = 0.0;
        cnst->usage = cnst->remaining;
        xbt_swag_remove(cnst, cnst_list);
        continue;
      }
      cnst->usage = cnst->remaining / nb;
      DEBUG3("\tConstraint Usage %p : %f with %d variables", cnst,
             cnst->usage, nb);
    }

    xbt_swag_foreach_safe(var, var_next, var_list) {
      double min_inc = var->cnsts[0].constraint->usage / var->cnsts[0].value;
      for (i = 1; i < var->cnsts_number; i++) {
        lmm_element_t elm = &var->cnsts[i];
        min_inc = MIN(min_inc, elm->constraint->usage / elm->value);
      }
      if (var->bound > 0)
        min_inc = MIN(min_inc, var->bound - var->value);
      var->mu = min_inc;
      DEBUG2("Updating variable %p maximum increment: %g", var, var->mu);
      var->value += var->mu;
      if (var->value == var->bound) {
        xbt_swag_remove(var, var_list);
      }
    }

    xbt_swag_foreach_safe(cnst, cnst_next, cnst_list) {
      DEBUG1("Updating cnst %p ", cnst);
      elem_list = &(cnst->element_set);
      xbt_swag_foreach(elem, elem_list) {
        if (elem->variable->weight <= 0)
          break;
        if (cnst->shared) {
          DEBUG4("\tUpdate constraint %p (%g) with variable %p by %g",
                 cnst, cnst->remaining, elem->variable, elem->variable->mu);
          double_update(&(cnst->remaining), elem->value * elem->variable->mu);
        } else {
          DEBUG4
            ("\tNon-Shared variable. Update constraint usage of %p (%g) with variable %p by %g",
             cnst, cnst->usage, elem->variable, elem->variable->mu);
          cnst->usage = MIN(cnst->usage, elem->value * elem->variable->mu);
        }
      }
      if (!cnst->shared) {
        DEBUG3("\tUpdate constraint %p (%g) by %g",
               cnst, cnst->remaining, cnst->usage);

        double_update(&(cnst->remaining), cnst->usage);
      }

      DEBUG2("\tRemaining for %p : %g", cnst, cnst->remaining);
      if (cnst->remaining == 0.0) {
        DEBUG1("\tGet rid of constraint %p", cnst);

        xbt_swag_remove(cnst, cnst_list);
        xbt_swag_foreach(elem, elem_list) {
          if (elem->variable->weight <= 0)
            break;
          if (elem->value > 0) {
            DEBUG1("\t\tGet rid of variable %p", elem->variable);
            xbt_swag_remove(elem->variable, var_list);
          }
        }
      }
    }
  } while (xbt_swag_size(var_list));

  xbt_swag_foreach_safe(cnst, cnst_next, cnst_list) {
    xbt_swag_remove(cnst, cnst_list);
  }
  sys->modified = 0;
  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    DEBUG0("Fair bottleneck done");
    lmm_print(sys);
  }
}
