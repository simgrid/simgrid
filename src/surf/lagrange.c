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
XBT_LOG_NEW_SUBCATEGORY(surf_lagrange_dichotomy, surf,
			"Logging specific to SURF (lagrange dichotomy)");

/*
 * Local prototypes to implement the lagrangian optimization with optimal step, also called dichotomy.
 */
//solves the proportional fairness using a lagrange optimizition with dichotomy step
void lagrange_solve(lmm_system_t sys);
//computes the value of the dichotomy using a initial values, init, with a specific variable or constraint
double dichotomy(double init, double diff(double, void *), void *var_cnst,
		 double min_error);
//computes the value of the differential of variable param_var applied to mu  
double partial_diff_mu(double mu, void *param_var);
//computes the value of the differential of constraint param_cnst applied to lambda  
double partial_diff_lambda(double lambda, void *param_cnst);
//auxiliar function to compute the partial_diff
double diff_aux(lmm_variable_t var, double x);


static int __check_kkt(xbt_swag_t cnst_list, xbt_swag_t var_list, int warn)
{
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem = NULL;
  lmm_constraint_t cnst = NULL;
  lmm_variable_t var = NULL;

  double tmp;

  //verify the KKT property for each link
  xbt_swag_foreach(cnst, cnst_list) {
    tmp = 0;
    elem_list = &(cnst->element_set);
    xbt_swag_foreach(elem, elem_list) {
      var = elem->variable;
      if (var->weight <= 0)
	continue;
      tmp += var->value;
    }

    if (double_positive(tmp - cnst->bound)) {
      if (warn)
	WARN3
	    ("The link (%p) is over-used. Expected less than %f and got %f",
	     cnst, cnst->bound, tmp);
      return 0;
    }
    DEBUG3("Checking KKT for constraint (%p): sat = %f, lambda = %f ",
	   cnst, tmp - cnst->bound, cnst->lambda);

/*     if(!((fabs(tmp - cnst->bound)<MAXMIN_PRECISION && cnst->lambda>=MAXMIN_PRECISION) || */
/* 	 (fabs(tmp - cnst->bound)>=MAXMIN_PRECISION && cnst->lambda<MAXMIN_PRECISION))) { */
/*       if(warn) WARN1("The KKT condition is not verified for cnst %p...", cnst); */
/*       return 0; */
/*     } */
  }

  //verify the KKT property of each flow
  xbt_swag_foreach(var, var_list) {
    if (var->bound < 0 || var->weight <= 0)
      continue;
    DEBUG3("Checking KKT for variable (%p): sat = %f mu = %f", var,
	   var->value - var->bound, var->mu);

    if (double_positive(var->value - var->bound)) {
      if (warn)
	WARN3
	    ("The variable (%p) is too large. Expected less than %f and got %f",
	     var, var->bound, var->value);
      return 0;
    }

/*     if(!((fabs(var->value - var->bound)<MAXMIN_PRECISION && var->mu>=MAXMIN_PRECISION) || */
/* 	 (fabs(var->value - var->bound)>=MAXMIN_PRECISION && var->mu<MAXMIN_PRECISION))) { */
/*       if(warn) WARN1("The KKT condition is not verified for var %p...",var); */
/*       return 0; */
/*     } */
  }
  return 1;
}

