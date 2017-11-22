/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MAXMIN_PRIVATE_H
#define SURF_MAXMIN_PRIVATE_H

#include "surf/maxmin.hpp"
#include "surf_interface.hpp"
#include "xbt/mallocator.h"
#include "xbt/swag.h"

#include <vector>

/** @ingroup SURF_lmm
 * @brief LMM element
 * Elements can be seen as glue between constraint objects and variable objects.
 * Basically, each variable will have a set of elements, one for each constraint where it is involved.
 * Then, it is used to list all variables involved in constraint through constraint's xxx_element_set lists, or vice-versa list all constraints for a given variable.
 */
struct s_lmm_element_t {
  /* hookup to constraint */
  s_xbt_swag_hookup_t enabled_element_set_hookup;
  s_xbt_swag_hookup_t disabled_element_set_hookup;
  s_xbt_swag_hookup_t active_element_set_hookup;

  lmm_constraint_t constraint;
  lmm_variable_t variable;

  // consumption_weight: impact of 1 byte or flop of your application onto the resource (in byte or flop)
  //   - if CPU, then probably 1.
  //   - If network, then 1 in forward direction and 0.05 backward for the ACKs
  double consumption_weight;
};
#define make_elem_active(elem) xbt_swag_insert_at_head((elem), &((elem)->constraint->active_element_set))
#define make_elem_inactive(elem) xbt_swag_remove((elem), &((elem)->constraint->active_element_set))

struct s_lmm_constraint_light_t {
  double remaining_over_usage;
  lmm_constraint_t cnst;
};

/** @ingroup SURF_lmm
 * @brief LMM constraint
 * Each constraint contains several partially overlapping logical sets of elements:
 * \li Disabled elements which variable's weight is zero. This variables are not at all processed by LMM, but eventually the corresponding action will enable it (at least this is the idea).
 * \li Enabled elements which variable's weight is non-zero. They are utilized in some LMM functions.
 * \li Active elements which variable's weight is non-zero (i.e. it is enabled) AND its element value is non-zero. LMM_solve iterates over active elements during resolution, dynamically making them active or unactive.
 *
 */
struct s_lmm_constraint_t {
  /* hookup to system */
  s_xbt_swag_hookup_t constraint_set_hookup;
  s_xbt_swag_hookup_t active_constraint_set_hookup;
  s_xbt_swag_hookup_t modified_constraint_set_hookup;
  s_xbt_swag_hookup_t saturated_constraint_set_hookup;

  s_xbt_swag_t enabled_element_set;     /* a list of lmm_element_t */
  s_xbt_swag_t disabled_element_set;     /* a list of lmm_element_t */
  s_xbt_swag_t active_element_set;      /* a list of lmm_element_t */
  double remaining;
  double usage;
  double bound;
  int concurrency_limit; /* The maximum number of variables that may be enabled at any time (stage variables if necessary) */
  //TODO MARTIN Check maximum value across resources at the end of simulation and give a warning is more than e.g. 500
  int concurrency_current; /* The current concurrency */
  int concurrency_maximum; /* The maximum number of (enabled and disabled) variables associated to the constraint at any given time (essentially for tracing)*/

  int sharing_policy; /* see @e_surf_link_sharing_policy_t (0: FATPIPE, 1: SHARED, 2: FULLDUPLEX) */
  void *id;
  int id_int;
  double lambda;
  double new_lambda;
  lmm_constraint_light_t cnst_light;
};

/** @ingroup SURF_lmm
 * @brief LMM variable
 *
 * When something prevents us from enabling a variable, we "stage" the weight that we would have like to set, so that as soon as possible we enable the variable with desired weight
 */
struct s_lmm_variable_t {
  /* hookup to system */
  s_xbt_swag_hookup_t variable_set_hookup;
  s_xbt_swag_hookup_t saturated_variable_set_hookup;

  std::vector<s_lmm_element_t> cnsts;

  // sharing_weight: variable's impact on the resource during the sharing
  //   if == 0, the variable is not considered by LMM
  //   on CPU, actions with N threads have a sharing of N
  //   on network, the actions with higher latency have a lesser sharing_weight
  double sharing_weight;

  double staged_weight; /* If non-zero, variable is staged for addition as soon as maxconcurrency constraints will be met */
  double bound;
  double value;
  short int concurrency_share; /* The maximum number of elements that variable will add to a constraint */
  simgrid::surf::Action* id;
  int id_int;
  unsigned visited;             /* used by lmm_update_modified_set */
  /* \begin{For Lagrange only} */
  double mu;
  double new_mu;
  double (*func_f)(s_lmm_variable_t* var, double x);   /* (f)    */
  double (*func_fp)(s_lmm_variable_t* var, double x);  /* (f')    */
  double (*func_fpi)(s_lmm_variable_t* var, double x); /* (f')^{-1}    */
  /* \end{For Lagrange only} */
};

/** @ingroup SURF_lmm
 * @brief LMM system
 */
class s_lmm_system_t {
public:
  /**
   * @brief Create a new Linear MaxMim system
   * @param selective_update whether we should do lazy updates
   */
  s_lmm_system_t(bool selective_update);
  /** @brief Free an existing Linear MaxMin system */
  ~s_lmm_system_t();

  /**
   * @brief Create a new Linear MaxMin constraint
   * @param id Data associated to the constraint (e.g.: a network link)
   * @param bound_value The bound value of the constraint
   */
  lmm_constraint_t constraint_new(void* id, double bound_value);

