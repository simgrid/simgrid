/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_MAXMIN_H
#define _SURF_MAXMIN_H

#include "src/internal_config.h"
#include "xbt/misc.h"
#include "xbt/asserts.h"
#include "surf/datatypes.h"
#include <math.h>

/** @addtogroup SURF_lmm 
 * @details 
 * A linear maxmin solver to resolve inequations systems.
 * 
 * Most SimGrid model rely on a "fluid/steady-state" modeling that simulate the sharing of resources between actions at
 * relatively coarse-grain.  Such sharing is generally done by solving a set of linear inequations. Let's take an
 * example and assume we have the variables \f$x_1\f$, \f$x_2\f$, \f$x_3\f$, and \f$x_4\f$ . Let's say that \f$x_1\f$
 * and \f$x_2\f$ correspond to activities running and the same CPU \f$A\f$ whose capacity is \f$C_A\f$. In such a
 * case, we need to enforce:
 *
 *   \f[ x_1 + x_2 \leq C_A \f]
 *
 * Likewise, if \f$x_3\f$ (resp. \f$x_4\f$) corresponds to a network flow \f$F_3\f$ (resp. \f$F_4\f$) that goes through
 * a set of links \f$L_1\f$ and \f$L_2\f$ (resp. \f$L_2\f$ and \f$L_3\f$), then we need to enforce:
 *
 *   \f[ x_3  \leq C_{L_1} \f]
 *   \f[ x_3 + x_4 \leq C_{L_2} \f]
 *   \f[ x_4 \leq C_{L_3} \f]
 *
 * One could set every variable to 0 to make sure the constraints are satisfied but this would obviously not be very
 * realistic. A possible objective is to try to maximize the minimum of the \f$x_i\f$ . This ensures that all the
 * \f$x_i\f$ are positive and "as large as possible".
 *
 * This is called *max-min fairness* and is the most commonly used objective in SimGrid. Another possibility is to
 * maximize \f$\sum_if(x_i)\f$, where \f$f\f$ is a strictly increasing concave function.
 *
 * Constraint: 
 *  - bound (set)
 *  - shared (set)
 *  - usage (computed)
 *
 * Variable:
 *  - weight (set)
 *  - bound (set)
 *  - value (computed)
 *
 * Element:
 *  - value (set)
 * 
 * A possible system could be:
 * - three variables: `var1`, `var2`, `var3`
 * - two constraints: `cons1`, `cons2`
 * - four elements linking:
 *  - `elem1` linking `var1` and `cons1`
 *  - `elem2` linking `var2` and `cons1`
 *  - `elem3` linking `var2` and `cons2`
 *  - `elem4` linking `var3` and `cons2`
 * 
 * And the corresponding inequations will be:
 * 
 *     var1.value <= var1.bound
 *     var2.value <= var2.bound
 *     var3.value <= var3.bound
 *     var1.weight * var1.value * elem1.value + var2.weight * var2.value * elem2.value <= cons1.bound
 *     var2.weight * var2.value * elem3.value + var3.weight * var3.value * elem4.value <= cons2.bound
 * 
 * where `var1.value`, `var2.value` and `var3.value` are the unknown values.
 * 
 * If a constraint is not shared, the sum is replaced by a max. 
 * For example, a third non-shared constraint `cons3` and the associated elements `elem5` and `elem6` could write as: 
 *
 *     max( var1.weight * var1.value * elem5.value  ,  var3.weight * var3.value * elem6.value ) <= cons3.bound
 *
 * This is usefull for the sharing of resources for various models.
 * For instance, for the network model, each link is associated to a constraint and each communication to a variable.
 *
 * Implementation details
 *
 * For implementation reasons, we are interested in distinguishing variables that actually participate to the
 * computation of constraints, and those who are part of the equations but are stuck to zero.
 * We call enabled variables, those which var.weight is strictly positive. Zero-weight variables are called disabled
 * variables.
 * Unfortunately this concept of enabled/disabled variables intersects with active/inactive variable.
 * Semantically, the intent is similar, but the conditions under which a variable is active is slightly more strict
 * than the conditions for it to be enabled.
 * A variable is active only if its var.value is non-zero (and, by construction, its var.weight is non-zero).
 * In general, variables remain disabled after their creation, which often models an initialization phase (e.g. first
 * packet propagating in the network). Then, it is enabled by the corresponding model. Afterwards, the max-min solver
 * (lmm_solve()) activates it when appropriate. It is possible that the variable is again disabled, e.g. to model the
 * pausing of an action.
 *
 * Concurrency limit and maximum 
 * 
 * We call concurrency, the number of variables that can be enabled at any time for each constraint. 
 * From a model perspective, this "concurrency" often represents the number of actions that actually compete for one
 * constraint.
 * The LMM solver is able to limit the concurrency for each constraint, and to monitor its maximum value.
 * 
 * One may want to limit the concurrency of constraints for essentially three reasons:
 *  - Keep LMM system in a size that can be solved (it does not react very well with tens of thousands of variables per
 *    constraint)
 *  - Stay within parameters where the fluid model is accurate enough.      
 *  - Model serialization effects
 *
 * The concurrency limit can also be set to a negative value to disable concurrency limit. This can improve performance
 * slightly.
 * 
 * Overall, each constraint contains three fields related to concurrency:
 *  - concurrency_limit which is the limit enforced by the solver
 *  - concurrency_current which is the current concurrency
 *  - concurrency_maximum which is the observed maximum concurrency 
 *
 * Variables also have one field related to concurrency: concurrency_share. 
 * In effect, in some cases, one variable is involved multiple times (i.e. two elements) in a constraint.
 * For example, cross-traffic is modeled using 2 elements per constraint.
 * concurrency_share formally corresponds to the maximum number of elements that associate the variable and any given
 * constraint.
 */