void lagrange_solve(lmm_system_t sys)
{
  /*
   * Lagrange Variables.
   */
  int max_iterations = 10000;
  double epsilon_min_error = 1e-6;
  double dichotomy_min_error = 1e-8;
  double overall_error = 1;

  /*
   * Variables to manipulate the data structure proposed to model the maxmin
   * fairness. See docummentation for more details.
   */
  xbt_swag_t cnst_list = NULL;
  lmm_constraint_t cnst = NULL;

  xbt_swag_t var_list = NULL;
  lmm_variable_t var = NULL;

  /*
   * Auxiliar variables.
   */
  int iteration = 0;
  double tmp = 0;
  int i;


  DEBUG0("Iterative method configuration snapshot =====>");
  DEBUG1("#### Maximum number of iterations       : %d", max_iterations);
  DEBUG1("#### Minimum error tolerated            : %e",
	 epsilon_min_error);
  DEBUG1("#### Minimum error tolerated (dichotomy) : %e",
	 dichotomy_min_error);

  if (!(sys->modified))
    return;

  /* 
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables. Initialize mu.
   */
  var_list = &(sys->variable_set);
  i = 0;
  xbt_swag_foreach(var, var_list) {
    if ((var->bound < 0.0) || (var->weight <= 0.0)) {
      DEBUG1("#### NOTE var(%d) is a boundless (or inactive) variable", i);
      var->mu = -1.0;
    } else {
      var->mu = 1.0;
      var->new_mu = 2.0;
    }
    DEBUG3("#### var(%d) %p ->mu :  %e", i, var, var->mu);
    DEBUG3("#### var(%d) %p ->weight: %e", i, var, var->weight);
    DEBUG3("#### var(%d) %p ->bound: %e", i, var, var->bound);
    i++;
  }

  /* 
   * Initialize lambda.
   */
  cnst_list = &(sys->active_constraint_set);
  xbt_swag_foreach(cnst, cnst_list) {
    cnst->lambda = 1.0;
    cnst->new_lambda = 2.0;
    DEBUG2("#### cnst(%p)->lambda :  %e", cnst, cnst->lambda);
  }

  /*
   * While doesn't reach a minimun error or a number maximum of iterations.
   */
  while (overall_error > epsilon_min_error && iteration < max_iterations) {

    iteration++;
    DEBUG1("************** ITERATION %d **************", iteration);

    /*                       
     * Compute the value of mu_i
     */
    //forall mu_i in mu_1, mu_2, ..., mu_n
    xbt_swag_foreach(var, var_list) {
      if ((var->bound >= 0) && (var->weight > 0)) {
	DEBUG1("====> Working on var (%p)", var);
	var->new_mu =
	    dichotomy(var->mu, partial_diff_mu, var, dichotomy_min_error);
	if (var->new_mu < 0)
	  var->new_mu = 0;
	DEBUG3("====> var->mu (%p) : %g -> %g", var, var->mu, var->new_mu);
	var->mu = var->new_mu;
      }
    }

    /*
     * Compute the value of lambda_i
     */
    //forall lambda_i in lambda_1, lambda_2, ..., lambda_n
    xbt_swag_foreach(cnst, cnst_list) {
      DEBUG1("====> Working on cnst (%p)", cnst);
      cnst->new_lambda =
	  dichotomy(cnst->lambda, partial_diff_lambda, cnst,
		    dichotomy_min_error);
      DEBUG2("====> cnst->lambda (%p) = %e", cnst, cnst->new_lambda);
      cnst->lambda = cnst->new_lambda;
    }

    /*
     * Now computes the values of each variable (\rho) based on
     * the values of \lambda and \mu.
     */
    overall_error = 0;
    xbt_swag_foreach(var, var_list) {
      if (var->weight <= 0)
	var->value = 0.0;
      else {
	//compute sigma_i + mu_i
	tmp = 0;
	for (i = 0; i < var->cnsts_number; i++) {
	  tmp += (var->cnsts[i].constraint)->lambda;
	}
	if (var->bound > 0)
	  tmp += var->mu;
	DEBUG3("\t Working on var (%p). cost = %e; Df = %e", var, tmp,
	       var->df);

	//uses the partial differential inverse function
	tmp = var->func_fpi(var, tmp);

	//computes de overall_error using normalized value
	if (overall_error < (fabs(var->value - tmp) / tmp)) {
	  overall_error = (fabs(var->value - tmp) / tmp);
	}

	var->value = tmp;
      }
      DEBUG3("======> value of var (%p)  = %e, overall_error = %e", var,
	     var->value, overall_error);
    }

    if (!__check_kkt(cnst_list, var_list, 0))
      overall_error = 1.0;
    DEBUG2("Iteration %d: Overall_error : %f", iteration, overall_error);
  }


  __check_kkt(cnst_list, var_list, 1);

  if (overall_error <= epsilon_min_error) {
    DEBUG1("The method converges in %d iterations.", iteration);
  }
  if (iteration >= max_iterations) {
    WARN1
	("Method reach %d iterations, which is the maximum number of iterations allowed.",
	 iteration);
  }
/*   INFO1("Method converged after %d iterations", iteration); */

  if (XBT_LOG_ISENABLED(surf_lagrange, xbt_log_priority_debug)) {
    lmm_print(sys);
  }
}

/*
 * Returns a double value corresponding to the result of a dichotomy proccess with
 * respect to a given variable/constraint (\mu in the case of a variable or \lambda in
 * case of a constraint) and a initial value init. 
 *
 * @param init initial value for \mu or \lambda
 * @param diff a function that computes the differential of with respect a \mu or \lambda
 * @param var_cnst a pointer to a variable or constraint 
 * @param min_erro a minimun error tolerated
 *
 * @return a double correponding to the result of the dichotomyal process
 */
