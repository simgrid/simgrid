/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Modeling the proportional fairness using the Lagrangian Optimization Approach. For a detailed description see:
 * "ssh://username@scm.gforge.inria.fr/svn/memo/people/pvelho/lagrange/ppf.ps".
 */
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_lagrange, surf, "Logging specific to SURF (lagrange)");
XBT_LOG_NEW_SUBCATEGORY(surf_lagrange_dichotomy, surf_lagrange, "Logging specific to SURF (lagrange dichotomy)");

static constexpr double VEGAS_SCALING = 1000.0;
static constexpr double RENO_SCALING  = 1.0;
static constexpr double RENO2_SCALING = 1.0;

namespace simgrid {
namespace kernel {
namespace lmm {

System* make_new_lagrange_system(bool selective_update)
{
  return new Lagrange(selective_update);
}

bool Lagrange::check_feasible(bool warn)
{
  for (Constraint const& cnst : active_constraint_set) {
    double tmp = 0;
    for (Element const& elem : cnst.enabled_element_set_) {
      Variable* var = elem.variable;
      xbt_assert(var->sharing_weight_ > 0);
      tmp += var->value_;
    }

    if (double_positive(tmp - cnst.bound_, sg_maxmin_precision)) {
      if (warn)
        XBT_WARN("The link (%p) is over-used. Expected less than %f and got %f", &cnst, cnst.bound_, tmp);
      return false;
    }
    XBT_DEBUG("Checking feasability for constraint (%p): sat = %f, lambda = %f ", &cnst, tmp - cnst.bound_,
              cnst.lambda_);
  }

  for (Variable const& var : variable_set) {
    if (not var.sharing_weight_)
      break;
    if (var.bound_ < 0)
      continue;
    XBT_DEBUG("Checking feasability for variable (%p): sat = %f mu = %f", &var, var.value_ - var.bound_, var.mu_);

    if (double_positive(var.value_ - var.bound_, sg_maxmin_precision)) {
      if (warn)
        XBT_WARN("The variable (%p) is too large. Expected less than %f and got %f", &var, var.bound_, var.value_);
      return false;
    }
  }
  return true;
}

double Lagrange::new_value(const Variable& var)
{
  double tmp = 0;

  for (Element const& elem : var.cnsts_) {
    tmp += elem.constraint->lambda_;
  }
  if (var.bound_ > 0)
    tmp += var.mu_;
  XBT_DEBUG("\t Working on var (%p). cost = %e; Weight = %e", &var, tmp, var.sharing_weight_);
  // uses the partial differential inverse function
  return func_fpi(var, tmp);
}

double Lagrange::new_mu(const Variable& var)
{
  double mu_i    = 0.0;
  double sigma_i = 0.0;

  for (Element const& elem : var.cnsts_) {
    sigma_i += elem.constraint->lambda_;
  }
  mu_i = func_fp(var, var.bound_) - sigma_i;
  if (mu_i < 0.0)
    return 0.0;
  return mu_i;
}

double Lagrange::dual_objective()
{
  double obj = 0.0;

  for (Variable const& var : variable_set) {
    double sigma_i = 0.0;

    if (not var.sharing_weight_)
      break;

    for (Element const& elem : var.cnsts_)
      sigma_i += elem.constraint->lambda_;

    if (var.bound_ > 0)
      sigma_i += var.mu_;

    XBT_DEBUG("var %p : sigma_i = %1.20f", &var, sigma_i);

    obj += func_f(var, func_fpi(var, sigma_i)) - sigma_i * func_fpi(var, sigma_i);

    if (var.bound_ > 0)
      obj += var.mu_ * var.bound_;
  }

  for (Constraint const& cnst : active_constraint_set)
    obj += cnst.lambda_ * cnst.bound_;

  return obj;
}

// solves the proportional fairness using a Lagrangian optimization with dichotomy step
void Lagrange::lagrange_solve()
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
    print();
  }

  if (not modified_)
    return;

