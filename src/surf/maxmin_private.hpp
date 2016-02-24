/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_MAXMIN_PRIVATE_H
#define _SURF_MAXMIN_PRIVATE_H

#include <xbt/base.h>

#include "surf/maxmin.h"
#include "xbt/swag.h"
#include "xbt/mallocator.h"
#include "surf_interface.hpp"

/** @ingroup SURF_lmm
 * @brief LMM element
 * Elements can be seen as glue between constraint objects and variable objects.
 * Basically, each variable will have a set of elements, one for each constraint where it is involved.
 * Then, it is used to list all variables involved in constraint through constraint's xxx_element_set lists, or vice-versa list all constraints for a given variable.  
 */
typedef struct lmm_element {
  /* hookup to constraint */
  s_xbt_swag_hookup_t enabled_element_set_hookup;
  s_xbt_swag_hookup_t disabled_element_set_hookup;
  s_xbt_swag_hookup_t active_element_set_hookup;

  lmm_constraint_t constraint;
  lmm_variable_t variable;
  double value;
} s_lmm_element_t;
#define make_elem_active(elem) xbt_swag_insert_at_head(elem,&(elem->constraint->active_element_set))
#define make_elem_inactive(elem) xbt_swag_remove(elem,&(elem->constraint->active_element_set))

typedef struct lmm_constraint_light {
  double remaining_over_usage;
  lmm_constraint_t cnst;
} s_lmm_constraint_light_t;

/** @ingroup SURF_lmm
 * @brief LMM constraint
 * Each constraint contains several partially overlapping logical sets of elements: 
 * \li Disabled elements which variable's weight is zero. This variables are not at all processed by LMM, but eventually the corresponding action will enable it (at least this is the idea).
 * \li Enabled elements which variable's weight is non-zero. They are utilized in some LMM functions.
 * \li Active elements which variable's weight is non-zero (i.e. it is enabled) AND its element value is non-zero. LMM_solve iterates over active elements during resolution, dynamically making them active or unactive. 
 * 
 */
typedef struct lmm_constraint {
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
} s_lmm_constraint_t;

/** @ingroup SURF_lmm
 * @brief LMM variable
 * 
 * When something prevents us from enabling a variable, we "stage" the weight that we would have like to set, so that as soon as possible we enable the variable with desired weight
 */
typedef struct lmm_variable {
  /* hookup to system */
  s_xbt_swag_hookup_t variable_set_hookup;
  s_xbt_swag_hookup_t saturated_variable_set_hookup;

  s_lmm_element_t *cnsts;
  int cnsts_size;
  int cnsts_number;
  double weight; /* weight > 0 means variable is considered by LMM, weight == 0 means variable is not considered by LMM*/
  double staged_weight; /* If non-zero, variable is staged for addition as soon as maxconcurrency constraints will be met */
  double bound;
  double value;
  short int concurrency_share; /* The maximum number of elements that variable will add to a constraint */
  void *id;
  int id_int;
  unsigned visited;             /* used by lmm_update_modified_set */
  /* \begin{For Lagrange only} */
  double mu;
  double new_mu;
  double (*func_f) (struct lmm_variable * var, double x);       /* (f)    */
  double (*func_fp) (struct lmm_variable * var, double x);      /* (f')    */
  double (*func_fpi) (struct lmm_variable * var, double x);     /* (f')^{-1}    */
  /* \end{For Lagrange only} */
} s_lmm_variable_t;

/** @ingroup SURF_lmm
 * @brief LMM system
 */
typedef struct lmm_system {
  int modified;
  int selective_update_active;  /* flag to update partially the system only selecting changed portions */
  unsigned visited_counter;     /* used by lmm_update_modified_set  and lmm_remove_modified_set to cleverly (un-)flag the constraints (more details in these functions)*/
  s_xbt_swag_t variable_set;    /* a list of lmm_variable_t */
  s_xbt_swag_t constraint_set;  /* a list of lmm_constraint_t */

  s_xbt_swag_t active_constraint_set;   /* a list of lmm_constraint_t */
  s_xbt_swag_t modified_constraint_set; /* a list of modified lmm_constraint_t */

  s_xbt_swag_t saturated_variable_set;  /* a list of lmm_variable_t */
  s_xbt_swag_t saturated_constraint_set;        /* a list of lmm_constraint_t_t */

  simgrid::surf::ActionLmmListPtr keep_track;

  xbt_mallocator_t variable_mallocator;
} s_lmm_system_t;

#define extract_variable(sys) xbt_swag_extract(&(sys->variable_set))
#define extract_constraint(sys) xbt_swag_extract(&(sys->constraint_set))
#define insert_constraint(sys,cnst) xbt_swag_insert(cnst,&(sys->constraint_set))
#define remove_variable(sys,var) do {xbt_swag_remove(var,&(sys->variable_set));\
                                 xbt_swag_remove(var,&(sys->saturated_variable_set));} while(0)
#define remove_constraint(sys,cnst) do {xbt_swag_remove(cnst,&(sys->constraint_set));\
                                        xbt_swag_remove(cnst,&(sys->saturated_constraint_set));} while(0)
#define make_constraint_active(sys,cnst) xbt_swag_insert(cnst,&(sys->active_constraint_set))
#define make_constraint_inactive(sys,cnst) \
  do { xbt_swag_remove(cnst, &sys->active_constraint_set);              \
    xbt_swag_remove(cnst, &sys->modified_constraint_set); } while (0)

/** @ingroup SURF_lmm
 * @brief Print informations about a lmm system
 * 
 * @param sys A lmm system
 */
//XBT_PRIVATE void lmm_print(lmm_system_t sys);

extern XBT_PRIVATE double (*func_f_def) (lmm_variable_t, double);
extern XBT_PRIVATE double (*func_fp_def) (lmm_variable_t, double);
extern XBT_PRIVATE double (*func_fpi_def) (lmm_variable_t, double);

#endif                          /* _SURF_MAXMIN_PRIVATE_H */