double dichotomy(double init, double diff(double, void *), void *var_cnst,
		 double min_error)
{
  double min, max;
  double overall_error;
  double middle;
  double min_diff, max_diff, middle_diff;
  double diff_0 = 0.0;
  min = max = init;

  XBT_IN;

  if (init == 0.0) {
    min = max = 0.5;
  }

  min_diff = max_diff = middle_diff = 0.0;
  overall_error = 1;

  if ((diff_0 = diff(1e-16, var_cnst)) >= 0) {
    CDEBUG1(surf_lagrange_dichotomy, "====> returning 0.0 (diff = %e)",
	    diff_0);
    return 0.0;
  }

  CDEBUG1(surf_lagrange_dichotomy,
	  "====> not detected positive diff in 0 (%e)", diff_0);

  while (overall_error > min_error) {

    min_diff = diff(min, var_cnst);
    max_diff = diff(max, var_cnst);

    CDEBUG2(surf_lagrange_dichotomy,
	    "DICHOTOMY ===> min = %1.20f , max = %1.20f", min, max);
    CDEBUG2(surf_lagrange_dichotomy,
	    "DICHOTOMY ===> diffmin = %1.20f , diffmax = %1.20f", min_diff,
	    max_diff);

    if (min_diff > 0 && max_diff > 0) {
      if (min == max) {
	CDEBUG0(surf_lagrange_dichotomy, "Decreasing min");
	min = min / 2.0;
      } else {
	CDEBUG0(surf_lagrange_dichotomy, "Decreasing max");
	max = min;
      }
    } else if (min_diff < 0 && max_diff < 0) {
      if (min == max) {
	CDEBUG0(surf_lagrange_dichotomy, "Increasing max");
	max = max * 2.0;
      } else {
	CDEBUG0(surf_lagrange_dichotomy, "Increasing min");
	min = max;
      }
    } else if (min_diff < 0 && max_diff > 0) {
      middle = (max + min) / 2.0;
      middle_diff = diff(middle, var_cnst);

      if (max != 0.0 && min != 0.0) {
	overall_error = fabs(min - max) / max;
      }

      if (middle_diff < 0) {
	min = middle;
      } else if (middle_diff > 0) {
	max = middle;
      } else {
	CWARN0(surf_lagrange_dichotomy,
	       "Found an optimal solution with 0 error!");
	overall_error = 0;
	return middle;
      }

    } else if (min_diff == 0) {
      return min;
    } else if (max_diff == 0) {
      return max;
    } else if (min_diff > 0 && max_diff < 0) {
      CWARN0(surf_lagrange_dichotomy,
	     "The impossible happened, partial_diff(min) > 0 && partial_diff(max) < 0");
    } else {
      CWARN2(surf_lagrange_dichotomy,
	     "diffmin (%1.20f) or diffmax (%1.20f) are something I don't know, taking no action.",
	     min_diff, max_diff);
      abort();
    }
  }

  XBT_OUT;

  CDEBUG1(surf_lagrange_dichotomy, "====> returning %e",
	  (min + max) / 2.0);
  return ((min + max) / 2.0);
}

/*
 *
 */
double partial_diff_mu(double mu, void *param_var)
{
  double mu_partial = 0.0;
  double sigma_mu = 0.0;
  lmm_variable_t var = (lmm_variable_t) param_var;
  int i;
  XBT_IN;
  //compute sigma_i
  for (i = 0; i < var->cnsts_number; i++)
    sigma_mu += (var->cnsts[i].constraint)->lambda;

  //compute sigma_i + mu_i
  sigma_mu += mu;

  //use auxiliar function passing (sigma_i + mu_i)
  mu_partial = diff_aux(var, sigma_mu);

  //add the RTT limit
  mu_partial += var->bound;

  XBT_OUT;
  return mu_partial;
}

/*
 *
 */
double partial_diff_lambda(double lambda, void *param_cnst)
{

  int i;
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem = NULL;
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = (lmm_constraint_t) param_cnst;
  double lambda_partial = 0.0;
  double sigma_i = 0.0;

  XBT_IN;
  elem_list = &(cnst->element_set);

  CDEBUG1(surf_lagrange_dichotomy,"Computting diff of cnst (%p)", cnst);

  xbt_swag_foreach(elem, elem_list) {
    var = elem->variable;
    if (var->weight <= 0)
      continue;

    //initilize de sumation variable
    sigma_i = 0.0;

    //compute sigma_i of variable var
    for (i = 0; i < var->cnsts_number; i++) {
      sigma_i += (var->cnsts[i].constraint)->lambda;
    }

    //add mu_i if this flow has a RTT constraint associated
    if (var->bound > 0)
      sigma_i += var->mu;

    //replace value of cnst->lambda by the value of parameter lambda
    sigma_i = (sigma_i - cnst->lambda) + lambda;

    //use the auxiliar function passing (\sigma_i + \mu_i)
    lambda_partial += diff_aux(var, sigma_i);
  }


  lambda_partial += cnst->bound;


  CDEBUG1(surf_lagrange_dichotomy, "returning = %1.20f", lambda_partial);

  XBT_OUT;
  return lambda_partial;
}


double diff_aux(lmm_variable_t var, double x)
{
  double tmp_fpi, result;

  XBT_IN2("(var (%p), x (%1.20f))", var, x);
  xbt_assert0(var->func_fp,
	      "Initialize the protocol functions first create variables before.");

  tmp_fpi = var->func_fpi(var, x);
  result = - tmp_fpi;

  CDEBUG1(surf_lagrange_dichotomy, "returning %1.20f", result);
  XBT_OUT;
  return result;
}
