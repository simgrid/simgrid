/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

typedef long double FLOAT;

typedef struct lmm_variable *lmm_variable_t;
typedef struct lmm_constraint *lmm_constraint_t;
typedef struct lmm_system *lmm_system_t;

lmm_system_t lmm_system_new(void);
void lmm_system_free(lmm_system_t sys);

lmm_constraint_t lmm_constraint_new(lmm_system_t sys,	/* void *id, */
				    FLOAT bound_value);
void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst);

lmm_variable_t lmm_variable_new(lmm_system_t sys,	/* void *id, */
				FLOAT weight_value, FLOAT bound,
				int number_of_constraints);
void lmm_variable_free(lmm_system_t sys, lmm_variable_t var);
void lmm_add_constraint(lmm_system_t sys, lmm_constraint_t cnst,
			lmm_variable_t var, FLOAT value);

void lmm_solve(lmm_system_t sys);