  /* Initialize lambda. */
  for (Constraint& cnst : active_constraint_set) {
    cnst.lambda_     = 1.0;
    cnst.new_lambda_ = 2.0;
    XBT_DEBUG("#### cnst(%p)->lambda :  %e", &cnst, cnst.lambda_);
  }

  /*
   * Initialize the active variables. Initialize mu.
   */
  for (Variable& var : variable_set) {
    if (not var.sharing_weight_)
      var.value_ = 0.0;
    else {
      if (var.bound_ < 0.0) {
        XBT_DEBUG("#### NOTE var(%p) is a boundless variable", &var);
        var.mu_ = -1.0;
      } else {
        var.mu_     = 1.0;
        var.new_mu_ = 2.0;
      }
      var.value_ = new_value(var);
      XBT_DEBUG("#### var(%p) ->weight :  %e", &var, var.sharing_weight_);
      XBT_DEBUG("#### var(%p) ->mu :  %e", &var, var.mu_);
      XBT_DEBUG("#### var(%p) ->weight: %e", &var, var.sharing_weight_);
      XBT_DEBUG("#### var(%p) ->bound: %e", &var, var.bound_);
      auto weighted = std::find_if(begin(var.cnsts_), end(var.cnsts_),
                                   [](Element const& x) { return x.consumption_weight != 0.0; });
      if (weighted == end(var.cnsts_))
        var.value_ = 1.0;
    }
  }

  /*  Compute dual objective. */
  double obj = dual_objective();

  /* While doesn't reach a minimum error or a number maximum of iterations. */
  int iteration = 0;
  while (overall_modification > epsilon_min_error && iteration < max_iterations) {
    iteration++;
    XBT_DEBUG("************** ITERATION %d **************", iteration);
    XBT_DEBUG("-------------- Gradient Descent ----------");

    /* Improve the value of mu_i */
    for (Variable& var : variable_set) {
      if (var.sharing_weight_ && var.bound_ >= 0) {
        XBT_DEBUG("Working on var (%p)", &var);
        var.new_mu_ = new_mu(var);
        XBT_DEBUG("Updating mu : var->mu (%p) : %1.20f -> %1.20f", &var, var.mu_, var.new_mu_);
        var.mu_ = var.new_mu_;

        double new_obj = dual_objective();
        XBT_DEBUG("Improvement for Objective (%g -> %g) : %g", obj, new_obj, obj - new_obj);
        xbt_assert(obj - new_obj >= -epsilon_min_error, "Our gradient sucks! (%1.20f)", obj - new_obj);
        obj = new_obj;
      }
    }

    /* Improve the value of lambda_i */
    for (Constraint& cnst : active_constraint_set) {
      XBT_DEBUG("Working on cnst (%p)", &cnst);
      cnst.new_lambda_ = dichotomy(cnst.lambda_, cnst, dichotomy_min_error);
      XBT_DEBUG("Updating lambda : cnst->lambda (%p) : %1.20f -> %1.20f", &cnst, cnst.lambda_, cnst.new_lambda_);
      cnst.lambda_ = cnst.new_lambda_;

      double new_obj = dual_objective();
      XBT_DEBUG("Improvement for Objective (%g -> %g) : %g", obj, new_obj, obj - new_obj);
      xbt_assert(obj - new_obj >= -epsilon_min_error, "Our gradient sucks! (%1.20f)", obj - new_obj);
      obj = new_obj;
    }

    /* Now computes the values of each variable (@rho) based on the values of @lambda and @mu. */
    XBT_DEBUG("-------------- Check convergence ----------");
    overall_modification = 0;
    for (Variable& var : variable_set) {
      if (var.sharing_weight_ <= 0)
        var.value_ = 0.0;
      else {
        double tmp = new_value(var);

        overall_modification = std::max(overall_modification, fabs(var.value_ - tmp));

        var.value_ = tmp;
        XBT_DEBUG("New value of var (%p)  = %e, overall_modification = %e", &var, var.value_, overall_modification);
      }
    }

    XBT_DEBUG("-------------- Check feasability ----------");
    if (not check_feasible(false))
      overall_modification = 1.0;
    XBT_DEBUG("Iteration %d: overall_modification : %f", iteration, overall_modification);
  }