  /**
   * @brief Create a new Linear MaxMin variable
   * @param id Data associated to the variable (e.g.: a network communication)
   * @param weight_value The weight of the variable (0.0 if not used)
   * @param bound The maximum value of the variable (-1.0 if no maximum value)
   * @param number_of_constraints The maximum number of constraint to associate to the variable
   */
  lmm_variable_t variable_new(simgrid::surf::Action* id, double weight_value, double bound, int number_of_constraints);

  /**
   * @brief Free a variable
   * @param var The variable to free
   */
  void variable_free(lmm_variable_t var);

  /**
   * @brief Associate a variable to a constraint with a coefficient
   * @param cnst A constraint
   * @param var A variable
   * @param value The coefficient associated to the variable in the constraint
   */
  void expand(lmm_constraint_t cnst, lmm_variable_t var, double value);

  /**
   * @brief Add value to the coefficient between a constraint and a variable or create one
   * @param cnst A constraint
   * @param var A variable
   * @param value The value to add to the coefficient associated to the variable in the constraint
   */
  void expand_add(lmm_constraint_t cnst, lmm_variable_t var, double value);

  /**
   * @brief Update the bound of a variable
   * @param var A constraint
   * @param bound The new bound
   */
  void update_variable_bound(lmm_variable_t var, double bound);

  /**
   * @brief Update the weight of a variable
   * @param var A variable
   * @param weight The new weight of the variable
   */
  void update_variable_weight(lmm_variable_t var, double weight);

  /**
   * @brief Update a constraint bound
   * @param cnst A constraint
   * @param bound The new bound of the consrtaint
   */
  void update_constraint_bound(lmm_constraint_t cnst, double bound);

  /**
   * @brief [brief description]
   * @param cnst A constraint
   * @return [description]
   */
  int constraint_used(lmm_constraint_t cnst) { return xbt_swag_belongs(cnst, &active_constraint_set); }

  /** @brief Print the lmm system */
  void print();

  /** @brief Solve the lmm system */
  void solve();

private:
  static void* variable_mallocator_new_f();
  static void variable_mallocator_free_f(void* var);

  void var_free(lmm_variable_t var);
  void cnst_free(lmm_constraint_t cnst);
  lmm_variable_t extract_variable() { return static_cast<lmm_variable_t>(xbt_swag_extract(&variable_set)); }
  lmm_constraint_t extract_constraint() { return static_cast<lmm_constraint_t>(xbt_swag_extract(&constraint_set)); }
  void insert_constraint(lmm_constraint_t cnst) { xbt_swag_insert(cnst, &constraint_set); }
  void remove_variable(lmm_variable_t var)
  {
    xbt_swag_remove(var, &variable_set);
    xbt_swag_remove(var, &saturated_variable_set);
  }
  void remove_constraint(lmm_constraint_t cnst) // FIXME: unused
  {
    xbt_swag_remove(cnst, &constraint_set);
    xbt_swag_remove(cnst, &saturated_constraint_set);
  }
  void make_constraint_active(lmm_constraint_t cnst) { xbt_swag_insert(cnst, &active_constraint_set); }
  void make_constraint_inactive(lmm_constraint_t cnst)
  {
    xbt_swag_remove(cnst, &active_constraint_set);
    xbt_swag_remove(cnst, &modified_constraint_set);
  }

  void enable_var(lmm_variable_t var);
  void disable_var(lmm_variable_t var);
  void on_disabled_var(lmm_constraint_t cnstr);

  /**
   * @brief Update the value of element linking the constraint and the variable
   * @param cnst A constraint
   * @param var A variable
   * @param value The new value
   */
  void update(lmm_constraint_t cnst, lmm_variable_t var, double value);

  void update_modified_set(lmm_constraint_t cnst);
  void update_modified_set_rec(lmm_constraint_t cnst);

  /** @brief Remove all constraints of the modified_constraint_set. */
  void remove_all_modified_set();
  void check_concurrency();

public:
  int modified;
  s_xbt_swag_t variable_set;    /* a list of lmm_variable_t */
  s_xbt_swag_t active_constraint_set;   /* a list of lmm_constraint_t */
  s_xbt_swag_t saturated_variable_set;  /* a list of lmm_variable_t */
  s_xbt_swag_t saturated_constraint_set;        /* a list of lmm_constraint_t_t */

  simgrid::surf::ActionLmmListPtr keep_track;

  void (*solve_fun)(lmm_system_t self);

private:
  bool selective_update_active; /* flag to update partially the system only selecting changed portions */
  unsigned visited_counter;     /* used by lmm_update_modified_set and lmm_remove_modified_set to cleverly (un-)flag the
                                 * constraints (more details in these functions) */
  s_xbt_swag_t constraint_set;  /* a list of lmm_constraint_t */
  s_xbt_swag_t modified_constraint_set; /* a list of modified lmm_constraint_t */
  xbt_mallocator_t variable_mallocator;
};

extern XBT_PRIVATE double (*func_f_def) (lmm_variable_t, double);
extern XBT_PRIVATE double (*func_fp_def) (lmm_variable_t, double);
extern XBT_PRIVATE double (*func_fpi_def) (lmm_variable_t, double);

#endif /* SURF_MAXMIN_PRIVATE_H */