XBT_PUBLIC_DATA(double) sg_maxmin_precision;
XBT_PUBLIC_DATA(double) sg_surf_precision;
 
static inline void double_update(double *variable, double value, double precision)
{
  //printf("Updating %g -= %g +- %g\n",*variable,value,precision);
  //xbt_assert(value==0  || value>precision);
  //Check that precision is higher than the machine-dependent size of the mantissa. If not, brutal rounding  may happen,
  //and the precision mechanism is not active...
  //xbt_assert(*variable< (2<<DBL_MANT_DIG)*precision && FLT_RADIX==2);
  *variable -= value;
  if (*variable < precision)
    *variable = 0.0;
}

static inline int double_positive(double value, double precision)
{
  return (value > precision);
}

static inline int double_equals(double value1, double value2, double precision)
{
  return (fabs(value1 - value2) < precision);
}

SG_BEGIN_DECL()

/** @{ @ingroup SURF_lmm */
/**
 * @brief Create a new Linear MaxMim system
 * @param selective_update [description]
 */
XBT_PUBLIC(lmm_system_t) lmm_system_new(int selective_update);

/**
 * @brief Free an existing Linear MaxMin system
 * @param sys The lmm system to free
 */
XBT_PUBLIC(void) lmm_system_free(lmm_system_t sys);

/**
 * @brief Create a new Linear MaxMin constraint
 * @param sys The system in which we add a constraint
 * @param id Data associated to the constraint (e.g.: a network link)
 * @param bound_value The bound value of the constraint 
 */
XBT_PUBLIC(lmm_constraint_t) lmm_constraint_new(lmm_system_t sys, void *id,double bound_value);

/**
 * @brief Share a constraint
 * @param cnst The constraint to share
 */
XBT_PUBLIC(void) lmm_constraint_shared(lmm_constraint_t cnst);

/**
 * @brief Check if a constraint is shared (shared by default)
 * @param cnst The constraint to share
 * @return 1 if shared, 0 otherwise
 */
XBT_PUBLIC(int) lmm_constraint_sharing_policy(lmm_constraint_t cnst);

/**
 * @brief Free a constraint
 * @param sys The system associated to the constraint
 * @param cnst The constraint to free
 */
XBT_PUBLIC(void) lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst);

/**
 * @brief Get the usage of the constraint after the last lmm solve
 * @param cnst A constraint
 * @return The usage of the constraint
 */
XBT_PUBLIC(double) lmm_constraint_get_usage(lmm_constraint_t cnst);

/**
 * @brief Sets the concurrency limit for this constraint
 * @param cnst A constraint
 * @param concurrency_limit The concurrency limit to use for this constraint
 */
XBT_PUBLIC(void) lmm_constraint_concurrency_limit_set(lmm_constraint_t cnst, int concurrency_limit);

/**
 * @brief Gets the concurrency limit for this constraint
 * @param cnst A constraint
 * @return The concurrency limit used by this constraint
 */
XBT_PUBLIC(int) lmm_constraint_concurrency_limit_get(lmm_constraint_t cnst);

