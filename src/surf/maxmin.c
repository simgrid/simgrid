/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/error.h"
#include "maxmin_private.h"
#include <stdlib.h>

lmm_system_t lmm_system_new(void)
{
  lmm_system_t l = NULL;
  s_lmm_variable_t var;
  s_lmm_constraint_t cnst;

  l = xbt_new0(s_lmm_system_t, 1);

  xbt_swag_init(&(l->variable_set),
		xbt_swag_offset(var, variable_set_hookup));
  xbt_swag_init(&(l->constraint_set),
		xbt_swag_offset(cnst, constraint_set_hookup));

  return l;
}

void lmm_system_free(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = NULL;

  while ((var = extract_variable(sys)))
    lmm_var_free(var);

  while ((cnst = extract_constraint(sys)))
    lmm_cnst_free(cnst);

  xbt_free(sys);
}

static void lmm_var_free(lmm_variable_t var)
{
  int i;
  lmm_element_t elem = NULL;

  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    xbt_swag_extract(elem, &(elem->constraint->element_set));
  }
  xbt_free(var->cnsts);
  xbt_free(var);
}

static void lmm_cnst_free(lmm_constraint_t cnst)
{
/*   xbt_assert0(xbt_fifo_size(&(cnst->row)), */
/* 		 "This list should be empty!"); */
  xbt_free(cnst);
}

lmm_constraint_t lmm_constraint_new(lmm_system_t sys,	/* void *id, */
				    FLOAT bound_value)
{
  lmm_constraint_t cnst = NULL;
  s_lmm_element_t elem;

  cnst = xbt_new0(s_lmm_constraint_t, 1);
/*   cnst->id = id; */
  xbt_swag_init(&(cnst->element_set),
		xbt_swag_offset(elem, element_set_hookup));

  cnst->bound = bound_value;
  cnst->usage = UNUSED_CONSTRAINT;
  insert_constraint(sys, cnst);

  return cnst;
}

void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst)
{
  remove_constraint(sys, cnst);
  lmm_cnst_free(cnst);
}

lmm_variable_t lmm_variable_new(lmm_system_t sys,	/* void *id, */
				FLOAT weight_value, FLOAT bound,
				int number_of_constraints)
{
  lmm_variable_t var = NULL;

  var = xbt_new0(s_lmm_variable_t, 1);
/*   var->id = id; */
  var->cnsts = xbt_new0(s_lmm_element_t, number_of_constraints);
  var->cnsts_size = number_of_constraints;
  /* var->cnsts_number = 0; *//* Useless because of the calloc  */
  var->weight_value = weight_value;
  var->bound = bound;
  var->value = -1;
  insert_variable(sys, var);

  return var;
}

void lmm_variable_free(lmm_system_t sys, lmm_variable_t var)
{
  remove_variable(sys, var);
  lmm_var_free(var);
}

void lmm_add_constraint(lmm_system_t sys, lmm_constraint_t cnst,
			lmm_variable_t var, FLOAT value)
{
  lmm_element_t elem = NULL;

/*   xbt_assert0(var->cnsts_number<var->cnsts_size,"Too much constraint"); */

  elem = &(var->cnsts[var->cnsts_number++]);

  elem->value = value;
  elem->constraint = cnst;

  insert_elem_in_constraint(elem);

}

void lmm_solve(lmm_system_t sys)
{
  void **id_table = NULL;

  /* Compute Usage and store the variables that reach the maximum */

  do {
    /* Fix the variables that have to be */

    /* Update Usage and store the variables that reach the maximum */

  } while (1);

}
