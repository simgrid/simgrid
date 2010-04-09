/* 	$Id$	 */

/* A few tests for the maxmin library                                       */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "xbt/sysdep.h"
#include "surf/maxmin.h"
#include "xbt/log.h"
#include "xbt/module.h"
#include <math.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

#define PRINT_VAR(var) DEBUG1(#var " = %g",lmm_variable_getvalue(var));
#define SHOW_EXPR(expr) DEBUG1(#expr " = %g",expr);

/*                               */
/*        ______                 */
/*  ==l1==  L2  ==L3==           */
/*        ------                 */
/*                               */

typedef enum {
  MAXMIN,
  LAGRANGE_RENO,
  LAGRANGE_VEGAS,
} method_t;

static double dichotomy(double func(double), double min, double max,
                        double min_error)
{
  double middle;
  double min_func, max_func, middle_func;

  double overall_error = 2 * min_error;

  min_func = func(min);
  max_func = func(max);

  if ((min_func > 0 && max_func > 0))
    return min - 1.0;
  if ((min_func < 0 && max_func < 0))
    return max + 1.0;
  if ((min_func > 0 && max_func < 0))
    abort();

  SHOW_EXPR(min_error);

  while (overall_error > min_error) {
    SHOW_EXPR(overall_error);
    if ((min_func > 0 && max_func > 0) ||
        (min_func < 0 && max_func < 0) || (min_func > 0 && max_func < 0)) {
      abort();
    }

    SHOW_EXPR(min);
    SHOW_EXPR(min_func);
    SHOW_EXPR(max);
    SHOW_EXPR(max_func);

    middle = (max + min) / 2.0;
    if ((min == middle) || (max == middle)) {
      break;
    }
    middle_func = func(middle);
    SHOW_EXPR(middle);
    SHOW_EXPR(middle_func);

    if (middle_func < 0) {
      min = middle;
      min_func = middle_func;
      overall_error = max_func - middle_func;
    } else if (middle_func > 0) {
      max = middle;
      max_func = middle_func;
      overall_error = middle_func - min_func;
    } else {
      overall_error = 0;
    }
  }
  return ((min + max) / 2.0);
}

double a_test_1 = 0;
double b_test_1 = 0;
static double diff_lagrange_test_1(double x)
{
  return -(3 / (1 + 3 * x * x / 2) -
           3 / (2 * (3 * (a_test_1 - x) * (a_test_1 - x) / 2 + 1)) +
           3 / (2 *
                (3 * (b_test_1 - a_test_1 + x) *
                 (b_test_1 - a_test_1 + x) / 2 + 1)));
}

