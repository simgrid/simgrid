/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_MAXMIN_H
#define _SURF_MAXMIN_H

#include "portable.h"
#include "xbt/misc.h"
#include "xbt/asserts.h"
#include "surf/datatypes.h"
#include <math.h>


/** @addtogroup SURF_lmm 
 * @details 
 * A linear maxmin solver to resolves inequations systems.
 * 
 * Most SimGrid model rely on a "fluid/steady-state" modeling that
 * samount to share resources between actions at relatively
 * coarse-grain.  Such sharing is generally done by solving a set of
 * linear inequations. Let's take an example and assume we have the
 * variables \f$x_1\f$, \f$x_2\f$, \f$x_3\f$, and \f$x_4\f$ . Let's
 * say that \f$x_1\f$ and \f$x_2\f$ correspond to activities running
 * and the same CPU \f$A\f$ whose capacity is \f$C_A\f$ . In such a
 * case, we need to enforce:
 *
 *   \f[ x_1 + x_2 \leq C_A \f]
 *
 * Likewise, if \f$x_3\f$ (resp. \f$x_4\f$) corresponds to a network
 * flow \f$F_3\f$ (resp. \f$F_4\f$) that goes through a set of links
 * \f$L_1\f$ and \f$L_2\f$ (resp. \f$L_2\f$ and \f$L_3\f$), then we
 * need to enforce:
 *
 *   \f[ x_3  \leq C_{L_1} \f]
 *   \f[ x_3 + x_4 \leq C_{L_2} \f]
 *   \f[ x_4 \leq C_{L_3} \f]
 * 
 * One could set every variable to 0 to make sure the constraints are
 * satisfied but this would obviously not be very realistic. A
 * possible objective is to try to maximize the minimum of the
 * \f$x_i\f$ . This ensures that all the \f$x_i\f$ are positive and "as
 * large as possible". 
 *
 * This is called *max-min fairness* and is the most commonly used
 * objective in SimGrid. Another possibility is to maximize
 * \f$\sum_if(x_i)\f$, where \f$f\f$ is a strictly increasing concave
 * function.
 *
 * Constraint: 
 *  - bound (set)
 *  - shared (set)
 *  - usage (computed)
 * Variable:
 *  - weight (set)
 *  - bound (set)
 *  - value (computed)
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
 * where `var1.value`, `var2.value` and `var3.value` are the unknown values
 * 
 * if a constraint is not shared the sum is replace by a max
 * 
 * Its usefull for the sharing of resources for various models.
 * For instance for the network model the link are associated 
 * to consrtaint and the communications to variables.
 */

XBT_PUBLIC_DATA(double) sg_maxmin_precision;
XBT_PUBLIC_DATA(double) sg_surf_precision;
 
static XBT_INLINE void double_update(double *variable, double value, double precision)
{
  //printf("Updating %g -= %g +- %g\n",*variable,value,precision);
  //xbt_assert(value==0  || value>precision);
  //Check that precision is higher than the machine-dependent size of the mantissa. If not, brutal rounding  may happen, and the precision mechanism is not active... 
  //xbt_assert(*variable< (2<<DBL_MANT_DIG)*precision && FLT_RADIX==2);
  *variable -= value;
  if (*variable < precision)
    *variable = 0.0;
}

static XBT_INLINE int double_positive(double value, double precision)
{
  return (value > precision);
}

static XBT_INLINE int double_equals(double value1, double value2, double precision)
{
  return (fabs(value1 - value2) < precision);
}

SG_BEGIN_DECL()

/** @{ @ingroup SURF_lmm */
/**
 * @brief Create a new Linear MaxMim system
 * 
 * @param selective_update [description]
 */
XBT_PUBLIC(lmm_system_t) lmm_system_new(int selective_update);

/**
 * @brief Free an existing Linear MaxMin system
 * 
 * @param sys The lmm system to free
 */
XBT_PUBLIC(void) lmm_system_free(lmm_system_t sys);

/**
 * @brief Create a new Linear MaxMin constraint
 * 
 * @param sys The system in which we add a constraint
 * @param id Data associated to the constraint (e.g.: a network link)
 * @param bound_value The bound value of the constraint 
 */
XBT_PUBLIC(lmm_constraint_t) lmm_constraint_new(lmm_system_t sys, void *id,
                                                double bound_value);

/**
 * @brief Share a constraint
 * @details [long description]
 * 
 * @param cnst The constraint to share
 */
