/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Modelling the proportional fairness using the Lagrange Optimization 
 * Approach. For a detailed description see:
 * "ssh://username@scm.gforge.inria.fr/svn/memo/people/pvelho/lagrange/ppf.ps".
 */
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "maxmin_private.h"

#include <stdlib.h>
#ifndef MATH
#include <math.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_lagrange, surf,
                                "Logging specific to SURF (lagrange)");
XBT_LOG_NEW_SUBCATEGORY(surf_lagrange_dichotomy, surf_lagrange,
                        "Logging specific to SURF (lagrange dichotomy)");

#define SHOW_EXPR(expr) CDEBUG1(surf_lagrange,#expr " = %g",expr);

double (*func_f_def) (lmm_variable_t, double);
double (*func_fp_def) (lmm_variable_t, double);
double (*func_fpi_def) (lmm_variable_t, double);

/*
 * Local prototypes to implement the lagrangian optimization with optimal step, also called dichotomy.
 */
//solves the proportional fairness using a lagrange optimizition with dichotomy step
void lagrange_solve(lmm_system_t sys);
//computes the value of the dichotomy using a initial values, init, with a specific variable or constraint
static double dichotomy(double init, double diff(double, void *),
                        void *var_cnst, double min_error);
//computes the value of the differential of variable param_var applied to mu  
static double partial_diff_mu(double mu, void *param_var);
//computes the value of the differential of constraint param_cnst applied to lambda  
static double partial_diff_lambda(double lambda, void *param_cnst);

static int __check_feasible(xbt_swag_t cnst_list, xbt_swag_t var_list,
                            int warn)
{
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem = NULL;
  lmm_constraint_t cnst = NULL;
  lmm_variable_t var = NULL;

  double tmp;

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
    DEBUG3
      ("Checking feasability for constraint (%p): sat = %f, lambda = %f ",
       cnst, tmp - cnst->bound, cnst->lambda);
  }

  xbt_swag_foreach(var, var_list) {
    if (!var->weight)
      break;
    if (var->bound < 0)
      continue;
    DEBUG3("Checking feasability for variable (%p): sat = %f mu = %f", var,
           var->value - var->bound, var->mu);

    if (double_positive(var->value - var->bound)) {
      if (warn)
        WARN3
          ("The variable (%p) is too large. Expected less than %f and got %f",
           var, var->bound, var->value);
      return 0;
    }
  }
  return 1;
}

static double new_value(lmm_variable_t var)
{
  double tmp = 0;
  int i;

  for (i = 0; i < var->cnsts_number; i++) {
    tmp += (var->cnsts[i].constraint)->lambda;
  }
  if (var->bound > 0)
    tmp += var->mu;
  DEBUG3("\t Working on var (%p). cost = %e; Weight = %e", var, tmp,
         var->weight);
  //uses the partial differential inverse function
  return var->func_fpi(var, tmp);
}

static double new_mu(lmm_variable_t var)
{
  double mu_i = 0.0;
  double sigma_i = 0.0;
  int j;

  for (j = 0; j < var->cnsts_number; j++) {
    sigma_i += (var->cnsts[j].constraint)->lambda;
  }
  mu_i = var->func_fp(var, var->bound) - sigma_i;
  if (mu_i < 0.0)
    return 0.0;
  return mu_i;
}

static double dual_objective(xbt_swag_t var_list, xbt_swag_t cnst_list)
{
  lmm_constraint_t cnst = NULL;
  lmm_variable_t var = NULL;

  double obj = 0.0;

  xbt_swag_foreach(var, var_list) {
    double sigma_i = 0.0;
    int j;

    if (!var->weight)
      break;

    for (j = 0; j < var->cnsts_number; j++)
      sigma_i += (var->cnsts[j].constraint)->lambda;

    if (var->bound > 0)
      sigma_i += var->mu;

    DEBUG2("var %p : sigma_i = %1.20f", var, sigma_i);

    obj += var->func_f(var, var->func_fpi(var, sigma_i)) -
      sigma_i * var->func_fpi(var, sigma_i);

    if (var->bound > 0)
      obj += var->mu * var->bound;
  }

  xbt_swag_foreach(cnst, cnst_list)
    obj += cnst->lambda * cnst->bound;

  return obj;
}

