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


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_lagrange, surf, "Logging specific to SURF (lagrange)");

/*
 * Local prototypes to implement the lagrangian optimization with optimal step, also called dicotomi.
 */
//solves the proportional fairness using a lagrange optimizition with dicotomi step
void   lagrange_solve       (lmm_system_t sys);
//computes the value of the dicotomi using a initial values, init, with a specific variable or constraint
double dicotomi(double init, double diff(double, void*), void *var_cnst, double min_error);
//computes the value of the differential of variable param_var applied to mu  
double partial_diff_mu      (double mu, void * param_var);
//computes the value of the differential of constraint param_cnst applied to lambda  
double partial_diff_lambda  (double lambda, void * param_cnst);
//auxiliar function to compute the partial_diff
double diff_aux(lmm_variable_t var, double x);


void lagrange_solve(lmm_system_t sys)
{
  /*
   * Lagrange Variables.
   */
  int max_iterations= 10000;
  double epsilon_min_error = 1e-4;
  double dicotomi_min_error = 1e-8;
  double overall_error = 1;

  /*
   * Variables to manipulate the data structure proposed to model the maxmin
   * fairness. See docummentation for more details.
   */
  xbt_swag_t elem_list  = NULL;
  lmm_element_t elem    = NULL;

  xbt_swag_t cnst_list  = NULL;
  lmm_constraint_t cnst = NULL;
  
  xbt_swag_t var_list   = NULL;
  lmm_variable_t var    = NULL;

  /*
   * Auxiliar variables.
   */
  int iteration=0;
  double tmp=0;
  int i;
   

  DEBUG0("Iterative method configuration snapshot =====>");
  DEBUG1("#### Maximum number of iterations       : %d", max_iterations);
  DEBUG1("#### Minimum error tolerated            : %e", epsilon_min_error);  
  DEBUG1("#### Minimum error tolerated (dicotomi) : %e", dicotomi_min_error);

  if ( !(sys->modified))
    return;

  /* 
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables. Initialize mu.
   */
  var_list = &(sys->variable_set);
  i=0;
  xbt_swag_foreach(var, var_list) {    
    if((var->bound > 0.0) || (var->weight <= 0.0)){
      DEBUG1("#### NOTE var(%d) is a boundless variable", i);
      var->mu = -1.0;
    } else{ 
      var->mu =   1.0;
      var->new_mu = 2.0;
    }
    DEBUG2("#### var(%d)->mu :  %e", i, var->mu);
    DEBUG2("#### var(%d)->weight: %e", i, var->weight);
    i++;
  }

  /* 
   * Initialize lambda.
   */
  cnst_list=&(sys->active_constraint_set); 
  xbt_swag_foreach(cnst, cnst_list){
    cnst->lambda = 1.0;
    cnst->new_lambda = 2.0;
    DEBUG2("#### cnst(%p)->lambda :  %e", cnst, cnst->lambda);
  }
  
  /*
   * While doesn't reach a minimun error or a number maximum of iterations.
   */
  while(overall_error > epsilon_min_error && iteration < max_iterations){
   
    iteration++;
    DEBUG1("************** ITERATION %d **************", iteration);    

    /*                       
     * Compute the value of mu_i
     */
    //forall mu_i in mu_1, mu_2, ..., mu_n
    xbt_swag_foreach(var, var_list) {
      if((var->bound >= 0) && (var->weight > 0) ){
	var->new_mu = dicotomi(var->mu, partial_diff_mu, var, dicotomi_min_error);
	if(var->new_mu < 0) var->new_mu = 0;
	var->mu = var->new_mu;
      }
    }

    /*
     * Compute the value of lambda_i
     */
    //forall lambda_i in lambda_1, lambda_2, ..., lambda_n
    xbt_swag_foreach(cnst, cnst_list) {
      cnst->new_lambda = dicotomi(cnst->lambda, partial_diff_lambda, cnst, dicotomi_min_error);
      DEBUG2("====> cnst->lambda (%p) = %e", cnst, cnst->new_lambda);      
      cnst->lambda = cnst->new_lambda;
    }

    /*
     * Now computes the values of each variable (\rho) based on
     * the values of \lambda and \mu.
     */
    overall_error=0;   
    xbt_swag_foreach(var, var_list) {
      if(var->weight <=0) 
	var->value = 0.0;
      else {
	//compute sigma_i + mu_i
	tmp = 0;
	for(i=0; i<var->cnsts_number; i++){
	  tmp += (var->cnsts[i].constraint)->lambda;
	  if(var->bound > 0) 
	    tmp+=var->mu;
	}

	//uses the partial differential inverse function
	tmp = var->func_fpi(var, tmp);

	//computes de overall_error
	if(overall_error < fabs(var->value - tmp)){
	  overall_error = fabs(var->value - tmp);
	}
	
	var->value = tmp;
      }
      DEBUG4("======> value of var %s (%p)  = %e, overall_error = %e", (char *)var->id, var, var->value, overall_error);       
    }
  }


  //verify the KKT property for each link
  xbt_swag_foreach(cnst, cnst_list){
    tmp = 0;
    elem_list = &(cnst->element_set);
    xbt_swag_foreach(elem, elem_list) {
      var = elem->variable;
      if(var->weight<=0) continue;
      tmp += var->value;
    }
  
    tmp = tmp - cnst->bound;

    if(tmp > epsilon_min_error){
      WARN4("The link %s(%p) doesn't match the KKT property, expected less than %e and got %e", (char *)cnst->id, cnst, epsilon_min_error, tmp);
    }
  
  }
  
  //verify the KKT property of each flow
  xbt_swag_foreach(var, var_list){
    if(var->bound <= 0 || var->weight <= 0) continue;
    tmp = 0;
    tmp = (var->value - var->bound);

    
    if(tmp != 0 ||  var->mu != 0){
      WARN4("The flow %s(%p) doesn't match the KKT property, value expected (=0) got (lambda=%e) (sum_rho=%e)", (char *)var->id, var, var->mu, tmp);
    }

  }

  if(overall_error <= epsilon_min_error){
    DEBUG1("The method converge in %d iterations.", iteration);
  }else{
    WARN1("Method reach %d iterations, which is the maxmimun number of iterations allowed.", iteration);
  }
}

