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

XBT_LOG_NEW_SUBCATEGORY(surf_writelambda, surf,
				"Generates the lambda.in file. WARNING: the size of this file might be a few GBs.");

void lagrange_solve(lmm_system_t sys);

void lagrange_solve(lmm_system_t sys)
{

  /*
   * Lagrange Variables.
   */
  double epsilon_min_error = 1e-6;
  double overall_error = 1;
  double sigma_step = 1e-3;
  double capacity_error=0, bound_error=0;
  

  /*
   * Variables to manipulate the data structure proposed to model the maxmin
   * fairness. See docummentation for more details.
   */
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem1 = NULL;
  lmm_element_t elem = NULL;

  xbt_swag_t cnst_list = NULL;
  lmm_constraint_t cnst1 = NULL;
  lmm_constraint_t cnst2 = NULL;
  lmm_constraint_t cnst = NULL;
  double sum;
  xbt_swag_t var_list = NULL;
  lmm_variable_t var1 = NULL;
  lmm_variable_t var = NULL;
  lmm_variable_t var2 = NULL;


  /*
   * Auxiliar variables.
   */
  int iteration=0;
  int max_iterations= 1000;
  double mu_partial=0;
  double lambda_partial=0;
  double tmp=0;
  int i,j;
  FILE *gnuplot_file=NULL;
  char print_buf[1024];
  char *trace_buf=xbt_malloc0(sizeof(char));






  if ( !(sys->modified))
    return;
  
  /* 
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables. Initialize mu.
   */
  var_list = &(sys->variable_set);
  i=0;
  xbt_swag_foreach(var1, var_list) {
    if((var1->bound > 0.0) || (var1->weight <= 0.0)){
      DEBUG1("## NOTE var1(%d) is a boundless variable", i);
      var1->mu = -1.0;
    } else 
      var1->mu = 1.0;
    DEBUG2("## var1(%d)->mu:  %e", i, var1->mu);
    DEBUG2("## var1(%d)->weight: %e", i, var1->weight);
    i++;
  }

  /* 
   * Initialize lambda.
   */
  cnst_list=&(sys->active_constraint_set); 
  xbt_swag_foreach(cnst1, cnst_list) {
    cnst1->lambda = 1.0;
    DEBUG2("## cnst1(%p)->lambda:  %e", cnst1, cnst1->lambda);
  }

  if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
    gnuplot_file = fopen("lambda.in", "w");
    fprintf(gnuplot_file, "# iteration    lambda1  lambda2 lambda3 ... lambdaP");
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
      if((var1->bound > 0) || (var1->weight <=0) ){
	//for each link with capacity cnsts[i] that uses flow of variable var1 do
	for(i=0; i<var1->cnsts_number; i++)
	  mu_partial += (var1->cnsts[i].constraint)->lambda;
       
        mu_partial = -1.0 / mu_partial + var1->bound;
	var1->new_mu = var1->mu - sigma_step * mu_partial; 
	/* Assert that var1->new_mu is positive */
     }
    }

    if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
      fprintf(gnuplot_file, "\n%d ", iteration);
    }

    /*                         d Dual
     * Compute the value of ------------- (\lambda^k, \mu^k) this portion
     *                      d \lambda_i^k
     * of code depends on function f(x).
     */

    DEBUG1("######Lambda partial at iteration  %d", iteration);
    cnst_list=&(sys->active_constraint_set);
    j=0;
    xbt_swag_foreach(cnst1, cnst_list) {
      j++;

      lambda_partial = 0;
      
      elem_list = &(cnst1->element_set);
      xbt_swag_foreach(elem1, elem_list) {
	lambda_partial = 0;
   
	var2 = elem1->variable;
	
	if(var2->weight<=0) continue;

	tmp = 0;

	//for each link with capacity cnsts[i] that uses flow of variable var1 do
	if(var2->bound > 0)
	  tmp += var2->mu;

	for(i=0; i<var2->cnsts_number; i++)
	  tmp += (var2->cnsts[i].constraint)->lambda;
	
	lambda_partial += -1 / tmp;
      }

      lambda_partial += cnst1->bound;

      DEBUG2("###########Lambda partial %p : %e", cnst1, lambda_partial);

      cnst1->new_lambda = cnst1->lambda - sigma_step * lambda_partial;
      
      if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
	fprintf(gnuplot_file, " %f", cnst1->lambda);
      }
    }

    /* Updating lambda's and mu's */  
    var_list = &(sys->variable_set); 
    xbt_swag_foreach(var1, var_list)
      if(!((var1->bound > 0.0) || (var1->weight <= 0.0)))
	var1->mu = var1->new_mu;
  

    cnst_list=&(sys->active_constraint_set); 
    xbt_swag_foreach(cnst1, cnst_list)
      cnst1->lambda = cnst1->new_lambda;
    
    /*
     * Now computes the values of each variable (\rho) based on
     * the values of \lambda and \mu.
     */
    var_list = &(sys->variable_set);
    xbt_swag_foreach(var1, var_list) {
      if(var1->weight <=0) 
	var1->value = 0.0;
      else {
	tmp = 0;
	if(var1->bound >0) 
	  tmp+=var1->mu;
	for(i=0; i<var1->cnsts_number; i++)
	  tmp += (var1->cnsts[i].constraint)->lambda;

	var1->value = 1 / tmp;
      }
	
      
      DEBUG2("var1->value (id=%s) : %e", (char *)var1->id, var1->value);
    }

  /* Printing Objective */
  var_list = &(sys->variable_set);
  sprintf(print_buf,"MAX-MIN ( ");
  trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
  strcat(trace_buf, print_buf);
  xbt_swag_foreach(var, var_list) {
    sprintf(print_buf,"'%p'(%f) ",var,var->weight);
    trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
    strcat(trace_buf, print_buf);
  }
  sprintf(print_buf,")");
  trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
  strcat(trace_buf, print_buf);
  DEBUG1("%s",trace_buf);
  trace_buf[0]='\000';

  /* Printing Constraints */
  cnst_list = &(sys->active_constraint_set);
  xbt_swag_foreach(cnst, cnst_list) {
    sum=0.0;
    elem_list = &(cnst->element_set);
    sprintf(print_buf,"\t");
    trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
    strcat(trace_buf, print_buf);
    xbt_swag_foreach(elem, elem_list) {
      sprintf(print_buf,"%f.'%p'(%f) + ",elem->value, 
	      elem->variable,elem->variable->value);
      trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
      strcat(trace_buf, print_buf);
      sum += elem->value * elem->variable->value;
    }
    sprintf(print_buf,"0 <= %f ('%p')",cnst->bound,cnst);
    trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
    strcat(trace_buf, print_buf);

    if(!cnst->shared) {
      sprintf(print_buf," [MAX-Constraint]");
      trace_buf = xbt_realloc(trace_buf,strlen(trace_buf)+strlen(print_buf)+1);
      strcat(trace_buf, print_buf);
    }
    DEBUG1("%s",trace_buf);
    trace_buf[0]='\000';
    if(!(sum<=cnst->bound))
      DEBUG3("Incorrect value (%f is not smaller than %f): %g",
		sum,cnst->bound,sum-cnst->bound);
  }

  /* Printing Result */
  xbt_swag_foreach(var, var_list) {
    if(var->bound>0) {
      DEBUG4("'%p'(%f) : %f (<=%f)",var,var->weight,var->value, var->bound);
      if(var->value<=var->bound) 
	DEBUG0("Incorrect value");
    }
    else 
      DEBUG3("'%p'(%f) : %f",var,var->weight,var->value);
  }


    /*
     * Verify for each capacity constraint (lambda) the error associated. 
     */
    capacity_error = 0;
    cnst_list=&(sys->active_constraint_set); 
    xbt_swag_foreach(cnst1, cnst_list) {
      cnst2 = xbt_swag_getNext(cnst1,(var_list)->offset);
      if(cnst2 != NULL){
	capacity_error += fabs( cnst1->lambda - cnst2->lambda );
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
	bound_error += fabs( var2->mu - var1->mu);
      }
    }

    overall_error = capacity_error + bound_error;
  }




  if(overall_error <= epsilon_min_error){
    DEBUG1("The method converge in %d iterations.", iteration);
  }else{
    WARN1("Method reach %d iterations, which is the maxmimun number of iterations allowed.", iteration);
  }


  if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
    fclose(gnuplot_file);
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
      tmp += (elem1->constraint)->lambda + var1->mu;
    }
    var1->weight = 1 / tmp;

    DEBUG2("var1->weight (id=%s) : %e", (char *)var1->id, var1->weight);
  }




}
