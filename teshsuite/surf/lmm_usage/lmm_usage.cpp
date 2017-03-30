/* A few tests for the maxmin library                                       */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "surf/maxmin.h"
#include "xbt/log.h"
#include "xbt/module.h"
#include <math.h>
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

#define PRINT_VAR(var) XBT_DEBUG(#var " = %g",lmm_variable_getvalue(var))
#define SHOW_EXPR(expr) XBT_DEBUG(#expr " = %g",expr)

/*        ______                 */
/*  ==l1==  L2  ==L3==           */
/*        ------                 */

typedef enum {
  MAXMIN,
  LAGRANGE_RENO,
  LAGRANGE_VEGAS
} method_t;

static double dichotomy(double func(double), double min, double max, double min_error)
{
  double overall_error = 2 * min_error;

  double min_func = func(min);
  double max_func = func(max);

  if ((min_func > 0 && max_func > 0))
    return min - 1.0;
  if ((min_func < 0 && max_func < 0))
    return max + 1.0;
  if ((min_func > 0 && max_func < 0))
    abort();

  SHOW_EXPR(min_error);

  while (overall_error > min_error) {
    SHOW_EXPR(overall_error);
    xbt_assert(min_func <= 0 || max_func <= 0);
    xbt_assert(min_func >= 0 || max_func >= 0);
    xbt_assert(min_func <= 0 || max_func >= 0);

    SHOW_EXPR(min);
    SHOW_EXPR(min_func);
    SHOW_EXPR(max);
    SHOW_EXPR(max_func);

    double middle = (max + min) / 2.0;
    if (fabs(min - middle) < 1e-12 || fabs(max - middle) < 1e-12) {
      break;
    }
    double middle_func = func(middle);
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
  return -(3 / (1 + 3 * x * x / 2) - 3 / (2 * (3 * (a_test_1 - x) * (a_test_1 - x) / 2 + 1)) +
           3 / (2 * (3 * (b_test_1 - a_test_1 + x) * (b_test_1 - a_test_1 + x) / 2 + 1)));
}

static void test1(method_t method)
{
  double a = 1.0;
  double b = 10.0;

  if (method == LAGRANGE_VEGAS)
    lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp, func_vegas_fpi);
  else if (method == LAGRANGE_RENO)
    lmm_set_default_protocol_function(func_reno_f, func_reno_fpi, func_reno_fpi);

  lmm_system_t Sys = lmm_system_new(1);
  lmm_constraint_t L1 = lmm_constraint_new(Sys, nullptr, a);
  lmm_constraint_t L2 = lmm_constraint_new(Sys, nullptr, b);
  lmm_constraint_t L3 = lmm_constraint_new(Sys, nullptr, a);

  lmm_variable_t R_1_2_3 = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 3);
  lmm_variable_t R_1 = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 1);
  lmm_variable_t R_2 = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 1);
  lmm_variable_t R_3 = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 1);

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
  } else {
    double x;
    if (method == LAGRANGE_VEGAS) {
      x = 3 * a / 4 - 3 * b / 8 + sqrt(9 * b * b + 4 * a * a - 4 * a * b) / 8;
      /* Computed with mupad and D_f=1.0 */
      if (x > a) {
        x = a;
      }
      if (x < 0) {
        x = 0;
      }
    } else if (method == LAGRANGE_RENO) {
      a_test_1 = a;
      b_test_1 = b;
      x = dichotomy(diff_lagrange_test_1, 0, a, 1e-13);

      if (x < 0)
        x = 0;
      if (x > a)
        x = a;
    } else {
      xbt_die( "Invalid method");
    }

    lagrange_solve(Sys);

    double max_deviation = 0.0;
    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_1) - x));
    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_3) - x));
    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_2) - (b - a + x)));
    max_deviation = MAX(max_deviation, fabs(lmm_variable_getvalue(R_1_2_3) - (a - x)));

    if (max_deviation > 0.00001) { // Legacy value used in lagrange.c
      XBT_WARN("Max Deviation from optimal solution : %g", max_deviation);
      XBT_WARN("Found x = %1.20f", x);
      XBT_WARN("Deviation from optimal solution (R_1 = %g): %1.20f", x, lmm_variable_getvalue(R_1) - x);
      XBT_WARN("Deviation from optimal solution (R_2 = %g): %1.20f", b - a + x,
               lmm_variable_getvalue(R_2) - (b - a + x));
      XBT_WARN("Deviation from optimal solution (R_3 = %g): %1.20f", x, lmm_variable_getvalue(R_3) - x);
      XBT_WARN("Deviation from optimal solution (R_1_2_3 = %g): %1.20f", a - x,
               lmm_variable_getvalue(R_1_2_3) - (a - x));
    }
  }

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);

  lmm_variable_free(Sys, R_1_2_3);
  lmm_variable_free(Sys, R_1);
  lmm_variable_free(Sys, R_2);
  lmm_variable_free(Sys, R_3);
  lmm_system_free(Sys);
}