/**
 * @brief Reset the concurrency maximum for a given variable (we will update the maximum to reflect constraint
 * evolution).
 * @param cnst A constraint
*/
XBT_PUBLIC(void) lmm_constraint_concurrency_maximum_reset(lmm_constraint_t cnst);

/**
 * @brief Get the concurrency maximum for a given variable (which reflects constraint evolution).
 * @param cnst A constraint
 * @return the maximum concurrency of the constraint
 */
XBT_PUBLIC(int) lmm_constraint_concurrency_maximum_get(lmm_constraint_t cnst);

/**
 * @brief Create a new Linear MaxMin variable
 * @param sys The system in which we add a constaint
 * @param id Data associated to the variable (e.g.: a network communication)
 * @param weight_value The weight of the variable (0.0 if not used)
 * @param bound The maximum value of the variable (-1.0 if no maximum value)
 * @param number_of_constraints The maximum number of constraint to associate to the variable
 */
XBT_PUBLIC(lmm_variable_t) lmm_variable_new(lmm_system_t sys, void *id, double weight_value, double bound,
                                            int number_of_constraints);
/**
 * @brief Free a variable
 * @param sys The system associated to the variable
 * @param var The variable to free
 */
XBT_PUBLIC(void) lmm_variable_free(lmm_system_t sys, lmm_variable_t var);

/**
 * @brief Get the value of the variable after the last lmm solve
 * @param var A variable
 * @return The value of the variable
 */
XBT_PUBLIC(double) lmm_variable_getvalue(lmm_variable_t var);

/**
 * @brief Get the maximum value of the variable (-1.0 if no maximum value)
 * @param var A variable
 * @return The bound of the variable
 */
XBT_PUBLIC(double) lmm_variable_getbound(lmm_variable_t var);

/**
 * @brief Set the concurrent share of the variable
 * @param var A variable
 * @param concurrency_share The new concurrency share
 */
XBT_PUBLIC(void) lmm_variable_concurrency_share_set(lmm_variable_t var, short int concurrency_share);

/**
 * @brief Remove a variable from a constraint
 * @param sys A system
 * @param cnst A constraint
 * @param var The variable to remove
 */
XBT_PUBLIC(void) lmm_shrink(lmm_system_t sys, lmm_constraint_t cnst, lmm_variable_t var);

/**
 * @brief Associate a variable to a constraint with a coefficient
 * @param sys A system
 * @param cnst A constraint
 * @param var A variable
 * @param value The coefficient associated to the variable in the constraint
 */
XBT_PUBLIC(void) lmm_expand(lmm_system_t sys, lmm_constraint_t cnst, lmm_variable_t var, double value);

/**
 * @brief Add value to the coefficient between a constraint and a variable or create one
 * @param sys A system
 * @param cnst A constraint
 * @param var A variable
 * @param value The value to add to the coefficient associated to the variable in the constraint
 */
XBT_PUBLIC(void) lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst, lmm_variable_t var, double value);

/**
 * @brief Get the numth constraint associated to the variable
 * @param sys The system associated to the variable (not used)
 * @param var A variable
 * @param num The rank of constraint we want to get
 * @return The numth constraint
 */
XBT_PUBLIC(lmm_constraint_t) lmm_get_cnst_from_var(lmm_system_t sys, lmm_variable_t var, int num);

/**
 * @brief Get the weigth of the numth constraint associated to the variable
 * @param sys The system associated to the variable (not used)
 * @param var A variable
 * @param num The rank of constraint we want to get
 * @return The numth constraint
 */
XBT_PUBLIC(double) lmm_get_cnst_weight_from_var(lmm_system_t sys, lmm_variable_t var, int num);

/**
 * @brief Get the number of constraint associated to a variable
 * @param sys The system associated to the variable (not used)
 * @param var A variable
 * @return The number of constraint associated to the variable
 */
XBT_PUBLIC(int) lmm_get_number_of_cnst_from_var(lmm_system_t sys, lmm_variable_t var);

/**
 * @brief Get a var associated to a constraint 
 * @details Get the first variable of the next variable of elem if elem is not NULL
 * @param sys The system associated to the variable (not used)
 * @param cnst A constraint
 * @param elem A element of constraint of the constraint or NULL
 * @return A variable associated to a constraint
 */
XBT_PUBLIC(lmm_variable_t) lmm_get_var_from_cnst(lmm_system_t sys, lmm_constraint_t cnst, lmm_element_t * elem);

