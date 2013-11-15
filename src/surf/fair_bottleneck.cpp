/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "solver.hpp"
#include <stdlib.h>
#include <math.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_maxmin);
#define SHOW_EXPR_G(expr) XBT_DEBUG(#expr " = %g",expr);
#define SHOW_EXPR_D(expr) XBT_DEBUG(#expr " = %d",expr);
#define SHOW_EXPR_P(expr) XBT_DEBUG(#expr " = %p",expr);

void bottleneck_solve(lmm_system_t sys)
{
  lmm_variable_t var_next = NULL;
  lmm_constraint_t cnst = NULL;
  //s_lmm_constraint_t s_cnst;
  lmm_constraint_t cnst_next = NULL;
  xbt_swag_t cnst_list = NULL;
  
  xbt_swag_t elem_list = NULL;
  int i;

  static s_xbt_swag_t cnst_to_update;
  vector<ConstraintPtr> cnstToUpdate;

  if (!(lmm_system_modified(sys)))
    return;

  /* Init */

  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;  
  std::vector<VariablePtr> *varList; 
  std::vector<VariablePtr>::iterator varIt;
  std::vector<ElementPtr> *elemList;  
  std::vector<ElementPtr>::iterator elemIt;
  std::vector<ConstraintPtr> *cnstList;
  std::vector<ConstraintPtr>::iterator cnstIt;

  varList = &(sys->m_variableSet);  
  XBT_DEBUG("Variable set : %d", varList->size());
  for (varIt=varList->begin(); varIt!=varList->end(); ++varIt) {
    int nb = 0;
    var = *varIt;
    var->m_value = 0.0;
    XBT_DEBUG("Handling variable %p", var);
    sys->m_saturatedVariableSet.push_back(var);
    for (elemIt=var->m_cnsts.begin(); elemIt!=var->m_cnsts.end(); ++elemIt) 
      if ((*elemIt)->m_value == 0.0)
	nb++; 
    if ((nb == var->getNumberOfCnst()) && (var->m_weight > 0.0)) {
      XBT_DEBUG("Err, finally, there is no need to take care of variable %p",
               var);
      sys->m_saturatedVariableSet.erase(std::find(sys->m_saturatedVariableSet.begin(), sys->m_saturatedVariableSet.end(), var));
      var->m_value = 1.0;
    }
    if (var->m_weight <= 0.0) {
      XBT_DEBUG("Err, finally, there is no need to take care of variable %p",
             var);
      sys->m_saturatedVariableSet.erase(std::find(sys->m_saturatedVariableSet.begin(), sys->m_saturatedVariableSet.end(), var));      
    }
  }
  varList = &(sys->m_saturatedVariableSet);
  cnstList = &(sys->m_activeConstraintSet);
  XBT_DEBUG("Active constraints : %d", cnstList->size());
  sys->m_saturatedConstraintSet.insert(sys->m_saturatedConstraintSet.end(), cnstList->begin(), cnstList->end());
  cnstList = &(sys->m_saturatedConstraintSet);
  for(cnstIt=cnstList->begin(); cnstIt!=cnstList->end(); ++cnstIt) {
    (*cnstIt)->m_remaining = (*cnstIt)->m_bound;
    (*cnstIt)->m_usage = 0.0;
  }
  XBT_DEBUG("Fair bottleneck Initialized");

  /* 
   * Compute Usage and store the variables that reach the maximum.
   */

  do {
    if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
      XBT_DEBUG("Fair bottleneck done");
      lmm_print(sys);
    }
    XBT_DEBUG("******* Constraints to process: %d *******",
           cnstList->size());
    for (cnstIt=cnstList->begin(); cnstIt!=cnstList->end();){
      cnst = *cnstIt;
      int nb = 0;
      XBT_DEBUG("Processing cnst %p ", cnst);
      elemList = &(cnst->m_elementSet);
      cnst->m_usage = 0.0;
      for(elemIt=elemList->begin(); elemIt!=elemList->end(); elemIt++) {
        if (elem->p_variable->m_weight <= 0)
          break;
        if ((elem->m_value > 0)
            && std::find(varList->begin(), varList->end(), elem->p_variable)!=varList->end());
          nb++;
      }
      XBT_DEBUG("\tThere are %d variables", nb);
      if (nb > 0 && !cnst->m_shared)
        nb = 1;
      if (!nb) {
        cnst->m_remaining = 0.0;
        cnst->m_usage = cnst->m_remaining;
        cnstList->erase(std::find(cnstList->begin(), cnstList->end(), *cnstIt));	
        continue;
      }
      cnst->m_usage = cnst->m_remaining / nb;
      XBT_DEBUG("\tConstraint Usage %p : %f with %d variables", cnst,
             cnst->m_usage, nb);
      ++cnstIt;      
    }

    for (varIt=varList->begin(); varIt!=varList->end();){
      var = *varIt;
      double minInc =
          var->m_cnsts.front()->p_constraint->m_usage / var->m_cnsts.front()->m_value;
      for (elemIt=++var->m_cnsts.begin(); elemIt!=var->m_cnsts.end(); ++elemIt) {
	elem = (*elemIt);
        minInc = MIN(minInc, elem->p_constraint->m_usage / elem->m_value);
      }
      if (var->m_bound > 0)
        minInc = MIN(minInc, var->m_bound - var->m_value);
      var->m_mu = minInc;
      XBT_DEBUG("Updating variable %p maximum increment: %g", var, var->m_mu);
      var->m_value += var->m_mu;
      if (var->m_value == var->m_bound) {
        varList->erase(std::find(varList->begin(), varList->end(), var));	
      } else 
	++varIt;
    }

    for (cnstIt=cnstList->begin(); cnstIt!=cnstList->end();) {
      cnst = *cnstIt;
      XBT_DEBUG("Updating cnst %p ", cnst);
      elemList = &(cnst->m_elementSet);
      for (elemIt=elemList->begin(); elemIt!=elemList->end();++elemIt) {
	elem = *elemIt;
        if (elem->p_variable->m_weight <= 0)
          break;
        if (cnst->m_shared) {
          XBT_DEBUG("\tUpdate constraint %p (%g) with variable %p by %g",
                 cnst, cnst->m_remaining, elem->p_variable,
                 elem->p_variable->m_mu);
          double_update(&(cnst->m_remaining),
                        elem->m_value * elem->p_variable->m_mu);
        } else {
          XBT_DEBUG
              ("\tNon-Shared variable. Update constraint usage of %p (%g) with variable %p by %g",
               cnst, cnst->m_usage, elem->p_variable, elem->p_variable->m_mu);
          cnst->m_usage = MIN(cnst->m_usage, elem->m_value * elem->p_variable->m_mu);
        }
      }

      if (!cnst->m_shared) {
        XBT_DEBUG("\tUpdate constraint %p (%g) by %g",
               cnst, cnst->m_remaining, cnst->m_usage);

        double_update(&(cnst->m_remaining), cnst->m_usage);
      }

      XBT_DEBUG("\tRemaining for %p : %g", cnst, cnst->m_remaining);
      if (cnst->m_remaining == 0.0) {
        XBT_DEBUG("\tGet rid of constraint %p", cnst);

        cnstList->erase(std::find(cnstList->begin(), cnstList->end(), cnst));

        for (elemIt=elemList->begin(); elemIt!=elemList->end();++elemIt) {
          if (elem->p_variable->m_weight <= 0)
            break;
          if (elem->m_value > 0) {
            XBT_DEBUG("\t\tGet rid of variable %p", elem->p_variable);
            varList->erase(std::find(varList->begin(), varList->end(), elem->p_variable));	
          }
        }
      } else
	++cnstIt;
    }
  } while (!varList->empty());
  
  cnstList->clear();
  sys->m_modified = 0;
  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    XBT_DEBUG("Fair bottleneck done");
    sys->print();
  }
}