XBT_PUBLIC(void) lmm_constraint_shared(lmm_constraint_t cnst);

/**
 * @brief Check if a constraint is shared (shared by default)
 * 
 * @param cnst The constraint to share
 * @return 1 if shared, 0 otherwise
 */
XBT_PUBLIC(int) lmm_constraint_is_shared(lmm_constraint_t cnst);

/**
 * @brief Free a constraint
 * 
 * @param sys The system associated to the constraint
 * @param cnst The constraint to free
 */
XBT_PUBLIC(void) lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst);

/**
 * @brief Get the usage of the constraint after the last lmm solve
 * 
 * @param cnst A constraint
 * @return The usage of the constraint
 */
XBT_PUBLIC(double) lmm_constraint_get_usage(lmm_constraint_t cnst);

/**
 * @brief Create a new Linear MaxMin variable
 * 
 * @param sys The system in which we add a constaint
 * @param id Data associated to the variable (e.g.: a network communication)
 * @param weight_value The weight of the variable (0.0 if not used)
 * @param bound The maximum value of the variable (-1.0 if no maximum value)
 * @param number_of_constraints The maximum number of constraint to associate to the variable
 */
XBT_PUBLIC(lmm_variable_t) lmm_variable_new(lmm_system_t sys, void *id,
                                            double weight_value,
                                            double bound,
                                            int number_of_constraints);
/**
 * @brief Free a variable
 * 
 * @param sys The system associated to the variable
 * @param var The variable to free
 */
XBT_PUBLIC(void) lmm_variable_free(lmm_system_t sys, lmm_variable_t var);

/**
 * @brief Get the value of the variable after the last lmm solve
 * 
 * @param var A variable
 * @return The value of the variable
 */
XBT_PUBLIC(double) lmm_variable_getvalue(lmm_variable_t var);

/**
 * @brief Get the maximum value of the variable (-1.0 if no maximum value)
 * 
 * @param var A variable
 * @return The bound of the variable
 */
XBT_PUBLIC(double) lmm_variable_getbound(lmm_variable_t var);

/**
 * @brief Remove a variable from a constraint
 * 
 * @param sys A system
 * @param cnst A constraint
 * @param var The variable to remove
 */
XBT_PUBLIC(void) lmm_shrink(lmm_system_t sys, lmm_constraint_t cnst,
                            lmm_variable_t var);

/**
 * @brief Associate a variable to a constraint with a coefficient
 * 
 * @param sys A system
 * @param cnst A constraint
 * @param var A variable
 * @param value The coefficient associated to the variable in the constraint
 */
XBT_PUBLIC(void) lmm_expand(lmm_system_t sys, lmm_constraint_t cnst,
                            lmm_variable_t var, double value);

/**
 * @brief Add value to the coefficient between a constraint and a variable or 
 *        create one
 * 
 * @param sys A system
 * @param cnst A constraint
 * @param var A variable
 * @param value The value to add to the coefficient associated to the variable in the constraint
 */
XBT_PUBLIC(void) lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst,
                    lmm_variable_t var, double value);

/**
 * @brief Get the numth constraint associated to the variable
 * 
 * @param sys The system associated to the variable (not used)
 * @param var A variable
 * @param num The rank of constraint we want to get
 * @return The numth constraint
 */
XBT_PUBLIC(lmm_constraint_t) lmm_get_cnst_from_var(lmm_system_t sys,
                                       lmm_variable_t var, int num);

/**
 * @brief Get the weigth of the numth constraint associated to the variable
 * 
 * @param sys The system associated to the variable (not used)
 * @param var A variable
 * @param num The rank of constraint we want to get
 * @return The numth constraint
 */
XBT_PUBLIC(double) lmm_get_cnst_weight_from_var(lmm_system_t sys, lmm_variable_t var,
                                    int num);

/**
 * @brief Get the number of constraint associated to a variable
 * 
 * @param sys The system associated to the variable (not used)
 * @param var A variable
 * @return The number of constraint associated to the variable
 */
XBT_PUBLIC(int) lmm_get_number_of_cnst_from_var(lmm_system_t sys, lmm_variable_t var);

/**
 * @brief Get a var associated to a constraint 
 * @details Get the first variable of the next variable of elem if elem is not NULL
 * 
 * @param sys The system associated to the variable (not used)
 * @param cnst A constraint
 * @param elem A element of constraint of the constraint or NULL
 * @return A variable associated to a constraint
 */
