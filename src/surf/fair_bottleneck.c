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
  lmm_constraint_t cnst = NULL;
  lmm_constraint_t useless_cnst = NULL;
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
    int nb=0;
    var->value = 0.0;
    for (i = 0; i < var->cnsts_number; i++) {
      if(var->cnsts[i].value==0.0) nb++;
    }
    if((nb==var->cnsts_number) && (var->weight>0.0))
      var->value = 1.0;
  }

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
  while (1) {
/*     if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) { */
/*       DEBUG0("Fair bottleneck done"); */
/*       lmm_print(sys); */
/*     } */
    DEBUG1("******* Constraints to process: %d *******", xbt_swag_size(cnst_list));
    min_usage = -1;
    xbt_swag_foreach_safe(cnst, cnst_next, cnst_list) {
      int nb = 0;
      double max_elem = 0.0;
      DEBUG1("Processing cnst %p ", cnst);
      elem_list = &(cnst->element_set);
      cnst->usage = 0.0;
      xbt_swag_foreach(elem, elem_list) {
	if (elem->variable->weight <= 0)
	  break;
	if ((elem->value > 0)) {
	  nb++;
	  if (max_elem > 0)
	    max_elem =
		MAX(max_elem, elem->value / elem->variable->weight);
	  else
	    max_elem = elem->value / elem->variable->weight;
	}
	//      make_elem_active(elem);
      }
      DEBUG2("\tmax_elem : %g with %d variables",  max_elem,nb);
      if(nb>0 && !cnst->shared)
	nb = 1;
      cnst->usage = max_elem * nb;
      DEBUG3("\tConstraint Usage %p : %f with %d variables", cnst, cnst->usage,nb);
      if(!nb) {
	xbt_swag_remove(cnst, cnst_list);
	continue;
      }

      /* Saturated constraints update */
      if (min_usage < 0 || min_usage > cnst->remaining / cnst->usage) {
	DEBUG3("Update min_usage (%g) with cnst %p -> %g",min_usage, cnst,
	       cnst->remaining / cnst->usage);

	min_usage = cnst->remaining / cnst->usage;
	while ((useless_cnst = xbt_swag_extract(&(cnst_to_update)))) {
	  xbt_swag_insert_at_head(useless_cnst, cnst_list);
	}
	xbt_swag_remove(cnst, cnst_list);
	xbt_swag_insert(cnst, &(cnst_to_update));
      } else if (min_usage == cnst->remaining / cnst->usage) {
	DEBUG2("Keep   min_usage (%g) with cnst %p",min_usage, cnst);
	xbt_swag_remove(cnst, cnst_list);
	xbt_swag_insert(cnst, &(cnst_to_update));
      } else {
	DEBUG1("\tmin_usage: %f. No update",min_usage);
      }
    }

    if (!xbt_swag_size(&cnst_to_update))
      break;

    while ((cnst_next = xbt_swag_extract(&cnst_to_update))) {
      int nb = 0;
      double remaining = cnst_next->remaining;
      elem_list = &(cnst_next->element_set);
      xbt_swag_foreach(elem, elem_list) {
	if (elem->variable->weight <= 0)
	  break;
	if ((elem->value > 0))
	  nb++;
      }
      if(nb>0 && !(cnst_next->shared))
	nb = 1;

      DEBUG1("Updating for cnst %p",cnst_next);

      xbt_swag_foreach(elem, elem_list) {
	if (elem->value <= 0) continue;

	var = elem->variable;

	if (var->weight <= 0)
	  break;
	if (var->value == 0.0) {
	  DEBUG2("\tUpdating var %p (%g)",var,var->value);
	  var->value = var->weight * remaining / (nb * elem->value);

	  /* Update usage */

	  for (i = 0; i < var->cnsts_number; i++) {
 	    lmm_element_t elm = &var->cnsts[i];
	    cnst = elm->constraint;
	    DEBUG1("\t\tUpdating cnst %p",cnst);
	    double_update(&(cnst->remaining), elm->value * var->value);
	    double_update(&(cnst->usage), elm->value / var->weight);
	    //              make_elem_inactive(elm);
	  }
	}
      }
    }
  }

  sys->modified = 0;
  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    DEBUG0("Fair bottleneck done");
    lmm_print(sys);
  }
}
