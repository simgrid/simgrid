/* A few tests for the maxmin library                                       */

/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "surf/maxmin.h"

/*                               */
/*        ______                 */
/*  ==l1==  l2  ==l3==           */
/*        ------                 */
/*                               */

void test(void);
void test(void)
{
  lmm_system_t sys = NULL ;
  lmm_constraint_t l1 = NULL;
  lmm_constraint_t l2 = NULL;
  lmm_constraint_t l3 = NULL;

  lmm_variable_t r_1_2_3 = NULL;
  lmm_variable_t r_1 = NULL;
  lmm_variable_t r_2 = NULL;
  lmm_variable_t r_3 = NULL;



  sys = lmm_system_new();
  l1 = lmm_constraint_new(sys, /* (void *) "L1", */ 1.0);
  l2 = lmm_constraint_new(sys, /* (void *) "L2", */ 10.0);
  l3 = lmm_constraint_new(sys, /* (void *) "L3", */ 1.0);

  r_1_2_3 = lmm_variable_new(sys, /* (void *) "R 1->2->3", */ 1.0 , 1.0 , 3);
  r_1 = lmm_variable_new(sys, /* (void *) "R 1", */ 1.0 , 1.0 , 1);
  r_2 = lmm_variable_new(sys, /* (void *) "R 2", */ 1.0 , 1.0 , 1);
  r_3 = lmm_variable_new(sys, /* (void *) "R 3", */ 1.0 , 1.0 , 1);

  lmm_add_constraint(sys, l1, r_1_2_3, 1.0);
  lmm_add_constraint(sys, l2, r_1_2_3, 1.0);
  lmm_add_constraint(sys, l3, r_1_2_3, 1.0);

  lmm_add_constraint(sys, l1, r_1, 1.0);

  lmm_add_constraint(sys, l2, r_2, 1.0);

  lmm_add_constraint(sys, l3, r_3, 1.0);

  lmm_system_free(sys);
} 


int main(int argc, char **argv)
{
  test();
  return 0;
}
