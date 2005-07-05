/* 	$Id$	 */

/* A few tests for the maxmin library                                       */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "surf/maxmin.h"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test,"Messages specific for surf example");

#define PRINT_VAR(var) DEBUG1(#var " = %g\n",lmm_variable_getvalue(var));

/*                               */
/*        ______                 */
/*  ==l1==  L2  ==L3==           */
/*        ------                 */
/*                               */

void test(void);
void test(void)
{
  lmm_system_t Sys = NULL ;
  lmm_constraint_t L1 = NULL;
  lmm_constraint_t L2 = NULL;
  lmm_constraint_t L3 = NULL;

  lmm_variable_t R_1_2_3 = NULL;
  lmm_variable_t R_1 = NULL;
  lmm_variable_t R_2 = NULL;
  lmm_variable_t R_3 = NULL;

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

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);

  DEBUG0("\n");
  lmm_solve(Sys);

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);
  DEBUG0("\n");


  lmm_update_variable_weight(Sys,R_1_2_3,.5);
  lmm_solve(Sys);

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);

  lmm_system_free(Sys);
} 

void test2(void);
void test2(void)
{
  lmm_system_t Sys = NULL ;
  lmm_constraint_t CPU1 = NULL;
  lmm_constraint_t CPU2 = NULL;

  lmm_variable_t T1 = NULL;
  lmm_variable_t T2 = NULL;

  Sys = lmm_system_new();
  CPU1 = lmm_constraint_new(Sys, (void *) "CPU1", 200.0);
  CPU2 = lmm_constraint_new(Sys, (void *) "CPU2", 100.0);

  T1 = lmm_variable_new(Sys, (void *) "T1", 1.0 , -1.0 , 1);
  T2 = lmm_variable_new(Sys, (void *) "T2", 1.0 , -1.0 , 1);

  lmm_expand(Sys, CPU1, T1, 1.0);
  lmm_expand(Sys, CPU2, T2, 1.0);

  PRINT_VAR(T1);
  PRINT_VAR(T2);

  DEBUG0("\n");
  lmm_solve(Sys);

  PRINT_VAR(T1);
  PRINT_VAR(T2);

  DEBUG0("\n");

  lmm_system_free(Sys);
} 

int main(int argc, char **argv)
{
  DEBUG0("***** Test 1 ***** \n");
  test();
  DEBUG0("***** Test 2 ***** \n");
  test2();

  return 0;
}