/*
 * Returns a double value corresponding to the result of a dicotomi proccess with
 * respect to a given variable/constraint (\mu in the case of a variable or \lambda in
 * case of a constraint) and a initial value init. 
 *
 * @param init initial value for \mu or \lambda
 * @param diff a function that computes the differential of with respect a \mu or \lambda
 * @param var_cnst a pointer to a variable or constraint 
 * @param min_erro a minimun error tolerated
 *
 * @return a double correponding to the result of the dicotomial process
 */
double dicotomi(double init, double diff(double, void*), void *var_cnst, double min_error){
  double min, max;
  double overall_error;
  double middle;
  double min_diff, max_diff, middle_diff;
  
  min = max = init;

  if(init == 0){
    min = max = 1;
  }

  min_diff = max_diff = middle_diff = 0.0;
  overall_error = 1;

  if(diff(0.0, var_cnst) > 0){
    DEBUG1("====> returning 0.0 (diff = %e)", diff(0.0, var_cnst));
    return 0.0;
  }

  DEBUG0("====> not detected positive diff in 0");

  while(overall_error > min_error){

    min_diff = diff(min, var_cnst);
    max_diff = diff(max, var_cnst);

    DEBUG2("DICOTOMI ===> min = %e , max = %e", min, max);
    DEBUG2("DICOTOMI ===> diffmin = %e , diffmax = %e", min_diff, max_diff);

    if( min_diff > 0 && max_diff > 0 ){
      if(min == max){
	min = min / 2.0;
      }else{
	max = min;
      }
    }else if( min_diff < 0 && max_diff < 0 ){
      if(min == max){
	max = max * 2.0;
      }else{
	min = max;
      }
    }else if( min_diff < 0 && max_diff > 0 ){
      middle = (max + min)/2.0;
      middle_diff = diff(middle, var_cnst);
      overall_error = fabs(min - max);

      if( middle_diff < 0 ){
	min = middle;
      }else if( middle_diff > 0 ){
	max = middle;
      }else{
	WARN0("Found an optimal solution with 0 error!");
	overall_error = 0;
	return middle;
      }

    }else if(min_diff == 0){
      return min;
    }else if(max_diff == 0){
      return max;
    }else if(min_diff > 0 && max_diff < 0){
      WARN0("The impossible happened, partial_diff(min) > 0 && partial_diff(max) < 0");
    }
  }


  DEBUG1("====> returning %e", (min+max)/2.0);
  return ((min+max)/2.0);
}

/*
 *
 */
double partial_diff_mu(double mu, void *param_var){
  double mu_partial=0.0;
  double sigma_mu=0.0;
  lmm_variable_t var = (lmm_variable_t)param_var;
  int i;

  //compute sigma_i
  for(i=0; i<var->cnsts_number; i++)
    sigma_mu += (var->cnsts[i].constraint)->lambda;
  
  //compute sigma_i + mu_i
  sigma_mu += var->mu;
  
  //use auxiliar function passing (sigma_i + mu_i)
  mu_partial = diff_aux(var, sigma_mu) ;
 
  //add the RTT limit
  mu_partial += var->bound;

  return mu_partial;
}

/*
 *
 */
double partial_diff_lambda(double lambda, void *param_cnst){

  int i;
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem = NULL;
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst= (lmm_constraint_t) param_cnst;
  double lambda_partial=0.0;
  double sigma_mu=0.0;

  elem_list = &(cnst->element_set);

  DEBUG2("Computting diff of cnst (%p) %s", cnst, (char *)cnst->id);
  
  xbt_swag_foreach(elem, elem_list) {
    var = elem->variable;
    if(var->weight<=0) continue;
    
    //initilize de sumation variable
    sigma_mu = 0.0;

    //compute sigma_i of variable var
    for(i=0; i<var->cnsts_number; i++){
      sigma_mu += (var->cnsts[i].constraint)->lambda;
    }
	
    //add mu_i if this flow has a RTT constraint associated
    if(var->bound > 0) sigma_mu += var->mu;

    //replace value of cnst->lambda by the value of parameter lambda
    sigma_mu = (sigma_mu - cnst->lambda) + lambda;
    
    //use the auxiliar function passing (\sigma_i + \mu_i)
    lambda_partial += diff_aux(var, sigma_mu);
  }

  lambda_partial += cnst->bound;

  return lambda_partial;
}


double diff_aux(lmm_variable_t var, double x){
  double tmp_fp, tmp_fpi, tmp_fpip, result;

  xbt_assert0(var->func_fp, "Initialize the protocol functions first create variables before.");

  tmp_fp = var->func_fp(var, x);
  tmp_fpi = var->func_fpi(var, x);
  tmp_fpip = var->func_fpip(var, x);

  result = tmp_fpip*(var->func_fp(var, tmp_fpi));
  
  result = result - tmp_fpi;

  result = result - (tmp_fpip * x);

  return result;
}  
 







