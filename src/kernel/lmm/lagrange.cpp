/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Modeling the proportional fairness using the Lagrangian Optimization Approach. For a detailed description see:
 * "ssh://username@scm.gforge.inria.fr/svn/memo/people/pvelho/lagrange/ppf.ps".
 */
#include "src/kernel/lmm/maxmin.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <algorithm>
#include <cstdlib>
#ifndef MATH
#include <cmath>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_lagrange, surf, "Logging specific to SURF (lagrange)");
XBT_LOG_NEW_SUBCATEGORY(surf_lagrange_dichotomy, surf_lagrange, "Logging specific to SURF (lagrange dichotomy)");

#define SHOW_EXPR(expr) XBT_CDEBUG(surf_lagrange, #expr " = %g", expr);
#define VEGAS_SCALING 1000.0
#define RENO_SCALING 1.0
#define RENO2_SCALING 1.0

namespace simgrid {
namespace kernel {
namespace lmm {

double (*func_f_def)(const Variable&, double);
double (*func_fp_def)(const Variable&, double);
double (*func_fpi_def)(const Variable&, double);

/*
 * Local prototypes to implement the Lagrangian optimization with optimal step, also called dichotomy.
 */
// solves the proportional fairness using a Lagrangian optimization with dichotomy step
void lagrange_solve(lmm_system_t sys);
// computes the value of the dichotomy using a initial values, init, with a specific variable or constraint
static double dichotomy(double init, double diff(double, const Constraint&), const Constraint& cnst, double min_error);
// computes the value of the differential of constraint cnst applied to lambda
static double partial_diff_lambda(double lambda, const Constraint& cnst);

template <class CnstList, class VarList>
static int __check_feasible(const CnstList& cnst_list, const VarList& var_list, int warn)
{
  for (Constraint const& cnst : cnst_list) {
    double tmp = 0;
    for (Element const& elem : cnst.enabled_element_set) {
      lmm_variable_t var = elem.variable;
      xbt_assert(var->sharing_weight > 0);
      tmp += var->value;
    }

    if (double_positive(tmp - cnst.bound, sg_maxmin_precision)) {
      if (warn)
        XBT_WARN("The link (%p) is over-used. Expected less than %f and got %f", &cnst, cnst.bound, tmp);
      return 0;
    }
    XBT_DEBUG("Checking feasability for constraint (%p): sat = %f, lambda = %f ", &cnst, tmp - cnst.bound, cnst.lambda);
  }

  for (Variable const& var : var_list) {
    if (not var.sharing_weight)
      break;
    if (var.bound < 0)
      continue;
    XBT_DEBUG("Checking feasability for variable (%p): sat = %f mu = %f", &var, var.value - var.bound, var.mu);

    if (double_positive(var.value - var.bound, sg_maxmin_precision)) {
      if (warn)
        XBT_WARN("The variable (%p) is too large. Expected less than %f and got %f", &var, var.bound, var.value);
      return 0;
    }
  }
  return 1;
}

static double new_value(const Variable& var)
{
  double tmp = 0;

  for (Element const& elem : var.cnsts) {
    tmp += elem.constraint->lambda;
  }
  if (var.bound > 0)
    tmp += var.mu;
  XBT_DEBUG("\t Working on var (%p). cost = %e; Weight = %e", &var, tmp, var.sharing_weight);
  // uses the partial differential inverse function
  return var.func_fpi(var, tmp);
}

static double new_mu(const Variable& var)
{
  double mu_i    = 0.0;
  double sigma_i = 0.0;

  for (Element const& elem : var.cnsts) {
    sigma_i += elem.constraint->lambda;
  }
  mu_i = var.func_fp(var, var.bound) - sigma_i;
  if (mu_i < 0.0)
    return 0.0;
  return mu_i;
}

template <class VarList, class CnstList>
static double dual_objective(const VarList& var_list, const CnstList& cnst_list)
{
  double obj = 0.0;

  for (Variable const& var : var_list) {
    double sigma_i = 0.0;

    if (not var.sharing_weight)
      break;

    for (Element const& elem : var.cnsts)
      sigma_i += elem.constraint->lambda;

    if (var.bound > 0)
      sigma_i += var.mu;

    XBT_DEBUG("var %p : sigma_i = %1.20f", &var, sigma_i);

    obj += var.func_f(var, var.func_fpi(var, sigma_i)) - sigma_i * var.func_fpi(var, sigma_i);

    if (var.bound > 0)
      obj += var.mu * var.bound;
  }

  for (Constraint const& cnst : cnst_list)
    obj += cnst.lambda * cnst.bound;

  return obj;
}

void lagrange_solve(lmm_system_t sys)
{
  /* Lagrange Variables. */
  int max_iterations       = 100;
  double epsilon_min_error = 0.00001; /* this is the precision on the objective function so it's none of the
                                         configurable values and this value is the legacy one */
  double dichotomy_min_error  = 1e-14;
  double overall_modification = 1;

  XBT_DEBUG("Iterative method configuration snapshot =====>");
  XBT_DEBUG("#### Maximum number of iterations        : %d", max_iterations);
  XBT_DEBUG("#### Minimum error tolerated             : %e", epsilon_min_error);
  XBT_DEBUG("#### Minimum error tolerated (dichotomy) : %e", dichotomy_min_error);

  if (XBT_LOG_ISENABLED(surf_lagrange, xbt_log_priority_debug)) {
    sys->print();
  }

  if (not sys->modified)
    return;

  /* Initialize lambda. */
  auto& cnst_list = sys->active_constraint_set;
  for (Constraint& cnst : cnst_list) {
    cnst.lambda     = 1.0;
    cnst.new_lambda = 2.0;
    XBT_DEBUG("#### cnst(%p)->lambda :  %e", &cnst, cnst.lambda);
  }

  /*
   * Initialize the var_list variable with only the active variables. Initialize mu.
   */
  auto& var_list = sys->variable_set;
  for (Variable& var : var_list) {
    if (not var.sharing_weight)
      var.value = 0.0;
    else {
      if (var.bound < 0.0) {
        XBT_DEBUG("#### NOTE var(%p) is a boundless variable", &var);
        var.mu = -1.0;
      } else {
        var.mu     = 1.0;
        var.new_mu = 2.0;
      }
      var.value = new_value(var);
      XBT_DEBUG("#### var(%p) ->weight :  %e", &var, var.sharing_weight);
      XBT_DEBUG("#### var(%p) ->mu :  %e", &var, var.mu);
      XBT_DEBUG("#### var(%p) ->weight: %e", &var, var.sharing_weight);
      XBT_DEBUG("#### var(%p) ->bound: %e", &var, var.bound);
      auto weighted =
          std::find_if(begin(var.cnsts), end(var.cnsts), [](Element const& x) { return x.consumption_weight != 0.0; });
      if (weighted == end(var.cnsts))
        var.value = 1.0;
    }
  }

  /*  Compute dual objective. */
  double obj = dual_objective(var_list, cnst_list);

  /* While doesn't reach a minimum error or a number maximum of iterations. */
  int iteration = 0;
  while (overall_modification > epsilon_min_error && iteration < max_iterations) {
    iteration++;
    XBT_DEBUG("************** ITERATION %d **************", iteration);
    XBT_DEBUG("-------------- Gradient Descent ----------");

    /* Improve the value of mu_i */
    for (Variable& var : var_list) {
      if (var.sharing_weight && var.bound >= 0) {
        XBT_DEBUG("Working on var (%p)", &var);
        var.new_mu = new_mu(var);
        XBT_DEBUG("Updating mu : var->mu (%p) : %1.20f -> %1.20f", &var, var.mu, var.new_mu);
        var.mu = var.new_mu;

        double new_obj = dual_objective(var_list, cnst_list);
        XBT_DEBUG("Improvement for Objective (%g -> %g) : %g", obj, new_obj, obj - new_obj);
        xbt_assert(obj - new_obj >= -epsilon_min_error, "Our gradient sucks! (%1.20f)", obj - new_obj);
        obj = new_obj;
      }
    }

    /* Improve the value of lambda_i */
    for (Constraint& cnst : cnst_list) {
      XBT_DEBUG("Working on cnst (%p)", &cnst);
      cnst.new_lambda = dichotomy(cnst.lambda, partial_diff_lambda, cnst, dichotomy_min_error);
      XBT_DEBUG("Updating lambda : cnst->lambda (%p) : %1.20f -> %1.20f", &cnst, cnst.lambda, cnst.new_lambda);
      cnst.lambda = cnst.new_lambda;

      double new_obj = dual_objective(var_list, cnst_list);
      XBT_DEBUG("Improvement for Objective (%g -> %g) : %g", obj, new_obj, obj - new_obj);
      xbt_assert(obj - new_obj >= -epsilon_min_error, "Our gradient sucks! (%1.20f)", obj - new_obj);
      obj = new_obj;
    }

    /* Now computes the values of each variable (\rho) based on the values of \lambda and \mu. */
    XBT_DEBUG("-------------- Check convergence ----------");
    overall_modification = 0;
    for (Variable& var : var_list) {
      if (var.sharing_weight <= 0)
        var.value = 0.0;
      else {
        double tmp = new_value(var);

        overall_modification = std::max(overall_modification, fabs(var.value - tmp));

        var.value = tmp;
        XBT_DEBUG("New value of var (%p)  = %e, overall_modification = %e", &var, var.value, overall_modification);
      }
    }

    XBT_DEBUG("-------------- Check feasability ----------");
    if (not __check_feasible(cnst_list, var_list, 0))
      overall_modification = 1.0;
    XBT_DEBUG("Iteration %d: overall_modification : %f", iteration, overall_modification);
  }

  __check_feasible(cnst_list, var_list, 1);

  if (overall_modification <= epsilon_min_error) {
    XBT_DEBUG("The method converges in %d iterations.", iteration);
  }
  if (iteration >= max_iterations) {
    XBT_DEBUG("Method reach %d iterations, which is the maximum number of iterations allowed.", iteration);
  }

  if (XBT_LOG_ISENABLED(surf_lagrange, xbt_log_priority_debug)) {
    sys->print();
  }
}

/*
 * Returns a double value corresponding to the result of a dichotomy process with respect to a given
 * variable/constraint (\mu in the case of a variable or \lambda in case of a constraint) and a initial value init.
 *
 * @param init initial value for \mu or \lambda
 * @param diff a function that computes the differential of with respect a \mu or \lambda
 * @param var_cnst a pointer to a variable or constraint
 * @param min_erro a minimum error tolerated
 *
 * @return a double corresponding to the result of the dichotomy process
 */
static double dichotomy(double init, double diff(double, const Constraint&), const Constraint& cnst, double min_error)
{
  double min = init;
  double max = init;
  double overall_error;
  double middle;
  double middle_diff;
  double diff_0 = 0.0;

  XBT_IN();

  if (fabs(init) < 1e-20) {
    min = 0.5;
    max = 0.5;
  }

  overall_error = 1;

  diff_0 = diff(1e-16, cnst);
  if (diff_0 >= 0) {
    XBT_CDEBUG(surf_lagrange_dichotomy, "returning 0.0 (diff = %e)", diff_0);
    XBT_OUT();
    return 0.0;
  }

  double min_diff = diff(min, cnst);
  double max_diff = diff(max, cnst);

  while (overall_error > min_error) {
    XBT_CDEBUG(surf_lagrange_dichotomy, "[min, max] = [%1.20f, %1.20f] || diffmin, diffmax = %1.20f, %1.20f", min, max,
               min_diff, max_diff);

    if (min_diff > 0 && max_diff > 0) {
      if (min == max) {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Decreasing min");
        min      = min / 2.0;
        min_diff = diff(min, cnst);
      } else {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Decreasing max");
        max      = min;
        max_diff = min_diff;
      }
    } else if (min_diff < 0 && max_diff < 0) {
      if (min == max) {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Increasing max");
        max      = max * 2.0;
        max_diff = diff(max, cnst);
      } else {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Increasing min");
        min      = max;
        min_diff = max_diff;
      }
    } else if (min_diff < 0 && max_diff > 0) {
      middle = (max + min) / 2.0;
      XBT_CDEBUG(surf_lagrange_dichotomy, "Trying (max+min)/2 : %1.20f", middle);

      if ((fabs(min - middle) < 1e-20) || (fabs(max - middle) < 1e-20)) {
        XBT_CWARN(surf_lagrange_dichotomy,
                  "Cannot improve the convergence! min=max=middle=%1.20f, diff = %1.20f."
                  " Reaching the 'double' limits. Maybe scaling your function would help ([%1.20f,%1.20f]).",
                  min, max - min, min_diff, max_diff);
        break;
      }
      middle_diff = diff(middle, cnst);

      if (middle_diff < 0) {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Increasing min");
        min           = middle;
        overall_error = max_diff - middle_diff;
        min_diff      = middle_diff;
      } else if (middle_diff > 0) {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Decreasing max");
        max           = middle;
        overall_error = max_diff - middle_diff;
        max_diff      = middle_diff;
      } else {
        overall_error = 0;
      }
    } else if (fabs(min_diff) < 1e-20) {
      max           = min;
      overall_error = 0;
    } else if (fabs(max_diff) < 1e-20) {
      min           = max;
      overall_error = 0;
    } else if (min_diff > 0 && max_diff < 0) {
      XBT_CWARN(surf_lagrange_dichotomy, "The impossible happened, partial_diff(min) > 0 && partial_diff(max) < 0");
      xbt_abort();
    } else {
      XBT_CWARN(surf_lagrange_dichotomy,
                "diffmin (%1.20f) or diffmax (%1.20f) are something I don't know, taking no action.", min_diff,
                max_diff);
      xbt_abort();
    }
  }

  XBT_CDEBUG(surf_lagrange_dichotomy, "returning %e", (min + max) / 2.0);
  XBT_OUT();
  return ((min + max) / 2.0);
}

static double partial_diff_lambda(double lambda, const Constraint& cnst)
{
  double diff           = 0.0;

  XBT_IN();

  XBT_CDEBUG(surf_lagrange_dichotomy, "Computing diff of cnst (%p)", &cnst);

  for (Element const& elem : cnst.enabled_element_set) {
    Variable& var = *elem.variable;
    xbt_assert(var.sharing_weight > 0);
    XBT_CDEBUG(surf_lagrange_dichotomy, "Computing sigma_i for var (%p)", &var);
    // Initialize the summation variable
    double sigma_i = 0.0;

    // Compute sigma_i
    for (Element const& elem2 : var.cnsts)
      sigma_i += elem2.constraint->lambda;

    // add mu_i if this flow has a RTT constraint associated
    if (var.bound > 0)
      sigma_i += var.mu;

    // replace value of cnst.lambda by the value of parameter lambda
    sigma_i = (sigma_i - cnst.lambda) + lambda;

    diff += -var.func_fpi(var, sigma_i);
  }

  diff += cnst.bound;

  XBT_CDEBUG(surf_lagrange_dichotomy, "d D/d lambda for cnst (%p) at %1.20f = %1.20f", &cnst, lambda, diff);
  XBT_OUT();
  return diff;
}

/** \brief Attribute the value bound to var->bound.
 *
 *  \param func_fpi  inverse of the partial differential of f (f prime inverse, (f')^{-1})
 *
 *  Set default functions to the ones passed as parameters. This is a polymorphism in C pure, enjoy the roots of
 *  programming.
 *
 */
void set_default_protocol_function(double (*func_f)(const Variable& var, double x),
                                   double (*func_fp)(const Variable& var, double x),
                                   double (*func_fpi)(const Variable& var, double x))
{
  func_f_def   = func_f;
  func_fp_def  = func_fp;
  func_fpi_def = func_fpi;
}

/**************** Vegas and Reno functions *************************/
/* NOTE for Reno: all functions consider the network coefficient (alpha) equal to 1. */

/*
 * For Vegas: $f(x) = \alpha D_f\ln(x)$
 * Therefore: $fp(x) = \frac{\alpha D_f}{x}$
 * Therefore: $fpi(x) = \frac{\alpha D_f}{x}$
 */
double func_vegas_f(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return VEGAS_SCALING * var.sharing_weight * log(x);
}

double func_vegas_fp(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return VEGAS_SCALING * var.sharing_weight / x;
}

double func_vegas_fpi(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return var.sharing_weight / (x / VEGAS_SCALING);
}

/*
 * For Reno:  $f(x) = \frac{\sqrt{3/2}}{D_f} atan(\sqrt{3/2}D_f x)$
 * Therefore: $fp(x)  = \frac{3}{3 D_f^2 x^2+2}$
 * Therefore: $fpi(x)  = \sqrt{\frac{1}{{D_f}^2 x} - \frac{2}{3{D_f}^2}}$
 */
double func_reno_f(const Variable& var, double x)
{
  xbt_assert(var.sharing_weight > 0.0, "Don't call me with stupid values!");

  return RENO_SCALING * sqrt(3.0 / 2.0) / var.sharing_weight * atan(sqrt(3.0 / 2.0) * var.sharing_weight * x);
}

double func_reno_fp(const Variable& var, double x)
{
  return RENO_SCALING * 3.0 / (3.0 * var.sharing_weight * var.sharing_weight * x * x + 2.0);
}

double func_reno_fpi(const Variable& var, double x)
{
  double res_fpi;

  xbt_assert(var.sharing_weight > 0.0, "Don't call me with stupid values!");
  xbt_assert(x > 0.0, "Don't call me with stupid values!");

  res_fpi = 1.0 / (var.sharing_weight * var.sharing_weight * (x / RENO_SCALING)) -
            2.0 / (3.0 * var.sharing_weight * var.sharing_weight);
  if (res_fpi <= 0.0)
    return 0.0;
  return sqrt(res_fpi);
}

/* Implementing new Reno-2
 * For Reno-2:  $f(x)   = U_f(x_f) = \frac{{2}{D_f}}*ln(2+x*D_f)$
 * Therefore:   $fp(x)  = 2/(Weight*x + 2)
 * Therefore:   $fpi(x) = (2*Weight)/x - 4
 */
double func_reno2_f(const Variable& var, double x)
{
  xbt_assert(var.sharing_weight > 0.0, "Don't call me with stupid values!");
  return RENO2_SCALING * (1.0 / var.sharing_weight) *
         log((x * var.sharing_weight) / (2.0 * x * var.sharing_weight + 3.0));
}

double func_reno2_fp(const Variable& var, double x)
{
  return RENO2_SCALING * 3.0 / (var.sharing_weight * x * (2.0 * var.sharing_weight * x + 3.0));
}

double func_reno2_fpi(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values!");
  double tmp     = x * var.sharing_weight * var.sharing_weight;
  double res_fpi = tmp * (9.0 * x + 24.0);

  if (res_fpi <= 0.0)
    return 0.0;

  res_fpi = RENO2_SCALING * (-3.0 * tmp + sqrt(res_fpi)) / (4.0 * tmp);
  return res_fpi;
}
}
}
}