static void test2(method_t method)
{
  if (method == LAGRANGE_VEGAS)
    lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp, func_vegas_fpi);
  if (method == LAGRANGE_RENO)
    lmm_set_default_protocol_function(func_reno_f, func_reno_fp, func_reno_fpi);

  lmm_system_t Sys = lmm_system_new(1);
  lmm_constraint_t CPU1 = lmm_constraint_new(Sys, nullptr, 200.0);
  lmm_constraint_t CPU2 = lmm_constraint_new(Sys, nullptr, 100.0);

  lmm_variable_t T1 = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 1);
  lmm_variable_t T2 = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 1);

  lmm_update_variable_weight(Sys, T1, 1.0);
  lmm_update_variable_weight(Sys, T2, 1.0);

  lmm_expand(Sys, CPU1, T1, 1.0);
  lmm_expand(Sys, CPU2, T2, 1.0);

  if (method == MAXMIN) {
    lmm_solve(Sys);
  } else if (method == LAGRANGE_VEGAS || method == LAGRANGE_RENO) {
    lagrange_solve(Sys);
  } else {
    xbt_die("Invalid method");
  }

  PRINT_VAR(T1);
  PRINT_VAR(T2);

  lmm_variable_free(Sys, T1);
  lmm_variable_free(Sys, T2);
  lmm_system_free(Sys);
}

static void test3(method_t method)
{
  int flows = 11;
  int links = 10;

  double **A = xbt_new0(double *, links + 5);
  /* array to add the constraints of fictitious variables */
  double B[15] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 1, 1, 1, 1, 1 };

  for (int i = 0; i < links + 5; i++) {
    A[i] = xbt_new0(double, flows + 5);
    for (int j = 0; j < flows + 5; j++) {
      A[i][j] = 0.0;

      if (i >= links || j >= flows) {
        A[i][j] = 0.0;
      }
    }
  }

  /*matrix that store the constraints/topology */
  A[0][1] = A[0][7] =                                1.0;
  A[1][1] = A[1][7] = A[1][8] =                      1.0;
  A[2][1] = A[2][8] =                                1.0;
  A[3][8] =                                          1.0;
  A[4][0] = A[4][3] = A[4][9] =                      1.0;
  A[5][0] = A[5][3] = A[5][4] = A[5][9] =            1.0;
  A[6][0] = A[6][4] = A[6][9] = A[6][10] =           1.0;
  A[7][2] = A[7][4] = A[7][6] = A[7][9] = A[7][10] = 1.0;
  A[8][2] = A[8][10] =                               1.0;
  A[9][5] = A[9][6] = A[9][9] =                      1.0;
  A[10][11] =                                        1.0;
  A[11][12] =                                        1.0;
  A[12][13] =                                        1.0;
  A[13][14] =                                        1.0;
  A[14][15] =                                        1.0;

  if (method == LAGRANGE_VEGAS)
    lmm_set_default_protocol_function(func_vegas_f, func_vegas_fp, func_vegas_fpi);
  if (method == LAGRANGE_RENO)
    lmm_set_default_protocol_function(func_reno_f, func_reno_fp, func_reno_fpi);

  lmm_system_t Sys = lmm_system_new(1);

  /* Creates the constraints */
  lmm_constraint_t *tmp_cnst = xbt_new0(lmm_constraint_t, 15);
  for (int i = 0; i < 15; i++) 
    tmp_cnst[i] = lmm_constraint_new(Sys, nullptr, B[i]);

  /* Creates the variables */
  lmm_variable_t *tmp_var = xbt_new0(lmm_variable_t, 16);
  for (int j = 0; j < 16; j++) {
    tmp_var[j] = lmm_variable_new(Sys, nullptr, 1.0, -1.0, 15);
    lmm_update_variable_weight(Sys, tmp_var[j], 1.0);
  }

  /* Link constraints and variables */
  for (int i = 0; i < 15; i++)
    for (int j = 0; j < 16; j++)
      if (A[i][j])
        lmm_expand(Sys, tmp_cnst[i], tmp_var[j], 1.0);

  if (method == MAXMIN) {
    lmm_solve(Sys);
  } else if (method == LAGRANGE_VEGAS) {
    lagrange_solve(Sys);
  } else if (method == LAGRANGE_RENO) {
    lagrange_solve(Sys);
  } else {
    xbt_die("Invalid method");
  }

  for (int j = 0; j < 16; j++)
    PRINT_VAR(tmp_var[j]);

  for (int j = 0; j < 16; j++)
    lmm_variable_free(Sys, tmp_var[j]);
  xbt_free(tmp_var);
  xbt_free(tmp_cnst);
  lmm_system_free(Sys);
  for (int i = 0; i < links + 5; i++)
    xbt_free(A[i]);
  xbt_free(A);
}

int main(int argc, char **argv)
{
  XBT_INFO("***** Test 1 (Max-Min)");
  test1(MAXMIN);
  XBT_INFO("***** Test 1 (Lagrange - Vegas)");
  test1(LAGRANGE_VEGAS);
  XBT_INFO("***** Test 1 (Lagrange - Reno)");
  test1(LAGRANGE_RENO);

  XBT_INFO("***** Test 2 (Max-Min)");
  test2(MAXMIN);
  XBT_INFO("***** Test 2 (Lagrange - Vegas)");
  test2(LAGRANGE_VEGAS);
  XBT_INFO("***** Test 2 (Lagrange - Reno)");
  test2(LAGRANGE_RENO);

  XBT_INFO("***** Test 3 (Max-Min)");
  test3(MAXMIN);
  XBT_INFO("***** Test 3 (Lagrange - Vegas)");
  test3(LAGRANGE_VEGAS);
  XBT_INFO("***** Test 3 (Lagrange - Reno)");
  test3(LAGRANGE_RENO);

  return 0;
}
