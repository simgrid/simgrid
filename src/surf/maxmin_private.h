/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_MAXMIN_PRIVATE_H
#define _SURF_MAXMIN_PRIVATE_H

#include "surf/maxmin.h"
#include "xbt/swag.h"
#include "xbt/mallocator.h"

typedef struct lmm_element {
  /* hookup to constraint */
  s_xbt_swag_hookup_t element_set_hookup;
  s_xbt_swag_hookup_t active_element_set_hookup;

  lmm_constraint_t constraint;
  lmm_variable_t variable;
  double value;
} s_lmm_element_t;
#define make_elem_active(elem) xbt_swag_insert_at_head(elem,&(elem->constraint->active_element_set))
#define make_elem_inactive(elem) xbt_swag_remove(elem,&(elem->constraint->active_element_set))

typedef struct lmm_constraint {
  /* hookup to system */
  s_xbt_swag_hookup_t constraint_set_hookup;
  s_xbt_swag_hookup_t active_constraint_set_hookup;
  s_xbt_swag_hookup_t modified_constraint_set_hookup;
  s_xbt_swag_hookup_t saturated_constraint_set_hookup;

  s_xbt_swag_t element_set;     /* a list of lmm_mat_element_t */
  s_xbt_swag_t active_element_set;      /* a list of lmm_mat_element_t */
  double bound;
  double lambda;
  double new_lambda;
  double remaining;
  int shared;
  double usage;
  void *id;
  int id_int;
} s_lmm_constraint_t;

typedef struct lmm_variable {
  /* hookup to system */
  s_xbt_swag_hookup_t variable_set_hookup;
  s_xbt_swag_hookup_t saturated_variable_set_hookup;
  s_xbt_swag_hookup_t modified_variable_set_hookup;

  s_lmm_element_t *cnsts;
  int cnsts_size;
  int cnsts_number;
  double weight;
  double bound;
  double value;
  void *id;
  int id_int;
  /* \begin{For Lagrange only} */
  double mu;
  double new_mu;
  double (*func_f) (struct lmm_variable * var, double x);       /* (f)    */
  double (*func_fp) (struct lmm_variable * var, double x);      /* (f')    */
  double (*func_fpi) (struct lmm_variable * var, double x);     /* (f')^{-1}    */
  /* \end{For Lagrange only} */
} s_lmm_variable_t;

typedef struct lmm_system {
  int modified;
  int selective_update_active;  /* flag to update partially the system only selecting changed portions */

  s_xbt_swag_t variable_set;    /* a list of lmm_variable_t */
  s_xbt_swag_t constraint_set;  /* a list of lmm_constraint_t */

  s_xbt_swag_t active_constraint_set;   /* a list of lmm_constraint_t */
  s_xbt_swag_t modified_constraint_set; /* a list of modified lmm_constraint_t */

  s_xbt_swag_t saturated_variable_set;  /* a list of lmm_variable_t */
  s_xbt_swag_t saturated_constraint_set;        /* a list of lmm_constraint_t_t */

  s_xbt_swag_t modified_variable_set;   /* list of modified variables used in new model CpuIM */

  xbt_mallocator_t variable_mallocator;
} s_lmm_system_t;

#define extract_variable(sys) xbt_swag_remove(xbt_swag_getFirst(&(sys->variable_set)),&(sys->variable_set))
#define extract_constraint(sys) xbt_swag_remove(xbt_swag_getFirst(&(sys->constraint_set)),&(sys->constraint_set))
#define insert_constraint(sys,cnst) xbt_swag_insert(cnst,&(sys->constraint_set))
#define remove_variable(sys,var) do {xbt_swag_remove(var,&(sys->variable_set));\
                                 xbt_swag_remove(var,&(sys->saturated_variable_set));} while(0)
#define remove_constraint(sys,cnst) do {xbt_swag_remove(cnst,&(sys->constraint_set));\
                                        xbt_swag_remove(cnst,&(sys->saturated_constraint_set));} while(0)
#define remove_active_constraint(sys,cnst) xbt_swag_remove(cnst,&(sys->active_constraint_set))
#define make_constraint_active(sys,cnst) xbt_swag_insert(cnst,&(sys->active_constraint_set))
#define make_constraint_inactive(sys,cnst) remove_active_constraint(sys,cnst)

static void lmm_var_free(lmm_system_t sys, lmm_variable_t var);
static void lmm_cnst_free(lmm_system_t sys, lmm_constraint_t cnst);

void lmm_print(lmm_system_t sys);

extern double (*func_f_def) (lmm_variable_t, double);
extern double (*func_fp_def) (lmm_variable_t, double);
extern double (*func_fpi_def) (lmm_variable_t, double);

#endif /* _SURF_MAXMIN_PRIVATE_H */
