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

#define LAMBDA_STEP 0.01


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
  int max_iterations= 1000000;
  double epsilon_min_error = 0.00001;
  double overall_error = 1;
  double sigma_step = LAMBDA_STEP;
  //double capacity_error=0, bound_error=0;
  int watch_out = 0;

  /*
   * Variables to manipulate the data structure proposed to model the maxmin
   * fairness. See docummentation for more details.
   */
  xbt_swag_t elem_list = NULL;
  //lmm_element_t elem = NULL;
  lmm_element_t elem1 = NULL;


  xbt_swag_t cnst_list = NULL;
  //lmm_constraint_t cnst = NULL;
  lmm_constraint_t cnst1 = NULL;
  //lmm_constraint_t cnst2 = NULL;


  xbt_swag_t var_list = NULL;
  //lmm_variable_t var = NULL;
  lmm_variable_t var1 = NULL;
  lmm_variable_t var2 = NULL;

  /*
   * Auxiliar variables.
   */
  int iteration=0;
  double mu_partial=0;
  double lambda_partial=0;
  double tmp=0;
  int i,j;
  FILE *gnuplot_file=NULL;
  //char print_buf[1024];
  //char *trace_buf=xbt_malloc0(sizeof(char));
  //double sum;


  DEBUG0("Iterative method configuration snapshot =====>");
  DEBUG1("#### Maximum number of iterations : %d", max_iterations);
  DEBUG1("#### Minimum error tolerated      : %e", epsilon_min_error);
  DEBUG1("#### Step                         : %e", sigma_step);


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
      DEBUG1("#### NOTE var1(%d) is a boundless variable", i);
      var1->mu = -1.0;
    } else{ 
      var1->mu = 1.0;
      var1->new_mu = 2.0;
    }
    DEBUG2("#### var1(%d)->mu:  %e", i, var1->mu);
    DEBUG2("#### var1(%d)->weight: %e", i, var1->weight);
    i++;
  }

  /* 
   * Initialize lambda.
   */
  cnst_list=&(sys->active_constraint_set); 
  xbt_swag_foreach(cnst1, cnst_list) {
    cnst1->lambda = 1.0;
    cnst1->new_lambda = 2.0;
    DEBUG2("#### cnst1(%p)->lambda:  %e", cnst1, cnst1->lambda);
  }

  if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
    gnuplot_file = fopen("lambda.in", "w");
    fprintf(gnuplot_file, "# iteration    lambda1  lambda2 lambda3 ... lambdaP\n");
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

	if(var1->new_mu < 0){
	  var1->new_mu = 0;
	}
      }
    }


    /*                         d Dual
     * Compute the value of ------------- (\lambda^k, \mu^k) this portion
     *                      d \lambda_i^k
     * of code depends on function f(x).
     */
    j=0;
    if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
      fprintf(gnuplot_file, "\n%d",iteration);
    }
    xbt_swag_foreach(cnst1, cnst_list) {
      j++;

      lambda_partial = 0;
      
      elem_list = &(cnst1->element_set);
      watch_out=0;
      xbt_swag_foreach(elem1, elem_list) {
   
	var2 = elem1->variable;
	
	if(var2->weight<=0) continue;

	tmp = 0;

	for(i=0; i<var2->cnsts_number; i++){
	  tmp += (var2->cnsts[i].constraint)->lambda;
	}
	if(var2->bound > 0)
	  tmp += var2->mu;

	
	if(tmp==0) break;

	if (tmp==cnst1->lambda)
	  watch_out=1;
	lambda_partial += (-1.0 / tmp);
      }

      if(tmp == 0) 
	cnst1->new_lambda = LAMBDA_STEP;
      else {
	lambda_partial += cnst1->bound;
	if(watch_out && (lambda_partial>0)) {
	  /* INFO6("Watch Out (%d) %p! lambda_partial: %e; lambda : %e ; (%e %e) \n",iteration, cnst1,  */
	  /* 		lambda_partial, cnst1->lambda, cnst1->lambda / 2, */
	  /* 		cnst1->lambda - sigma_step * lambda_partial); */

	  if(cnst1->lambda < 0) WARN2("Value of cnst1->lambda(%p) = %e < 0", cnst1, cnst1->lambda);
	  if((cnst1->lambda - sigma_step * lambda_partial) < 0) WARN1("Value of lambda_new = %e < 0", (cnst1->lambda - sigma_step * lambda_partial));

	  if(cnst1->lambda - sigma_step * lambda_partial < cnst1->lambda / 2)
	    cnst1->new_lambda = cnst1->lambda / 2;
	  else
	    cnst1->new_lambda = cnst1->lambda - sigma_step * lambda_partial;
	} else
	  cnst1->new_lambda = cnst1->lambda - sigma_step * lambda_partial;
	if(cnst1->new_lambda < 0){
	  cnst1->new_lambda = 0;
	}
      }

      if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
	fprintf(gnuplot_file, "  %e", cnst1->lambda);
      }

    }


    /*
     * Now computes the values of each variable (\rho) based on
     * the values of \lambda and \mu.
     */
    overall_error=0;
    xbt_swag_foreach(var1, var_list) {
      if(var1->weight <=0) 
	var1->value = 0.0;
      else {
	tmp = 0;
	for(i=0; i<var1->cnsts_number; i++){
	  tmp += (var1->cnsts[i].constraint)->lambda;
	  if(var1->bound > 0) 
	    tmp+=var1->mu;
	}
	
	//computes de overall_error
	if(overall_error < fabs(var1->value - 1.0/tmp)){
	  overall_error = fabs(var1->value - 1.0/tmp);
	}

	var1->value = 1.0 / tmp;
      }
      
    }


    /* Updating lambda's and mu's */  
    xbt_swag_foreach(var1, var_list)
      if(!((var1->bound > 0.0) || (var1->weight <= 0.0)))
	var1->mu = var1->new_mu;
    
    
    xbt_swag_foreach(cnst1, cnst_list)
      cnst1->lambda = cnst1->new_lambda;
  }




  //verify the KKT property
  xbt_swag_foreach(cnst1, cnst_list){
    tmp = 0;
    elem_list = &(cnst1->element_set);
    xbt_swag_foreach(elem1, elem_list) {
      var1 = elem1->variable;
      if(var1->weight<=0) continue;
      tmp += var1->value;
    }

    tmp = tmp - cnst1->bound;
 

    if(tmp != 0 ||  cnst1->lambda != 0){
      WARN4("The link %s(%p) doesn't match the KKT property, value expected (=0) got (lambda=%e) (sum_rho=%e)", (char *)cnst1->id, cnst1, cnst1->lambda, tmp);
    }
    
  }

    
  xbt_swag_foreach(var1, var_list){
    if(var1->bound <= 0 || var1->weight <= 0) continue;
    tmp = 0;
    tmp = (var1->value - var1->bound);

    
    if(tmp != 0 ||  var1->mu != 0){
      WARN4("The flow %s(%p) doesn't match the KKT property, value expected (=0) got (lambda=%e) (sum_rho=%e)", (char *)var1->id, var1, var1->mu, tmp);
    }

  }




  if(overall_error <= epsilon_min_error){
    DEBUG1("The method converge in %d iterations.", iteration);
  }else{
    WARN1("Method reach %d iterations, which is the maxmimun number of iterations allowed.", iteration);
  }


  if(XBT_LOG_ISENABLED(surf_writelambda, xbt_log_priority_debug)) {
    fclose(gnuplot_file);
  }





/*   /\* */
/*    * Now computes the values of each variable (\rho) based on */
/*    * the values of \lambda and \mu. */
/*    *\/ */
/*   var_list = &(sys->variable_set); */
/*   xbt_swag_foreach(var1, var_list) { */
/*     tmp = 0; */
/*     for(i=0; i<var1->cnsts_number; i++){ */
/*       elem1 = &(var1->cnsts[i]); */
/*       tmp += (elem1->constraint)->lambda + var1->mu; */
/*     } */
/*     var1->weight = 1 / tmp; */

/*     DEBUG2("var1->weight (id=%s) : %e", (char *)var1->id, var1->weight); */
/*   } */




}
