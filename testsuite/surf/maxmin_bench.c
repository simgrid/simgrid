/* 	$Id$	 */

/* A crash few tests for the maxmin library                                 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __BORLANDC__
#pragma hdrstop
#endif


#include <stdlib.h>
#include <stdio.h>
#include "surf/maxmin.h"
#include "xbt/xbt_os_time.h"
#include "xbt/sysdep.h"         /* time manipulation for benchmarking */

double date;

double float_random(double max);
double float_random(double max)
{
  return ((max * rand()) / (RAND_MAX + 1.0));
}

int int_random(int max);
int int_random(int max)
{
  return (int) (((max * 1.0) * rand()) / (RAND_MAX + 1.0));
}

void test(int nb_cnst, int nb_var, int nb_elem);
void test(int nb_cnst, int nb_var, int nb_elem)
{
  lmm_system_t Sys = NULL;
  lmm_constraint_t *cnst = xbt_new0(lmm_constraint_t, nb_cnst);
  lmm_variable_t *var = xbt_new0(lmm_variable_t, nb_var);
  int *used = xbt_new0(int, nb_cnst);
  int i, j, k;

  Sys = lmm_system_new();

  for (i = 0; i < nb_cnst; i++) {
    cnst[i] = lmm_constraint_new(Sys, NULL, float_random(10.0));
  }

  for (i = 0; i < nb_var; i++) {
    var[i] = lmm_variable_new(Sys, NULL, 1.0, -1.0, nb_elem);
    for (j = 0; j < nb_cnst; j++)
      used[j] = 0;
    for (j = 0; j < nb_elem; j++) {
      k = int_random(nb_cnst);
      if (used[k]) {
        j--;
        continue;
      }
      lmm_expand(Sys, cnst[k], var[i], float_random(1.0));
      used[k] = 1;
    }
  }

  printf("Starting to solve\n");
  date = xbt_os_time() * 1000000;
  lmm_solve(Sys);
  date = xbt_os_time() * 1000000 - date;

  lmm_system_free(Sys);
  free(cnst);
  free(var);
  free(used);
}

#ifdef __BORLANDC__
#pragma argsused
#endif


int main(int argc, char **argv)
{
  int nb_cnst = 2000;
  int nb_var = 2000;
  int nb_elem = 80;
  date = xbt_os_time() * 1000000;
  test(nb_cnst, nb_var, nb_elem);
  printf("One shot execution time for a total of %d constraints, "
         "%d variables with %d active constraint each : %g microsecondes \n",
         nb_cnst, nb_var, nb_elem, date);
  return 0;
}
