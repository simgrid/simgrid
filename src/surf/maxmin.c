/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/error.h"
#include "maxmin_private.h"
#include <stdlib.h>

/* extern lmm_system_t Sys; */
/* extern lmm_constraint_t L1; */
/* extern lmm_constraint_t L2; */
/* extern lmm_constraint_t L3; */

/* extern lmm_variable_t R_1_2_3; */
/* extern lmm_variable_t R_1; */
/* extern lmm_variable_t R_2; */
/* extern lmm_variable_t R_3; */


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

  xbt_swag_init(&(l->active_constraint_set),
		xbt_swag_offset(cnst, active_constraint_set_hookup));

  xbt_swag_init(&(l->saturated_variable_set),
		xbt_swag_offset(var, saturated_variable_set_hookup));
  xbt_swag_init(&(l->saturated_constraint_set),
		xbt_swag_offset(cnst, saturated_constraint_set_hookup));

  return l;
}

void lmm_system_free(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = NULL;

  while ((var = extract_variable(sys)))
    lmm_var_free(sys, var);

  while ((cnst = extract_constraint(sys)))
    lmm_cnst_free(sys, cnst);

  xbt_free(sys);
}

static void lmm_var_free(lmm_system_t sys, lmm_variable_t var)
{
  int i;
  lmm_element_t elem = NULL;

  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    xbt_swag_remove(elem, &(elem->constraint->element_set));
    if (xbt_swag_size(&(elem->constraint->element_set)))
      make_constraint_inactive(sys, elem->constraint);
  }
  xbt_free(var->cnsts);
  xbt_free(var);
}

static void lmm_cnst_free(lmm_system_t sys, lmm_constraint_t cnst)
{
/*   xbt_assert0(xbt_fifo_size(&(cnst->row)), */
/* 		 "This list should be empty!"); */
  remove_active_constraint(sys, cnst);
  xbt_free(cnst);
}

lmm_constraint_t lmm_constraint_new(lmm_system_t sys, void *id,
				    xbt_maxmin_float_t bound_value)
{
  lmm_constraint_t cnst = NULL;
  s_lmm_element_t elem;

  cnst = xbt_new0(s_lmm_constraint_t, 1);
/*   cnst->id = id; */
  xbt_swag_init(&(cnst->element_set),
		xbt_swag_offset(elem, element_set_hookup));
  xbt_swag_init(&(cnst->active_element_set),
		xbt_swag_offset(elem, active_element_set_hookup));

  cnst->bound = bound_value;
  cnst->usage = 0;
  insert_constraint(sys, cnst);

  cnst->id = id;

  return cnst;
}

void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst)
{
  remove_constraint(sys, cnst);
  lmm_cnst_free(sys, cnst);
}

lmm_variable_t lmm_variable_new(lmm_system_t sys, void *id,
				xbt_maxmin_float_t weight,
				xbt_maxmin_float_t bound,
				int number_of_constraints)
{
  lmm_variable_t var = NULL;

  var = xbt_new0(s_lmm_variable_t, 1);
/*   var->id = id; */
  var->cnsts = xbt_new0(s_lmm_element_t, number_of_constraints);
  var->cnsts_size = number_of_constraints;
  /* var->cnsts_number = 0; *//* Useless because of the calloc  */
  var->weight = weight;
  var->bound = bound;
  var->value = -1;
  insert_variable(sys, var);

  var->id = id;

  return var;
}

void lmm_variable_free(lmm_system_t sys, lmm_variable_t var)
{
  remove_variable(sys, var);
  lmm_var_free(sys, var);
}

xbt_maxmin_float_t lmm_variable_getvalue(lmm_variable_t var)
{
  return (var->value);
}

void lmm_expand(lmm_system_t sys, lmm_constraint_t cnst,
		lmm_variable_t var, xbt_maxmin_float_t value)
{
  lmm_element_t elem = NULL;

  if (var->cnsts_number >= var->cnsts_size)
    abort();

  elem = &(var->cnsts[var->cnsts_number++]);

  elem->value = value;
  elem->constraint = cnst;
  elem->variable = var;

  insert_elem_in_constraint(elem);

  make_constraint_active(sys, cnst);
}

static void saturated_constraints_update(lmm_system_t sys,
					 lmm_constraint_t cnst,
					 xbt_maxmin_float_t * min_usage)
{
  lmm_constraint_t useless_cnst = NULL;

  if (cnst->remaining <= 0)
    return;
  if ((*min_usage < 0) || (*min_usage > cnst->remaining / cnst->usage)) {
    xbt_swag_t active_elem_list = &(cnst->active_element_set);

    *min_usage = cnst->remaining / cnst->usage;

    while ((useless_cnst = xbt_swag_getFirst(&(sys->saturated_constraint_set))))
      xbt_swag_remove(useless_cnst, &(sys->saturated_constraint_set));

    xbt_swag_insert(cnst, &(sys->saturated_constraint_set));
  } else if (*min_usage == cnst->remaining / cnst->usage) {
    xbt_swag_insert(cnst, &(sys->saturated_constraint_set));
  }
}