void test1(method_t method);
void test1(method_t method)
{
  lmm_system_t Sys = NULL;
  lmm_constraint_t L1 = NULL;
  lmm_constraint_t L2 = NULL;
  lmm_constraint_t L3 = NULL;

  lmm_variable_t R_1_2_3 = NULL;
  lmm_variable_t R_1 = NULL;
  lmm_variable_t R_2 = NULL;
  lmm_variable_t R_3 = NULL;

  double a = 1.0, b = 10.0;

  if (method == LAGRANGE_VEGAS)
    lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
                                      func_vegas_fpi);
  else if (method == LAGRANGE_RENO)
    lmm_set_default_protocol_function(func_reno_f, func_reno_fpi,
                                      func_reno_fpi);

  Sys = lmm_system_new();
  L1 = lmm_constraint_new(Sys, (void *) "L1", a);
  L2 = lmm_constraint_new(Sys, (void *) "L2", b);
  L3 = lmm_constraint_new(Sys, (void *) "L3", a);

  R_1_2_3 = lmm_variable_new(Sys, (void *) "R 1->2->3", 1.0, -1.0, 3);
  R_1 = lmm_variable_new(Sys, (void *) "R 1", 1.0, -1.0, 1);
  R_2 = lmm_variable_new(Sys, (void *) "R 2", 1.0, -1.0, 1);
  R_3 = lmm_variable_new(Sys, (void *) "R 3", 1.0, -1.0, 1);

  lmm_update_variable_weight(Sys, R_1_2_3, 1.0);
  lmm_update_variable_weight(Sys, R_1, 1.0);
  lmm_update_variable_weight(Sys, R_2, 1.0);
  lmm_update_variable_weight(Sys, R_3, 1.0);

  lmm_expand(Sys, L1, R_1_2_3, 1.0);
  lmm_expand(Sys, L2, R_1_2_3, 1.0);
  lmm_expand(Sys, L3, R_1_2_3, 1.0);

  lmm_expand(Sys, L1, R_1, 1.0);

  lmm_expand(Sys, L2, R_2, 1.0);

  lmm_expand(Sys, L3, R_3, 1.0);


  if (method == MAXMIN) {
    lmm_solve(Sys);
  } else if (method == LAGRANGE_VEGAS) {
    double x = 3 * a / 4 - 3 * b / 8 +
      sqrt(9 * b * b + 4 * a * a - 4 * a * b) / 8;
    /* Computed with mupad and D_f=1.0 */
    double max_deviation = 0.0;
    if (x > a) {
      x = a;
    }
    if (x < 0) {
      x = 0;
    }

    lagrange_solve(Sys);

    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_1) - x));
    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_3) - x));
    max_deviation =
      MAX(max_deviation, fabs(lmm_variable_getvalue(R_2) - (b - a + x)));
    max_deviation =
      MAX(max_deviation, fabs(lmm_variable_getvalue(R_1_2_3) - (a - x)));

    if (max_deviation > MAXMIN_PRECISION) {
      WARN1("Max Deviation from optimal solution : %g", max_deviation);
      WARN1("Found x = %1.20f", x);
      WARN2("Deviation from optimal solution (R_1 = %g): %1.20f", x,
            lmm_variable_getvalue(R_1) - x);
      WARN2("Deviation from optimal solution (R_2 = %g): %1.20f",
            b - a + x, lmm_variable_getvalue(R_2) - (b - a + x));
      WARN2("Deviation from optimal solution (R_3 = %g): %1.20f", x,
            lmm_variable_getvalue(R_3) - x);
      WARN2("Deviation from optimal solution (R_1_2_3 = %g): %1.20f",
            a - x, lmm_variable_getvalue(R_1_2_3) - (a - x));
    }
  } else if (method == LAGRANGE_RENO) {
    double x;
    double max_deviation = 0.0;

    a_test_1 = a;
    b_test_1 = b;
    x = dichotomy(diff_lagrange_test_1, 0, a, 1e-13);

    if (x < 0)
      x = 0;
    if (x > a)
      x = a;
    lagrange_solve(Sys);

    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_1) - x));
    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_3) - x));
    max_deviation =
      MAX(max_deviation, fabs(lmm_variable_getvalue(R_2) - (b - a + x)));
    max_deviation =
      MAX(max_deviation, fabs(lmm_variable_getvalue(R_1_2_3) - (a - x)));

    if (max_deviation > MAXMIN_PRECISION) {
      WARN1("Max Deviation from optimal solution : %g", max_deviation);
      WARN1("Found x = %1.20f", x);
      WARN2("Deviation from optimal solution (R_1 = %g): %1.20f", x,
            lmm_variable_getvalue(R_1) - x);
      WARN2("Deviation from optimal solution (R_2 = %g): %1.20f",
            b - a + x, lmm_variable_getvalue(R_2) - (b - a + x));
      WARN2("Deviation from optimal solution (R_3 = %g): %1.20f", x,
            lmm_variable_getvalue(R_3) - x);
      WARN2("Deviation from optimal solution (R_1_2_3 = %g): %1.20f",
            a - x, lmm_variable_getvalue(R_1_2_3) - (a - x));
    }
  } else {
    xbt_assert0(0, "Invalid method");
  }

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);

  lmm_system_free(Sys);
}

void test2(method_t method);
void test2(method_t method)
{
  lmm_system_t Sys = NULL;
  lmm_constraint_t CPU1 = NULL;
  lmm_constraint_t CPU2 = NULL;

  lmm_variable_t T1 = NULL;
  lmm_variable_t T2 = NULL;


  if (method == LAGRANGE_VEGAS)
    lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
                                      func_vegas_fpi);
  else if (method == LAGRANGE_RENO)
    lmm_set_default_protocol_function(func_reno_f, func_reno_fp,
                                      func_reno_fpi);

  Sys = lmm_system_new();
  CPU1 = lmm_constraint_new(Sys, (void *) "CPU1", 200.0);
  CPU2 = lmm_constraint_new(Sys, (void *) "CPU2", 100.0);

  T1 = lmm_variable_new(Sys, (void *) "T1", 1.0, -1.0, 1);
  T2 = lmm_variable_new(Sys, (void *) "T2", 1.0, -1.0, 1);

  lmm_update_variable_weight(Sys, T1, 1.0);
  lmm_update_variable_weight(Sys, T2, 1.0);


  lmm_expand(Sys, CPU1, T1, 1.0);
  lmm_expand(Sys, CPU2, T2, 1.0);



  if (method == MAXMIN) {
    lmm_solve(Sys);
  } else if (method == LAGRANGE_VEGAS) {
    lagrange_solve(Sys);
  } else if (method == LAGRANGE_RENO) {
    lagrange_solve(Sys);
  } else {
    xbt_assert0(0, "Invalid method");
  }

  PRINT_VAR(T1);
  PRINT_VAR(T2);

  lmm_system_free(Sys);
}