XBT_PUBLIC(lmm_variable_t) lmm_get_var_from_cnst(lmm_system_t sys,
                                     lmm_constraint_t cnst,
                                     lmm_element_t * elem);

/**
 * @brief Get a var associated to a constraint
 * @details Get the first variable of the next variable of elem if elem is not NULL
 *
 * @param cnst A constraint
 * @param elem A element of constraint of the constraint or NULL
 * @param nextelem A element of constraint of the constraint or NULL, the one after elem
 * @param numelem parameter representing the number of elements to go
 *
 * @return A variable associated to a constraint
 */
XBT_PUBLIC(lmm_variable_t) lmm_get_var_from_cnst_safe(lmm_system_t /*sys*/,
                                     lmm_constraint_t cnst,
                                     lmm_element_t * elem,
                                     lmm_element_t * nextelem,
                                     int * numelem);

/**
 * @brief Get the first active constraint of a system
 * 
 * @param sys A system
 * @return The first active constraint
 */
XBT_PUBLIC(lmm_constraint_t) lmm_get_first_active_constraint(lmm_system_t sys);

/**
 * @brief Get the next active constraint of a constraint in a system
 * 
 * @param sys A system
 * @param cnst An active constraint of the system
 * 
 * @return The next active constraint
 */
XBT_PUBLIC(lmm_constraint_t) lmm_get_next_active_constraint(lmm_system_t sys,
                                                lmm_constraint_t cnst);

#ifdef HAVE_LATENCY_BOUND_TRACKING
XBT_PUBLIC(int) lmm_is_variable_limited_by_latency(lmm_variable_t var);
#endif

/**
 * @brief Get the data associated to a constraint
 * 
 * @param cnst A constraint
 * @return The data associated to the constraint
 */
XBT_PUBLIC(void *) lmm_constraint_id(lmm_constraint_t cnst);

/**
 * @brief Get the data associated to a variable
 * 
 * @param var A variable
 * @return The data associated to the variable
 */
XBT_PUBLIC(void *) lmm_variable_id(lmm_variable_t var);

/**
 * @brief Update the value of element linking the constraint and the variable
 * 
 * @param sys A system
 * @param cnst A constraint
 * @param var A variable
 * @param value The new value
 */
XBT_PUBLIC(void) lmm_update(lmm_system_t sys, lmm_constraint_t cnst,
                lmm_variable_t var, double value);

/**
 * @brief Update the bound of a variable
 * 
 * @param sys A system
 * @param var A constraint
 * @param bound The new bound
 */
XBT_PUBLIC(void) lmm_update_variable_bound(lmm_system_t sys, lmm_variable_t var,
                               double bound);

/**
 * @brief Update the weight of a variable
 * 
 * @param sys A system
 * @param var A variable
 * @param weight The new weight of the variable
 */
XBT_PUBLIC(void) lmm_update_variable_weight(lmm_system_t sys,
                                            lmm_variable_t var,
                                            double weight);

/**
 * @brief Get the weight of a variable
 * 
 * @param var A variable
 * @return The weight of the variable
 */
XBT_PUBLIC(double) lmm_get_variable_weight(lmm_variable_t var);

/**
 * @brief Update a constraint bound
 * 
 * @param sys A system
 * @param cnst A constraint
 * @param bound The new bound of the consrtaint
 */
XBT_PUBLIC(void) lmm_update_constraint_bound(lmm_system_t sys,
                                             lmm_constraint_t cnst,
                                             double bound);

/**
 * @brief [brief description]
 * 
 * @param sys A system
 * @param cnst A constraint
 * @return [description]
 */
XBT_PUBLIC(int) lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst);

/**
 * @brief Solve the lmm system
 * 
 * @param sys The lmm system to solve
 */
XBT_PUBLIC(void) lmm_solve(lmm_system_t sys);

XBT_PUBLIC(void) lagrange_solve(lmm_system_t sys);
XBT_PUBLIC(void) bottleneck_solve(lmm_system_t sys);

/**
 * Default functions associated to the chosen protocol. When
 * using the lagrangian approach.
 */

XBT_PUBLIC(void) lmm_set_default_protocol_function(double (*func_f)
                                                    (lmm_variable_t var,
                                                     double x),
                                                   double (*func_fp)
                                                    (lmm_variable_t var,
                                                     double x),
                                                   double (*func_fpi)
                                                    (lmm_variable_t var,
                                                     double x));

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