  check_feasible(true);

  if (overall_modification <= epsilon_min_error) {
    XBT_DEBUG("The method converges in %d iterations.", iteration);
  }
  if (iteration >= max_iterations) {
    XBT_DEBUG("Method reach %d iterations, which is the maximum number of iterations allowed.", iteration);
  }

  if (XBT_LOG_ISENABLED(surf_lagrange, xbt_log_priority_debug)) {
    print();
  }
}

/*
 * Returns a double value corresponding to the result of a dichotomy process with respect to a given
 * variable/constraint (@mu in the case of a variable or @lambda in case of a constraint) and a initial value init.
 *
 * @param init initial value for @mu or @lambda
 * @param diff a function that computes the differential of with respect a @mu or @lambda
 * @param var_cnst a pointer to a variable or constraint
 * @param min_erro a minimum error tolerated
 *
 * @return a double corresponding to the result of the dichotomy process
 */
double Lagrange::dichotomy(double init, const Constraint& cnst, double min_error)
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

  diff_0 = partial_diff_lambda(1e-16, cnst);
  if (diff_0 >= 0) {
    XBT_CDEBUG(surf_lagrange_dichotomy, "returning 0.0 (diff = %e)", diff_0);
    XBT_OUT();
    return 0.0;
  }

  double min_diff = partial_diff_lambda(min, cnst);
  double max_diff = partial_diff_lambda(max, cnst);