void test3(method_t method);
void test3(method_t method)
{
  int flows = 11;
  int links = 10;

  int i = 0;
  int j = 0;

  double **A;

  lmm_system_t Sys = NULL;
  lmm_constraint_t *tmp_cnst = NULL;
  lmm_variable_t *tmp_var = NULL;
  char **tmp_name;


  /*array to add the the constraints of fictiv variables */
  double B[15] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,

    1, 1, 1, 1, 1
  };

  /*A = xbt_new0(double*, links + 5); */
  A = xbt_new0(double *, links + 5);

  for (i = 0; i < links + 5; i++) {
    A[i] = xbt_new0(double, flows + 5);
    for (j = 0; j < flows + 5; j++) {
      A[i][j] = 0.0;

      if (i >= links || j >= flows) {
        A[i][j] = 0.0;
      }
    }
  }

  /*matrix that store the constraints/topollogy */
  /*double A[15][16]=
     {{0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,    0, 0, 0, 0, 0},
     {0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0,    0, 0, 0, 0, 0},
     {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0,    0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,    0, 0, 0, 0, 0},
     {1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0,    0, 0, 0, 0, 0},
     {1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0,    0, 0, 0, 0, 0},
     {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1,    0, 0, 0, 0, 0},
     {0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1,    0, 0, 0, 0, 0},
     {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,    0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0,    0, 0, 0, 0, 0},

     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    1, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 1, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 1, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 1}
     }; */

  A[0][1] = 1.0;
  A[0][7] = 1.0;

  A[1][1] = 1.0;
  A[1][7] = 1.0;
  A[1][8] = 1.0;

  A[2][1] = 1.0;
  A[2][8] = 1.0;

  A[2][1] = 1.0;
  A[2][8] = 1.0;

  A[3][8] = 1.0;

  A[4][0] = 1.0;
  A[4][3] = 1.0;
  A[4][9] = 1.0;

  A[5][0] = 1.0;
  A[5][3] = 1.0;
  A[5][4] = 1.0;
  A[5][9] = 1.0;

  A[6][0] = 1.0;
  A[6][4] = 1.0;
  A[6][9] = 1.0;
  A[6][10] = 1.0;

  A[7][2] = 1.0;
  A[7][4] = 1.0;
  A[7][6] = 1.0;
  A[7][9] = 1.0;
  A[7][10] = 1.0;

  A[8][2] = 1.0;
  A[8][10] = 1.0;

  A[9][5] = 1.0;
  A[9][6] = 1.0;
  A[9][9] = 1.0;


  A[10][11] = 1.0;
  A[11][12] = 1.0;
  A[12][13] = 1.0;
  A[13][14] = 1.0;
  A[14][15] = 1.0;


  if (method == LAGRANGE_VEGAS)
    lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp,
                                      func_vegas_fpi);
  else if (method == LAGRANGE_RENO)
    lmm_set_default_protocol_function(func_reno_f, func_reno_fp,
                                      func_reno_fpi);

  Sys = lmm_system_new();



  tmp_name = xbt_new0(char *, 31);

  /*
   * Creates the constraints
   */
  tmp_cnst = xbt_new0(lmm_constraint_t, 15);
  for (i = 0; i < 15; i++) {
    tmp_name[i] = bprintf("C_%03d", i);
    tmp_cnst[i] = lmm_constraint_new(Sys, (void *) tmp_name[i], B[i]);
  }


  /*
   * Creates the variables
   */
  tmp_var = xbt_new0(lmm_variable_t, 16);
  for (j = 0; j < 16; j++) {
    tmp_name[i + j] = bprintf("X_%03d", j);
    tmp_var[j] =
      lmm_variable_new(Sys, (void *) tmp_name[i + j], 1.0, -1.0, 15);
    lmm_update_variable_weight(Sys, tmp_var[j], 1.0);
  }

  /*
   * Link constraints and variables
   */
  for (i = 0; i < 15; i++) {
    for (j = 0; j < 16; j++) {
      if (A[i][j]) {
        lmm_expand(Sys, tmp_cnst[i], tmp_var[j], 1.0);
      }
    }
  }



  if (method == MAXMIN) {
    lmm_solve(Sys);
  } else if (method == LAGRANGE_VEGAS) {
    lagrange_solve(Sys);
  } else if (method == LAGRANGE_RENO) {
    lagrange_solve(Sys);
  } else {
    xbt_assert0(0, "Invalid method");
  }

  for (j = 0; j < 16; j++) {
    PRINT_VAR(tmp_var[j]);
  }

  free(tmp_var);
  free(tmp_cnst);
  free(tmp_name);
  lmm_system_free(Sys);
}

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char **argv)
{
  xbt_init(&argc, argv);

  INFO0("***** Test 1 (Max-Min)");
  test1(MAXMIN);
  INFO0("***** Test 1 (Lagrange - Vegas)");
  test1(LAGRANGE_VEGAS);
  INFO0("***** Test 1 (Lagrange - Reno)");
  test1(LAGRANGE_RENO);



  INFO0("***** Test 2 (Max-Min)");
  test2(MAXMIN);
  INFO0("***** Test 2 (Lagrange - Vegas)");
  test2(LAGRANGE_VEGAS);
  INFO0("***** Test 2 (Lagrange - Reno)");
  test2(LAGRANGE_RENO);


  INFO0("***** Test 3 (Max-Min)");
  test3(MAXMIN);
  INFO0("***** Test 3 (Lagrange - Vegas)");
  test3(LAGRANGE_VEGAS);
  INFO0("***** Test 3 (Lagrange - Reno)");
  test3(LAGRANGE_RENO);

  xbt_exit();
  return 0;
}
