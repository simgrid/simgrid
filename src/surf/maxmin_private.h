/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/maxmin.h"
#include "xbt/swag.h"

typedef struct lmm_element {
  s_xbt_swag_hookup_t element_set_hookup;
  lmm_constraint_t constraint;
  FLOAT value;
} s_lmm_element_t, *lmm_element_t;
#define insert_elem_in_constraint(elem) xbt_swag_insert(elem,&(elem->constraint->element_set))

typedef struct lmm_constraint {
  s_xbt_swag_hookup_t constraint_set_hookup;
  /*   void *id; */
  s_xbt_swag_t element_set;	/* a list of lmm_mat_element_t */
  FLOAT bound;
  FLOAT usage;
} s_lmm_constraint_t;

typedef struct lmm_variable {
  s_xbt_swag_hookup_t variable_set_hookup;
  /*   void *id; */
  s_lmm_element_t *cnsts;
  int cnsts_size;
  int cnsts_number;
  FLOAT weight_value;
  FLOAT bound;
  FLOAT value;
} s_lmm_variable_t;

typedef struct lmm_system {
  s_xbt_swag_t variable_set;	/* a list of lmm_variable_t */
  s_xbt_swag_t constraint_set;	/* a list of lmm_constraint_t */

  s_xbt_swag_t active_constraint_set;	/* a list of lmm_constraint_t */
} s_lmm_system_t;

#define extract_variable(sys) xbt_swag_extract(xbt_swag_getFirst(&(sys->variable_set)),&(sys->variable_set))
#define extract_constraint(sys) xbt_swag_extract(xbt_swag_getFirst(&(sys->constraint_set)),&(sys->constraint_set))
#define insert_variable(sys,var) xbt_swag_insert(var,&(sys->variable_set))
#define insert_constraint(sys,cnst) xbt_swag_insert(cnst,&(sys->constraint_set))
#define remove_variable(sys,var) xbt_swag_extract(var,&(sys->variable_set))
#define remove_constraint(sys,cnst) xbt_swag_extract(cnst,&(sys->constraint_set))

static void lmm_var_free(lmm_variable_t e);
static void lmm_cnst_free(lmm_constraint_t cnst);

/* #define UNDEFINED_VALUE -1.0 */
#define UNUSED_CONSTRAINT -2.0
