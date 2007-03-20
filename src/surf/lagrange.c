/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Pedro Velho. All rights reserved.     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Modelling the proportional fairness using the Lagrange Optimization 
 * Approach. For a detailed description see:
 * "ssh://username@scm.gforge.inria.fr/svn/memo/people/pvelho/lagrange/ppf.ps".
 */
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "xbt/mallocator.h"
#include "maxmin_private.h"

#include <stdlib.h>
#ifndef MATH
#include <math.h>
#endif


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_lagrange, surf,
				"Logging specific to SURF (lagrange)");

void lagrange_solve(lmm_system_t sys)
{

  /*
   * Lagrange Variables.
   */
  double epsilon_min_error = 1e-6;
  double overall_error = 1;
  double sigma_step = 0.5e-3;
  double capacity_error, bound_error;
  double sum_capacity = 0;
  double sum_bound = 0;
  

  /*
   * Variables to manipulate the data structure proposed to model the maxmin
   * fairness. See docummentation for more details.
   */
  lmm_element_t elem = NULL;
  xbt_swag_t cnst_list = NULL;
  lmm_constraint_t cnst1 = NULL;
  lmm_constraint_t cnst2 = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  lmm_variable_t var1 = NULL;
  lmm_variable_t var2 = NULL;


  /*
   * Auxiliar variables.
   */
  int iteration=0;
  int max_iterations=100000;
  double mu_partial=0;
  double lambda_partial=0;


  if ( !(sys->modified))
    return;
  
  /* 
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables and compute the sum
   * of all round trip time constraints. May change depending on the 
   * function f(x).
   */
  var_list = &(sys->active_variable_set);
  i=0;
  xbt_swag_foreach(var1, var_list) {
    if(var1->weight != 0.0){
      i++;
      sum_bound += var1->bound;
    }
  }

  /* 
   * Compute the sum of all capacities constraints. May change depending
   * on the function f(x).
   */
  cnst_list=&(sys->active_constraint_set); 
  xbt_swag_foreach(cnst1, cnst_list) {
     sum_capacity += cnst1->value;
  }

  
  /*
   * While doesn't reach a minimun error or a number maximum of iterations.
   */
  while(overall_error > epsilon_min_error && iteration < max_iterations){

    iteration++;

    
    
    /*                        d Dual
     * Compute the value of ----------- (\lambda^k, \mu^k) this portion
     *                       d \mu_i^k
     * of code depends on function f(x).
     */
    bound_error = 0;
    xbt_swag_foreach(var1, var_list) {
      
      mu_partial = 0;
      
      //for each link elem1 that uses flow of variable var1 do
      //mu_partial += elem1->weight + var1->bound; 

      mu_partial = - (1 / mu_partial) + sum_bound;

      var1->bound = var1->bound + sigma_step * mu_partial;
    }

    
    
    /*
     * Verify for each capacity constraint (lambda) the error associated. 
     */
    xbt_swag_foreach(cnst1, cnst_list) {
      cnst2 = xbt_swag_getNext(cnst1,(var_list)->offset);
      if(cnst2 != NULL){
	 capacity_error += fabs(cnst1->value - cnsts2->value);
      }
    }

    /*
     * Verify for each variable the error of round trip time constraint (mu).
     */
    bound_error = 0;
    xbt_swag_foreach(var1, var_list) {
      var2 = xbt_swag_getNext(var1,(var_list)->offset);
      if(var2 != NULL){
	bound_error += fabs( var2->weight - var1->weight);
      }
    }

    overall_error = capacity_error + bound_error;
  }

}
