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
 * Then, it is used to list all variables involved in constraint through constraint's xxx_element_set lists, or
 * vice-versa list all constraints for a given variable.
 */
class s_lmm_element_t {
public:
  int get_concurrency() const;
  void decrease_concurrency();
  void increase_concurrency();

  void make_active();
  void make_inactive();

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
class s_lmm_constraint_t {
public:
  s_lmm_constraint_t() = default;
  s_lmm_constraint_t(void* id_value, double bound_value);

  /** @brief Share a constraint. FIXME: name is misleading */
  void shared() { sharing_policy = 0; }

  /**
   * @brief Check if a constraint is shared (shared by default)
   * @return 1 if shared, 0 otherwise
   */
  int get_sharing_policy() const { return sharing_policy; }

  /**
   * @brief Get the usage of the constraint after the last lmm solve
   * @return The usage of the constraint
   */
  double get_usage() const;
  int get_variable_amount() const;

  /**
   * @brief Sets the concurrency limit for this constraint
   * @param concurrency_limit The concurrency limit to use for this constraint
   */
  void set_concurrency_limit(int limit)
  {
    xbt_assert(limit < 0 || concurrency_maximum <= limit,
               "New concurrency limit should be larger than observed concurrency maximum. Maybe you want to call"
               " concurrency_maximum_reset() to reset the maximum?");
    concurrency_limit = limit;
  }

  /**
   * @brief Gets the concurrency limit for this constraint
   * @return The concurrency limit used by this constraint
   */
  int get_concurrency_limit() const { return concurrency_limit; }

  /**
   * @brief Reset the concurrency maximum for a given variable (we will update the maximum to reflect constraint
   * evolution).
   */
  void reset_concurrency_maximum() { concurrency_maximum = 0; }

  /**
   * @brief Get the concurrency maximum for a given variable (which reflects constraint evolution).
   * @return the maximum concurrency of the constraint
   */
  int get_concurrency_maximum() const
  {
    xbt_assert(concurrency_limit < 0 || concurrency_maximum <= concurrency_limit,
               "Very bad: maximum observed concurrency is higher than limit. This is a bug of SURF, please report it.");
    return concurrency_maximum;
  }

  int get_concurrency_slack() const
  {
    return concurrency_limit < 0 ? std::numeric_limits<int>::max() : concurrency_limit - concurrency_current;
  }

  /**
   * @brief Get a var associated to a constraint
   * @details Get the first variable of the next variable of elem if elem is not NULL
   * @param elem A element of constraint of the constraint or NULL
   * @return A variable associated to a constraint
   */
  lmm_variable_t get_variable(lmm_element_t* elem) const;

  /**
   * @brief Get a var associated to a constraint
   * @details Get the first variable of the next variable of elem if elem is not NULL
   * @param elem A element of constraint of the constraint or NULL
   * @param nextelem A element of constraint of the constraint or NULL, the one after elem
   * @param numelem parameter representing the number of elements to go
   * @return A variable associated to a constraint
   */
  lmm_variable_t get_variable_safe(lmm_element_t* elem, lmm_element_t* nextelem, int* numelem) const;

  /**
   * @brief Get the data associated to a constraint
   * @return The data associated to the constraint
   */
  void* get_id() const { return id; }

  /* hookup to system */
  s_xbt_swag_hookup_t constraint_set_hookup           = {nullptr, nullptr};
  s_xbt_swag_hookup_t active_constraint_set_hookup    = {nullptr, nullptr};
  s_xbt_swag_hookup_t modified_constraint_set_hookup  = {nullptr, nullptr};
  s_xbt_swag_hookup_t saturated_constraint_set_hookup = {nullptr, nullptr};
  s_xbt_swag_t enabled_element_set;     /* a list of lmm_element_t */
  s_xbt_swag_t disabled_element_set;     /* a list of lmm_element_t */
  s_xbt_swag_t active_element_set;      /* a list of lmm_element_t */
  double remaining;
  double usage;
  double bound;
  //TODO MARTIN Check maximum value across resources at the end of simulation and give a warning is more than e.g. 500
  int concurrency_current; /* The current concurrency */
  int concurrency_maximum; /* The maximum number of (enabled and disabled) variables associated to the constraint at any given time (essentially for tracing)*/

  int sharing_policy; /* see @e_surf_link_sharing_policy_t (0: FATPIPE, 1: SHARED, 2: FULLDUPLEX) */
  int id_int;
  double lambda;
  double new_lambda;
  lmm_constraint_light_t cnst_light;

private:
  static int Global_debug_id;
  int concurrency_limit; /* The maximum number of variables that may be enabled at any time (stage variables if
                          * necessary) */
  void* id;
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

inline void s_lmm_element_t::make_active()
{
  xbt_swag_insert_at_head(this, &constraint->active_element_set);
}
inline void s_lmm_element_t::make_inactive()
{
  xbt_swag_remove(this, &constraint->active_element_set);
}

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
