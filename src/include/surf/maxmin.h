/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

typedef long double FLOAT ;

typedef struct lmm_var_element *lmm_var_element_t;
typedef struct lmm_cnst_element *lmm_cnst_element_t;
typedef struct lmm_system *lmm_system_t;

lmm_system_t lmm_system_new(void);
void lmm_system_free(lmm_system_t system);

lmm_cnst_element_t lmm_add_constraint(lmm_system_t system, void *id, FLOAT bound_value);
lmm_var_element_t lmm_add_variable(lmm_system_t system, void *id, FLOAT weight_value, int number_of_constraints);
void lmm_extend_cnsts(lmm_system_t system, lmm_cnst_element_t cnsts,
		      lmm_var_element_t var, FLOAT value);

void lmm_del_constraint(lmm_system_t system, lmm_cnst_element_t cnsts);
void lmm_del_var(lmm_system_t system, lmm_var_element_t var);

void lmm_solve(lmm_system_t system, void ***p_id_table, int *p_id_table_size);