static void saturated_variables_update(lmm_system_t sys)
{
  lmm_constraint_t cnst = NULL;
  xbt_swag_t cnst_list = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t elem_list = NULL;

  cnst_list = &(sys->saturated_constraint_set);
  while ((cnst = xbt_swag_getFirst(cnst_list))) {
/*   xbt_swag_foreach(cnst, cnst_list) { */
    elem_list = &(cnst->active_element_set);
    xbt_swag_foreach(elem, elem_list)
	xbt_swag_insert(elem->variable, &(sys->saturated_variable_set));
    xbt_swag_remove(cnst, cnst_list);
  }

}

void lmm_solve(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  xbt_maxmin_float_t min_usage = -1;

  /* Init */
  var_list = &(sys->variable_set);
  xbt_swag_foreach(var, var_list) {
    var->value = -1;
  }


  /* Compute Usage and store the variables that reach the maximum */

  cnst_list = &(sys->active_constraint_set);
  xbt_swag_foreach(cnst, cnst_list) {
    /* INIT */
    elem_list = &(cnst->element_set);
    cnst->remaining = cnst->bound;
    cnst->usage = 0;
    xbt_swag_foreach(elem, elem_list) {
      cnst->usage += elem->value / elem->variable->weight;
      insert_active_elem_in_constraint(elem);
    }

    /* Saturated constraints update */
    saturated_constraints_update(sys, cnst, &min_usage);
  }

  saturated_variables_update(sys);

  /* Saturated variables update */

  do {
    /* Fix the variables that have to be */
    var_list = &(sys->saturated_variable_set);


    xbt_swag_foreach(var, var_list) {
      /* First check if some of these variables have reach their upper
         bound and update min_usage accordingly. */
      if ((var->bound > 0) && (var->bound / var->weight < min_usage)) {
	min_usage = var->bound / var->weight;
      }
    }


    while ((var = xbt_swag_getFirst(var_list))) {
      int i;

      var->value = min_usage / var->weight;

      /* Update usage */

      for (i = 0; i < var->cnsts_number; i++) {
	elem = &var->cnsts[i];
	cnst = elem->constraint;
	cnst->remaining -= elem->value * var->value;
	cnst->usage -= elem->value / var->weight;
	remove_active_elem_in_constraint(elem);
      }
      xbt_swag_remove(var, var_list);
    }

    /* Find out which variables reach the maximum */
    cnst_list = &(sys->active_constraint_set);
    min_usage = -1;
    xbt_swag_foreach(cnst, cnst_list) {
      saturated_constraints_update(sys, cnst, &min_usage);
    }
    saturated_variables_update(sys);

  } while (xbt_swag_size(&(sys->saturated_variable_set)));
}

/* Not a O(1) function */

void lmm_update(lmm_system_t sys, lmm_constraint_t cnst,
		lmm_variable_t var, xbt_maxmin_float_t value)
{
  int i;
  lmm_element_t elem = NULL;

  for (i = 0; i < var->cnsts_number; i++)
    if (var->cnsts[i].constraint == cnst) {
      elem->value = value;
      return;
    }
}

void lmm_update_variable_bound(lmm_variable_t var,
			       xbt_maxmin_float_t bound)
{
  var->bound = bound;
}

void lmm_update_variable_weight(lmm_variable_t var,
				xbt_maxmin_float_t weight)
{
  var->weight = weight;
}

void lmm_update_constraint_bound(lmm_constraint_t cnst,
				 xbt_maxmin_float_t bound)
{
  cnst->bound = bound;
}

int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst)
{
  return xbt_swag_belongs(cnst,&(sys->active_constraint_set));
}


/* void lmm_print(lmm_system_t sys) */
/* { */
/*   lmm_variable_t var = NULL; */
/*   lmm_constraint_t cnst = NULL; */
/*   lmm_element_t elem = NULL; */
/*   xbt_swag_t cnst_list = NULL; */
/*   xbt_swag_t var_list = NULL; */
/*   xbt_swag_t elem_list = NULL; */

/*   var_list = &(sys->variable_set); */
/*   xbt_swag_foreach(var, var_list) { */
/*     printf("%s : {\n",var->id); */
/*     printf("\t %s : {\n",var->id); */
/*   } */
/* } */