  while (overall_error > min_error) {
    XBT_CDEBUG(surf_lagrange_dichotomy, "[min, max] = [%1.20f, %1.20f] || diffmin, diffmax = %1.20f, %1.20f", min, max,
               min_diff, max_diff);

    if (min_diff > 0 && max_diff > 0) {
      if (min == max) {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Decreasing min");
        min      = min / 2.0;
        min_diff = partial_diff_lambda(min, cnst);
      } else {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Decreasing max");
        max      = min;
        max_diff = min_diff;
      }
    } else if (min_diff < 0 && max_diff < 0) {
      if (min == max) {
        XBT_CDEBUG(surf_lagrange_dichotomy, "Increasing max");
        max      = max * 2.0;
        max_diff = partial_diff_lambda(max, cnst);
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
      middle_diff = partial_diff_lambda(middle, cnst);

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

double Lagrange::partial_diff_lambda(double lambda, const Constraint& cnst)
{
  double diff           = 0.0;

  XBT_IN();

  XBT_CDEBUG(surf_lagrange_dichotomy, "Computing diff of cnst (%p)", &cnst);

  for (Element const& elem : cnst.enabled_element_set_) {
    Variable& var = *elem.variable;
    xbt_assert(var.sharing_weight_ > 0);
    XBT_CDEBUG(surf_lagrange_dichotomy, "Computing sigma_i for var (%p)", &var);
    // Initialize the summation variable
    double sigma_i = 0.0;

    // Compute sigma_i
    for (Element const& elem2 : var.cnsts_)
      sigma_i += elem2.constraint->lambda_;

    // add mu_i if this flow has a RTT constraint associated
    if (var.bound_ > 0)
      sigma_i += var.mu_;

    // replace value of cnst.lambda by the value of parameter lambda
    sigma_i = (sigma_i - cnst.lambda_) + lambda;

    diff += -func_fpi(var, sigma_i);
  }

  diff += cnst.bound_;

  XBT_CDEBUG(surf_lagrange_dichotomy, "d D/d lambda for cnst (%p) at %1.20f = %1.20f", &cnst, lambda, diff);
  XBT_OUT();
  return diff;
}

/** @brief Attribute the value bound to var->bound.
 *
 *  @param f    function (f)
 *  @param fp   partial differential of f (f prime, (f'))
 *  @param fpi  inverse of the partial differential of f (f prime inverse, (f')^{-1})
 *
 *  Set default functions to the ones passed as parameters.
 */
void Lagrange::set_default_protocol_function(double (*f)(const Variable& var, double x),
                                             double (*fp)(const Variable& var, double x),
                                             double (*fpi)(const Variable& var, double x))
{
  func_f   = f;
  func_fp  = fp;
  func_fpi = fpi;
}

double (*Lagrange::func_f)(const Variable&, double);
double (*Lagrange::func_fp)(const Variable&, double);
double (*Lagrange::func_fpi)(const Variable&, double);

/**************** Vegas and Reno functions *************************/
/* NOTE for Reno: all functions consider the network coefficient (alpha) equal to 1. */

/*
 * For Vegas: $f(x) = @alpha D_f@ln(x)$
 * Therefore: $fp(x) = @frac{@alpha D_f}{x}$
 * Therefore: $fpi(x) = @frac{@alpha D_f}{x}$
 */
double func_vegas_f(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return VEGAS_SCALING * var.sharing_weight_ * log(x);
}

double func_vegas_fp(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return VEGAS_SCALING * var.sharing_weight_ / x;
}

double func_vegas_fpi(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values! (%1.20f)", x);
  return var.sharing_weight_ / (x / VEGAS_SCALING);
}

/*
 * For Reno:  $f(x) = @frac{@sqrt{3/2}}{D_f} atan(@sqrt{3/2}D_f x)$
 * Therefore: $fp(x)  = @frac{3}{3 D_f^2 x^2+2}$
 * Therefore: $fpi(x)  = @sqrt{@frac{1}{{D_f}^2 x} - @frac{2}{3{D_f}^2}}$
 */
double func_reno_f(const Variable& var, double x)
{
  xbt_assert(var.sharing_weight_ > 0.0, "Don't call me with stupid values!");

  return RENO_SCALING * sqrt(3.0 / 2.0) / var.sharing_weight_ * atan(sqrt(3.0 / 2.0) * var.sharing_weight_ * x);
}

double func_reno_fp(const Variable& var, double x)
{
  return RENO_SCALING * 3.0 / (3.0 * var.sharing_weight_ * var.sharing_weight_ * x * x + 2.0);
}

double func_reno_fpi(const Variable& var, double x)
{
  double res_fpi;

  xbt_assert(var.sharing_weight_ > 0.0, "Don't call me with stupid values!");
  xbt_assert(x > 0.0, "Don't call me with stupid values!");

  res_fpi = 1.0 / (var.sharing_weight_ * var.sharing_weight_ * (x / RENO_SCALING)) -
            2.0 / (3.0 * var.sharing_weight_ * var.sharing_weight_);
  if (res_fpi <= 0.0)
    return 0.0;
  return sqrt(res_fpi);
}

/* Implementing new Reno-2
 * For Reno-2:  $f(x)   = U_f(x_f) = @frac{{2}{D_f}}*ln(2+x*D_f)$
 * Therefore:   $fp(x)  = 2/(Weight*x + 2)
 * Therefore:   $fpi(x) = (2*Weight)/x - 4
 */
double func_reno2_f(const Variable& var, double x)
{
  xbt_assert(var.sharing_weight_ > 0.0, "Don't call me with stupid values!");
  return RENO2_SCALING * (1.0 / var.sharing_weight_) *
         log((x * var.sharing_weight_) / (2.0 * x * var.sharing_weight_ + 3.0));
}

double func_reno2_fp(const Variable& var, double x)
{
  return RENO2_SCALING * 3.0 / (var.sharing_weight_ * x * (2.0 * var.sharing_weight_ * x + 3.0));
}

double func_reno2_fpi(const Variable& var, double x)
{
  xbt_assert(x > 0.0, "Don't call me with stupid values!");
  double tmp     = x * var.sharing_weight_ * var.sharing_weight_;
  double res_fpi = tmp * (9.0 * x + 24.0);

  if (res_fpi <= 0.0)
    return 0.0;

  res_fpi = RENO2_SCALING * (-3.0 * tmp + sqrt(res_fpi)) / (4.0 * tmp);
  return res_fpi;
}
}
}
}
