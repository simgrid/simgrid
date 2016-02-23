/* A crash few tests for the maxmin library                                 */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/maxmin.h"
#include "xbt/module.h"
#include "xbt/xbt_os_time.h"
#include "xbt/sysdep.h"         /* time manipulation for benchmarking */

#include <stdlib.h>
#include <stdio.h>

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

void test(int nb_cnst, int nb_var, int nb_elem, int pw_base_limit, int pw_max_limit, float rate_no_limit, int max_share);
void test(int nb_cnst, int nb_var, int nb_elem, int pw_base_limit, int pw_max_limit, float rate_no_limit, int max_share)
{
  lmm_system_t Sys = NULL;
  lmm_constraint_t *cnst = xbt_new0(lmm_constraint_t, nb_cnst);
  lmm_variable_t *var = xbt_new0(lmm_variable_t, nb_var);
  int *used = xbt_new0(int, nb_cnst);
  int i, j, k,l;
  
  Sys = lmm_system_new(1);

  for (i = 0; i < nb_cnst; i++) {
    cnst[i] = lmm_constraint_new(Sys, NULL, float_random(10.0));
    if(rate_no_limit>float_random(1.0))
      //Look at what happens when there is no concurrency limit 
      l=-1;
    else
      //Badly logarithmically random concurrency limit in [2^pw_base_limit+1,2^pw_base_limit+2^pw_max_limit]
      l=(1<<pw_base_limit)+(1<<int_random(pw_max_limit));
 
    lmm_constraint_concurrency_limit_set(cnst[i],l );
  }
  
  for (i = 0; i < nb_var; i++) {
    var[i] = lmm_variable_new(Sys, NULL, 1.0, -1.0, nb_elem);
    //Have a few variables with a concurrency share of two (e.g. cross-traffic in some cases)
    lmm_variable_concurrency_share_set(var[i],1+int_random(max_share));
      
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

  printf("Max concurrency:\n");
  l=0;
  for (i = 0; i < nb_cnst; i++) {
    j=lmm_constraint_concurrency_maximum_get(cnst[i]);
    k=lmm_constraint_concurrency_limit_get(cnst[i]);
    xbt_assert(k<0 || j<=k);
    if(j>l)
      l=j;
    printf("(%i):%i/%i ",i,j,k);
    lmm_constraint_concurrency_maximum_reset(cnst[i]);
    xbt_assert(!lmm_constraint_concurrency_maximum_get(cnst[i]));
    if(i%10==9)
       printf("\n");    
  }
  printf("\nTotal maximum concurrency is %i\n",l);

  lmm_print(Sys);
  
  for (i = 0; i < nb_var; i++)
    lmm_variable_free(Sys, var[i]);
  lmm_system_free(Sys);
  free(cnst);
  free(var);
  free(used);
  
}

int main(int argc, char **argv)
{
  int nb_cnst = 2000;
  int nb_var = 2000;
  int nb_elem; 
  int pw_base_limit=5; 
  int pw_max_limit=8;
  float rate_no_limit=0.2;
  int max_share=1<<(pw_base_limit/2+1);
  
  //If you want to test concurrency, you need nb_elem >> 2^pw_base_limit:
  nb_elem= (1<<pw_base_limit)+(1<<(8*pw_max_limit/10));
  //Otherwise, just set it to a constant value (and set rate_no_limit to 1.0):
  //nb_elem=200
  
  xbt_init(&argc, argv);
  date = xbt_os_time() * 1000000;
  test(nb_cnst, nb_var, nb_elem, pw_base_limit, pw_max_limit, rate_no_limit,max_share);    
  printf("One shot execution time for a total of %d constraints, "
         "%d variables with %d active constraint each : %g microseconds \n",
         nb_cnst, nb_var, nb_elem, date);
  return 0;
}