void lagrange_solve(lmm_system_t sys)
{
  /*
   * Lagrange Variables.
   */
  int max_iterations = 100;
  double epsilon_min_error = MAXMIN_PRECISION;
  double dichotomy_min_error = 1e-14;
  double overall_modification = 1;

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
  double obj, new_obj;

  DEBUG0("Iterative method configuration snapshot =====>");
  DEBUG1("#### Maximum number of iterations       : %d", max_iterations);
  DEBUG1("#### Minimum error tolerated            : %e", epsilon_min_error);
  DEBUG1("#### Minimum error tolerated (dichotomy) : %e",
         dichotomy_min_error);

  if (XBT_LOG_ISENABLED(surf_lagrange, xbt_log_priority_debug)) {
    lmm_print(sys);
  }

  if (!(sys->modified))
    return;


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
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables. Initialize mu.
   */
  var_list = &(sys->variable_set);
  i = 0;
  xbt_swag_foreach(var, var_list) {
    if (!var->weight)
      var->value = 0.0;
    else {
      int nb = 0;
      if (var->bound < 0.0) {
        DEBUG1("#### NOTE var(%d) is a boundless variable", i);
        var->mu = -1.0;
        var->value = new_value(var);
      } else {
        var->mu = 1.0;
        var->new_mu = 2.0;
        var->value = new_value(var);
      }
      DEBUG2("#### var(%p) ->weight :  %e", var, var->weight);
      DEBUG2("#### var(%p) ->mu :  %e", var, var->mu);
      DEBUG2("#### var(%p) ->weight: %e", var, var->weight);
      DEBUG2("#### var(%p) ->bound: %e", var, var->bound);
      for (i = 0; i < var->cnsts_number; i++) {
        if (var->cnsts[i].value == 0.0)
          nb++;
      }
      if (nb == var->cnsts_number)
        var->value = 1.0;
    }
  }

  /* 
   * Compute dual objective.
   */
  obj = dual_objective(var_list, cnst_list);

  /*
   * While doesn't reach a minimun error or a number maximum of iterations.
   */
  while (overall_modification > epsilon_min_error
         && iteration < max_iterations) {
/*     int dual_updated=0; */

    iteration++;
    DEBUG1("************** ITERATION %d **************", iteration);
    DEBUG0("-------------- Gradient Descent ----------");

    /*                       
     * Improve the value of mu_i
     */
    xbt_swag_foreach(var, var_list) {
      if (!var->weight)
        break;
      if (var->bound >= 0) {
        DEBUG1("Working on var (%p)", var);
        var->new_mu = new_mu(var);
/* 	dual_updated += (fabs(var->new_mu-var->mu)>dichotomy_min_error); */
/* 	DEBUG2("dual_updated (%d) : %1.20f",dual_updated,fabs(var->new_mu-var->mu)); */
        DEBUG3("Updating mu : var->mu (%p) : %1.20f -> %1.20f", var,
               var->mu, var->new_mu);
        var->mu = var->new_mu;

        new_obj = dual_objective(var_list, cnst_list);
        DEBUG3("Improvement for Objective (%g -> %g) : %g", obj, new_obj,
               obj - new_obj);
        xbt_assert1(obj - new_obj >= -epsilon_min_error,
                    "Our gradient sucks! (%1.20f)", obj - new_obj);
        obj = new_obj;
      }
    }

    /*
     * Improve the value of lambda_i
     */
    xbt_swag_foreach(cnst, cnst_list) {
      DEBUG1("Working on cnst (%p)", cnst);
      cnst->new_lambda =
        dichotomy(cnst->lambda, partial_diff_lambda, cnst,
                  dichotomy_min_error);
/*       dual_updated += (fabs(cnst->new_lambda-cnst->lambda)>dichotomy_min_error); */
/*       DEBUG2("dual_updated (%d) : %1.20f",dual_updated,fabs(cnst->new_lambda-cnst->lambda)); */
      DEBUG3("Updating lambda : cnst->lambda (%p) : %1.20f -> %1.20f",
             cnst, cnst->lambda, cnst->new_lambda);
      cnst->lambda = cnst->new_lambda;

      new_obj = dual_objective(var_list, cnst_list);
      DEBUG3("Improvement for Objective (%g -> %g) : %g", obj, new_obj,
             obj - new_obj);
      xbt_assert1(obj - new_obj >= -epsilon_min_error,
                  "Our gradient sucks! (%1.20f)", obj - new_obj);
      obj = new_obj;
    }

    /*
     * Now computes the values of each variable (\rho) based on
     * the values of \lambda and \mu.
     */
    DEBUG0("-------------- Check convergence ----------");
    overall_modification = 0;
    xbt_swag_foreach(var, var_list) {
      if (var->weight <= 0)
        var->value = 0.0;
      else {
        tmp = new_value(var);

        overall_modification =
          MAX(overall_modification, fabs(var->value - tmp));

        var->value = tmp;
        DEBUG3("New value of var (%p)  = %e, overall_modification = %e",
               var, var->value, overall_modification);
      }
    }

    DEBUG0("-------------- Check feasability ----------");
    if (!__check_feasible(cnst_list, var_list, 0))
      overall_modification = 1.0;
    DEBUG2("Iteration %d: overall_modification : %f", iteration,
           overall_modification);
/*     if(!dual_updated) { */
/*       WARN1("Could not improve the convergence at iteration %d. Drop it!",iteration); */
/*       break; */
/*     } */
  }

  __check_feasible(cnst_list, var_list, 1);

  if (overall_modification <= epsilon_min_error) {
    DEBUG1("The method converges in %d iterations.", iteration);
  }
  if (iteration >= max_iterations) {
    DEBUG1
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
static double dichotomy(double init, double diff(double, void *),
                        void *var_cnst, double min_error)
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
    CDEBUG1(surf_lagrange_dichotomy, "returning 0.0 (diff = %e)", diff_0);
    XBT_OUT;
    return 0.0;
  }

  min_diff = diff(min, var_cnst);
  max_diff = diff(max, var_cnst);

  while (overall_error > min_error) {
    CDEBUG4(surf_lagrange_dichotomy,
            "[min, max] = [%1.20f, %1.20f] || diffmin, diffmax = %1.20f, %1.20f",
            min, max, min_diff, max_diff);

    if (min_diff > 0 && max_diff > 0) {
      if (min == max) {
        CDEBUG0(surf_lagrange_dichotomy, "Decreasing min");
        min = min / 2.0;
        min_diff = diff(min, var_cnst);
      } else {
        CDEBUG0(surf_lagrange_dichotomy, "Decreasing max");
        max = min;
        max_diff = min_diff;
      }
    } else if (min_diff < 0 && max_diff < 0) {
      if (min == max) {
        CDEBUG0(surf_lagrange_dichotomy, "Increasing max");
        max = max * 2.0;
        max_diff = diff(max, var_cnst);
      } else {
        CDEBUG0(surf_lagrange_dichotomy, "Increasing min");
        min = max;
        min_diff = max_diff;
      }
    } else if (min_diff < 0 && max_diff > 0) {
      middle = (max + min) / 2.0;
      CDEBUG1(surf_lagrange_dichotomy, "Trying (max+min)/2 : %1.20f", middle);

      if ((min == middle) || (max == middle)) {
        CWARN4(surf_lagrange_dichotomy,
               "Cannot improve the convergence! min=max=middle=%1.20f, diff = %1.20f."
               " Reaching the 'double' limits. Maybe scaling your function would help ([%1.20f,%1.20f]).",
               min, max - min, min_diff, max_diff);
        break;
      }
      middle_diff = diff(middle, var_cnst);

      if (middle_diff < 0) {
        CDEBUG0(surf_lagrange_dichotomy, "Increasing min");
        min = middle;
        overall_error = max_diff - middle_diff;
        min_diff = middle_diff;
/* 	SHOW_EXPR(overall_error); */
      } else if (middle_diff > 0) {
        CDEBUG0(surf_lagrange_dichotomy, "Decreasing max");
        max = middle;
        overall_error = max_diff - middle_diff;
        max_diff = middle_diff;
/* 	SHOW_EXPR(overall_error); */
      } else {
        overall_error = 0;
/* 	SHOW_EXPR(overall_error); */
      }
    } else if (min_diff == 0) {
      max = min;
      overall_error = 0;
/*       SHOW_EXPR(overall_error); */
    } else if (max_diff == 0) {
      min = max;
      overall_error = 0;
/*       SHOW_EXPR(overall_error); */
    } else if (min_diff > 0 && max_diff < 0) {
      CWARN0(surf_lagrange_dichotomy,
             "The impossible happened, partial_diff(min) > 0 && partial_diff(max) < 0");
      abort();
    } else {
      CWARN2(surf_lagrange_dichotomy,
             "diffmin (%1.20f) or diffmax (%1.20f) are something I don't know, taking no action.",
             min_diff, max_diff);
      abort();
    }
  }

  CDEBUG1(surf_lagrange_dichotomy, "returning %e", (min + max) / 2.0);
  XBT_OUT;
  return ((min + max) / 2.0);
}

