/* A few tests for the maxmin library                                       */

/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "surf/maxmin.h"

/*                               */
/*        ______                 */
/*  ==l1==  L2  ==L3==           */
/*        ------                 */
/*                               */

  lmm_system_t Sys = NULL ;
  lmm_constraint_t L1 = NULL;
  lmm_constraint_t L2 = NULL;
  lmm_constraint_t L3 = NULL;

  lmm_variable_t R_1_2_3 = NULL;
  lmm_variable_t R_1 = NULL;
  lmm_variable_t R_2 = NULL;
  lmm_variable_t R_3 = NULL;

void test(void);
void test(void)
{

  Sys = lmm_system_new();
  L1 = lmm_constraint_new(Sys, (void *) "L1", 1.0);
  L2 = lmm_constraint_new(Sys, (void *) "L2", 10.0);
  L3 = lmm_constraint_new(Sys, (void *) "L3", 1.0);

  R_1_2_3 = lmm_variable_new(Sys, (void *) "R 1->2->3", 1.0 , -1.0 , 3);
  R_1 = lmm_variable_new(Sys, (void *) "R 1", 1.0 , -1.0 , 1);
  R_2 = lmm_variable_new(Sys, (void *) "R 2", 1.0 , -1.0 , 1);
  R_3 = lmm_variable_new(Sys, (void *) "R 3", 1.0 , -1.0 , 1);

  lmm_expand(Sys, L1, R_1_2_3, 1.0);
  lmm_expand(Sys, L2, R_1_2_3, 1.0);
  lmm_expand(Sys, L3, R_1_2_3, 1.0);

  lmm_expand(Sys, L1, R_1, 1.0);

  lmm_expand(Sys, L2, R_2, 1.0);

  lmm_expand(Sys, L3, R_3, 1.0);

#define AFFICHE(var) printf(#var " = %Lg\n",lmm_variable_getvalue(var));
  AFFICHE(R_1_2_3);
  AFFICHE(R_1);
  AFFICHE(R_2);
  AFFICHE(R_3);

  printf("\n");
  lmm_solve(Sys);

  AFFICHE(R_1_2_3);
  AFFICHE(R_1);
  AFFICHE(R_2);
  AFFICHE(R_3);
  printf("\n");


  lmm_update_variable_weight(R_1_2_3,.5);
  lmm_solve(Sys);

  AFFICHE(R_1_2_3);
  AFFICHE(R_1);
  AFFICHE(R_2);
  AFFICHE(R_3);
#undef AFFICHE

  lmm_system_free(Sys);
} 


int main(int argc, char **argv)
{
  test();
  return 0;
}
