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

void lagrange_solve(lmm_system_t sys);

void lagrange_solve(lmm_system_t sys)
{

  /*
   * Lagrange Variables.
   */
  double epsilon_min_error = 1e-6;
  double overall_error = 1;
  double sigma_step = 0.5e-3;
  double capacity_error=0, bound_error=0;
  

  /*
   * Variables to manipulate the data structure proposed to model the maxmin
   * fairness. See docummentation for more details.
   */
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem1 = NULL;
  lmm_element_t elem2 = NULL;

  xbt_swag_t cnst_list = NULL;
  lmm_constraint_t cnst1 = NULL;
  lmm_constraint_t cnst2 = NULL;

  xbt_swag_t var_list = NULL;
  lmm_variable_t var1 = NULL;
  lmm_variable_t var2 = NULL;


  /*
   * Auxiliar variables.
   */
  int iteration=0;
  int max_iterations=100000;
  double mu_partial=0;
  double lambda_partial=0;
  double tmp=0;
  int i;


  if ( !(sys->modified))
    return;
  
  /* 
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables. Saves the initial value
   * of bound associated with.
   */
  var_list = &(sys->variable_set);
  i=0;
  xbt_swag_foreach(var1, var_list) {
    if(var1->weight != 0.0){
      i++;
      var1->initial_bound = var1->bound;
    }
  }

  /* 
   * Saves the initial bound of each constraint.
   */
  cnst_list=&(sys->active_constraint_set); 
  xbt_swag_foreach(cnst1, cnst_list) {
    cnst1->initial_bound = cnst1->bound;
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
    var_list = &(sys->variable_set);
    xbt_swag_foreach(var1, var_list) {
      
      mu_partial = 0;
      
      //for each link with capacity cnsts[i] that uses flow of variable var1 do
      for(i=0; i<var1->cnsts_number; i++){
	elem1 = &(var1->cnsts[i]);
	mu_partial += (elem1->constraint)->bound + var1->initial_bound;
      }

      mu_partial = -1 / mu_partial + var1->initial_bound;
      var1->bound = var1->bound + sigma_step * mu_partial;
    }


    /*                         d Dual
     * Compute the value of ------------- (\lambda^k, \mu^k) this portion
     *                      d \lambda_i^k
     * of code depends on function f(x).
     */
    cnst_list=&(sys->active_constraint_set);
    xbt_swag_foreach(cnst1, cnst_list) {
      
      lambda_partial = 0;
      
      elem_list = &(cnst1->active_element_set);

      xbt_swag_foreach(elem1, elem_list) {
	lambda_partial = 0;
   
	var2 = elem1->variable;

	//for each link with capacity cnsts[i] that uses flow of variable var1 do
	for(i=0; i<var2->cnsts_number; i++){
	  elem2 = &(var2->cnsts[i]);
	  tmp += (elem2->constraint)->bound + var2->bound;
	}
	
	lambda_partial += -1 / tmp;
      }
      
      lambda_partial += cnst1->initial_bound;
      cnst1->bound = cnst1->bound + sigma_step * lambda_partial;
    }

    
    
    /*
     * Verify for each capacity constraint (lambda) the error associated. 
     */
    cnst_list=&(sys->active_constraint_set); 
    xbt_swag_foreach(cnst1, cnst_list) {
      cnst2 = xbt_swag_getNext(cnst1,(var_list)->offset);
      if(cnst2 != NULL){
	capacity_error += fabs( cnst1->bound - cnst2->bound );
      }
    }

    /*
     * Verify for each variable the error of round trip time constraint (mu).
     */
    bound_error = 0;
    var_list = &(sys->variable_set);
    xbt_swag_foreach(var1, var_list) {
      var2 = xbt_swag_getNext(var1,(var_list)->offset);
      if(var2 != NULL){
	bound_error += fabs( var2->bound - var1->bound);
      }
    }

    overall_error = capacity_error + bound_error;
  }


  if(overall_error > epsilon_min_error){
    DEBUG1("The method converge in %d iterations.", iteration);
  }

  /*
   * Now computes the values of each variable (\rho) based on
   * the values of \lambda and \mu.
   */
  var_list = &(sys->variable_set);
  xbt_swag_foreach(var1, var_list) {
    tmp = 0;
    for(i=0; i<var1->cnsts_number; i++){
      elem1 = &(var1->cnsts[i]);
      tmp += (elem1->constraint)->bound + var1->bound;
    }
    var1->weight = 1 / tmp;
  }


 

}