static double partial_diff_lambda(double lambda, void *param_cnst)
{

  int j;
  xbt_swag_t elem_list = NULL;
  lmm_element_t elem = NULL;
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = (lmm_constraint_t) param_cnst;
  double diff = 0.0;
  double sigma_i = 0.0;

  XBT_IN;
  elem_list = &(cnst->element_set);

  CDEBUG1(surf_lagrange_dichotomy, "Computing diff of cnst (%p)", cnst);

  xbt_swag_foreach(elem, elem_list) {
    var = elem->variable;
    if (var->weight <= 0)
      continue;

    CDEBUG1(surf_lagrange_dichotomy, "Computing sigma_i for var (%p)", var);
    // Initialize the summation variable
    sigma_i = 0.0;

    // Compute sigma_i 
    for (j = 0; j < var->cnsts_number; j++) {
      sigma_i += (var->cnsts[j].constraint)->lambda;
    }

    //add mu_i if this flow has a RTT constraint associated
    if (var->bound > 0)
      sigma_i += var->mu;

    //replace value of cnst->lambda by the value of parameter lambda
    sigma_i = (sigma_i - cnst->lambda) + lambda;

    diff += -var->func_fpi(var, sigma_i);
  }


  diff += cnst->bound;

  CDEBUG3(surf_lagrange_dichotomy,
          "d D/d lambda for cnst (%p) at %1.20f = %1.20f", cnst, lambda,
          diff);
  XBT_OUT;
  return diff;
}

