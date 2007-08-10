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

  static s_xbt_swag_t cnst_to_update;

  if (!(sys->modified))
    return;

  /* Init */
  xbt_swag_init(&(cnst_to_update),
		xbt_swag_offset(s_cnst, saturated_constraint_set_hookup));

  var_list = &(sys->variable_set);
  DEBUG1("Variable set : %d", xbt_swag_size(var_list));
  xbt_swag_foreach(var, var_list) {
    var->value = 0.0;
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

  /* 
   * Compute Usage and store the variables that reach the maximum.
   */
  while(1){
    DEBUG1("Constraints to process: %d", xbt_swag_size(cnst_list));
    xbt_swag_foreach_safe(cnst, cnst_next, cnst_list) {
      int nb = 0;
      elem_list = &(cnst->element_set);
      cnst->usage = 0.0;
      xbt_swag_foreach(elem, elem_list) {
	if(elem->variable->weight <=0) break;
	if ((elem->value > 0)) {
	  nb++;
	  if(cnst->usage>0)
	    cnst->usage = MIN(cnst->usage, elem->value / elem->variable->weight);
	  else 
	    cnst->usage = elem->value / elem->variable->weight;
	} 
	DEBUG2("Constraint Usage %p : %f",cnst,cnst->usage);
	//	make_elem_active(elem);
      }
      if(!cnst->shared) nb=1;
      cnst->usage = cnst->usage * nb;
      /* Saturated constraints update */
      if(min_usage<0 || min_usage > cnst->remaining / cnst->usage) {
	min_usage = cnst->remaining / cnst->usage;
	while ((useless_cnst = xbt_swag_extract(&(cnst_to_update)))) {
	  xbt_swag_insert_at_head(useless_cnst, cnst_list);
	}
	xbt_swag_remove(cnst, cnst_list);
	xbt_swag_insert(cnst, &(cnst_to_update));
      } else if (min_usage == cnst->remaining / cnst->usage) {
	xbt_swag_remove(cnst, cnst_list);
	xbt_swag_insert(cnst, &(cnst_to_update));
      }
    }
    
    if(!xbt_swag_size(&cnst_to_update)) break;
    
    while((cnst = xbt_swag_extract(&cnst_to_update))) {
      int nb = 0;
      xbt_swag_foreach(elem, elem_list) {
	if(elem->variable->weight <=0) break;
	if ((elem->value > 0)) nb++;
      }
      if(!cnst->shared) nb=1;

      xbt_swag_foreach(elem, elem_list) {
	var=elem->variable;
	if(var->weight <=0) break;
	if(var->value == 0.0) {
	  int i;
	  var->value = cnst->remaining / nb * var->weight /
	    elem->value;
	  
	  /* Update usage */
	  
	  for (i = 0; i < var->cnsts_number; i++) {
	    lmm_element_t elm = &var->cnsts[i];
	    cnst = elm->constraint;
	      double_update(&(cnst->remaining), elm->value * var->value);
	      double_update(&(cnst->usage), elm->value / var->weight);
	      //	      make_elem_inactive(elm);
	    } 
	  }
	}
    }
  } 

  sys->modified = 0;
  if(XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    lmm_print(sys);
  }
}

