/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_MAXMIN_H
#define _SURF_MAXMIN_H

typedef long double xbt_maxmin_float_t;
#define XBT_MAXMIN_FLOAT_T "%Lg"	/* for printing purposes */

typedef struct lmm_variable *lmm_variable_t;
typedef struct lmm_constraint *lmm_constraint_t;
typedef struct lmm_system *lmm_system_t;

lmm_system_t lmm_system_new(void);
void lmm_system_free(lmm_system_t sys);

lmm_constraint_t lmm_constraint_new(lmm_system_t sys, void *id,
				    xbt_maxmin_float_t bound_value);
void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst);

lmm_variable_t lmm_variable_new(lmm_system_t sys, void *id,
				xbt_maxmin_float_t weight_value,
				xbt_maxmin_float_t bound,
				int number_of_constraints);
void lmm_variable_free(lmm_system_t sys, lmm_variable_t var);
xbt_maxmin_float_t lmm_variable_getvalue(lmm_variable_t var);


void lmm_expand(lmm_system_t sys, lmm_constraint_t cnst,
		lmm_variable_t var, xbt_maxmin_float_t value);

void lmm_update(lmm_system_t sys, lmm_constraint_t cnst,
		lmm_variable_t var, xbt_maxmin_float_t value);
void lmm_update_variable_bound(lmm_variable_t var,
			       xbt_maxmin_float_t bound);
void lmm_update_variable_weight(lmm_variable_t var,
				xbt_maxmin_float_t weight);
void lmm_update_constraint_bound(lmm_constraint_t cnst,
				 xbt_maxmin_float_t bound);

int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst);

void lmm_solve(lmm_system_t sys);
#endif				/* _SURF_MAXMIN_H */