/** \brief Attribute the value bound to var->bound.
 * 
 *  \param func_fpi  inverse of the partial differential of f (f prime inverse, (f')^{-1})
 * 
 *  Set default functions to the ones passed as parameters. This is a polimorfism in C pure, enjoy the roots of programming.
 *
 */
void lmm_set_default_protocol_function(double (*func_f)





                                        
                                       (lmm_variable_t var, double x),
                                       double (*func_fp) (lmm_variable_t var,
                                                          double x),
                                       double (*func_fpi) (lmm_variable_t var,
                                                           double x))
{
  func_f_def = func_f;
  func_fp_def = func_fp;
  func_fpi_def = func_fpi;
}


/**************** Vegas and Reno functions *************************/
/*
 * NOTE for Reno: all functions consider the network
 * coeficient (alpha) equal to 1.
 */

/*
 * For Vegas: $f(x) = \alpha D_f\ln(x)$
 * Therefore: $fp(x) = \frac{\alpha D_f}{x}$
 * Therefore: $fpi(x) = \frac{\alpha D_f}{x}$
 */
#define VEGAS_SCALING 1000.0

double func_vegas_f(lmm_variable_t var, double x)
{
  xbt_assert1(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return VEGAS_SCALING * var->weight * log(x);
}

double func_vegas_fp(lmm_variable_t var, double x)
{
  xbt_assert1(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return VEGAS_SCALING * var->weight / x;
}

double func_vegas_fpi(lmm_variable_t var, double x)
{
  xbt_assert1(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return var->weight / (x / VEGAS_SCALING);
}

/*
 * For Reno:  $f(x) = \frac{\sqrt{3/2}}{D_f} atan(\sqrt{3/2}D_f x)$
 * Therefore: $fp(x)  = \frac{3}{3 D_f^2 x^2+2}$
 * Therefore: $fpi(x)  = \sqrt{\frac{1}{{D_f}^2 x} - \frac{2}{3{D_f}^2}}$
 */
#define RENO_SCALING 1.0
double func_reno_f(lmm_variable_t var, double x)
{
  xbt_assert0(var->weight > 0.0, "Don't call me with stupid values!");

  return RENO_SCALING * sqrt(3.0 / 2.0) / var->weight * atan(sqrt(3.0 / 2.0) *
                                                             var->weight * x);
}

double func_reno_fp(lmm_variable_t var, double x)
{
  return RENO_SCALING * 3.0 / (3.0 * var->weight * var->weight * x * x + 2.0);
}

double func_reno_fpi(lmm_variable_t var, double x)
{
  double res_fpi;

  xbt_assert0(var->weight > 0.0, "Don't call me with stupid values!");
  xbt_assert0(x > 0.0, "Don't call me with stupid values!");

  res_fpi =
    1.0 / (var->weight * var->weight * (x / RENO_SCALING)) -
    2.0 / (3.0 * var->weight * var->weight);
  if (res_fpi <= 0.0)
    return 0.0;
/*   xbt_assert0(res_fpi>0.0,"Don't call me with stupid values!"); */
  return sqrt(res_fpi);
}


/* Implementing new Reno-2
 * For Reno-2:  $f(x)   = U_f(x_f) = \frac{{2}{D_f}}*ln(2+x*D_f)$
 * Therefore:   $fp(x)  = 2/(Weight*x + 2)
 * Therefore:   $fpi(x) = (2*Weight)/x - 4
 */
#define RENO2_SCALING 1.0
double func_reno2_f(lmm_variable_t var, double x)
{
  xbt_assert0(var->weight > 0.0, "Don't call me with stupid values!");
  return RENO2_SCALING * (1.0 / var->weight) * log((x * var->weight) /
                                                   (2.0 * x * var->weight +
                                                    3.0));
}

double func_reno2_fp(lmm_variable_t var, double x)
{
  return RENO2_SCALING * 3.0 / (var->weight * x *
                                (2.0 * var->weight * x + 3.0));
}

double func_reno2_fpi(lmm_variable_t var, double x)
{
  double res_fpi;
  double tmp;

  xbt_assert0(x > 0.0, "Don't call me with stupid values!");
  tmp = x * var->weight * var->weight;
  res_fpi = tmp * (9.0 * x + 24.0);

  if (res_fpi <= 0.0)
    return 0.0;

  res_fpi = RENO2_SCALING * (-3.0 * tmp + sqrt(res_fpi)) / (4.0 * tmp);
  return res_fpi;
}