/**
 * @brief Get a var associated to a constraint
 * @details Get the first variable of the next variable of elem if elem is not NULL
 * @param cnst A constraint
 * @param elem A element of constraint of the constraint or NULL
 * @param nextelem A element of constraint of the constraint or NULL, the one after elem
 * @param numelem parameter representing the number of elements to go
 *
 * @return A variable associated to a constraint
 */
XBT_PUBLIC(lmm_variable_t) lmm_get_var_from_cnst_safe(lmm_system_t /*sys*/, lmm_constraint_t cnst,
                                     lmm_element_t * elem, lmm_element_t * nextelem, int * numelem);

/**
 * @brief Get the first active constraint of a system
 * @param sys A system
 * @return The first active constraint
 */
XBT_PUBLIC(lmm_constraint_t) lmm_get_first_active_constraint(lmm_system_t sys);

/**
 * @brief Get the next active constraint of a constraint in a system
 * @param sys A system
 * @param cnst An active constraint of the system
 * 
 * @return The next active constraint
 */
XBT_PUBLIC(lmm_constraint_t) lmm_get_next_active_constraint(lmm_system_t sys, lmm_constraint_t cnst);

/**
 * @brief Get the data associated to a constraint
 * @param cnst A constraint
 * @return The data associated to the constraint
 */
XBT_PUBLIC(void *) lmm_constraint_id(lmm_constraint_t cnst);

/**
 * @brief Get the data associated to a variable
 * @param var A variable
 * @return The data associated to the variable
 */
XBT_PUBLIC(void *) lmm_variable_id(lmm_variable_t var);

/**
 * @brief Update the value of element linking the constraint and the variable
 * @param sys A system
 * @param cnst A constraint
 * @param var A variable
 * @param value The new value
 */
XBT_PUBLIC(void) lmm_update(lmm_system_t sys, lmm_constraint_t cnst, lmm_variable_t var, double value);

/**
 * @brief Update the bound of a variable
 * @param sys A system
 * @param var A constraint
 * @param bound The new bound
 */
XBT_PUBLIC(void) lmm_update_variable_bound(lmm_system_t sys, lmm_variable_t var, double bound);

/**
 * @brief Update the weight of a variable
 * @param sys A system
 * @param var A variable
 * @param weight The new weight of the variable
 */
XBT_PUBLIC(void) lmm_update_variable_weight(lmm_system_t sys, lmm_variable_t var, double weight);

/**
 * @brief Get the weight of a variable
 * @param var A variable
 * @return The weight of the variable
 */
XBT_PUBLIC(double) lmm_get_variable_weight(lmm_variable_t var);

/**
 * @brief Update a constraint bound
 * @param sys A system
 * @param cnst A constraint
 * @param bound The new bound of the consrtaint
 */
XBT_PUBLIC(void) lmm_update_constraint_bound(lmm_system_t sys, lmm_constraint_t cnst, double bound);

/**
 * @brief [brief description]
 * @param sys A system
 * @param cnst A constraint
 * @return [description]
 */
XBT_PUBLIC(int) lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst);

/**
 * @brief Print the lmm system
 * @param sys The lmm system to print
 */
XBT_PUBLIC(void) lmm_print(lmm_system_t sys);

/**
 * @brief Solve the lmm system
 * @param sys The lmm system to solve
 */
XBT_PUBLIC(void) lmm_solve(lmm_system_t sys);

XBT_PUBLIC(void) lagrange_solve(lmm_system_t sys);
XBT_PUBLIC(void) bottleneck_solve(lmm_system_t sys);

/** Default functions associated to the chosen protocol. When using the lagrangian approach. */

XBT_PUBLIC(void) lmm_set_default_protocol_function(double (*func_f)(lmm_variable_t var,double x),
                                                   double (*func_fp)(lmm_variable_t var,double x),
                                                   double (*func_fpi)(lmm_variable_t var,double x));

XBT_PUBLIC(double func_reno_f) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_reno_fp) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_reno_fpi) (lmm_variable_t var, double x);

XBT_PUBLIC(double func_reno2_f) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_reno2_fp) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_reno2_fpi) (lmm_variable_t var, double x);

XBT_PUBLIC(double func_vegas_f) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_vegas_fp) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_vegas_fpi) (lmm_variable_t var, double x);

/** @} */
SG_END_DECL()

#endif                          /* _SURF_MAXMIN_H */
