/* A crash few tests for the maxmin library                                 */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/maxmin.h"
#include "xbt/module.h"
#include "xbt/xbt_os_time.h"
#include "xbt/sysdep.h"         /* time manipulation for benchmarking */

#define MYRANDMAX 1000

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

double date;
int64_t seedx= 0;

static int myrand(void) {
  seedx=seedx * 16807 % 2147483647;
  return (int32_t) seedx%1000;
}

static double float_random(double max)
{
  return ((max * myrand()) / (MYRANDMAX + 1.0));
}

static int int_random(int max)
{
  return (int32_t) (((max * 1.0) * myrand()) / (MYRANDMAX + 1.0));
}

static void test(int nb_cnst, int nb_var, int nb_elem, int pw_base_limit, int pw_max_limit, float rate_no_limit,
                 int max_share, int mode)
{
  lmm_system_t Sys = NULL;
  lmm_constraint_t *cnst = xbt_new0(lmm_constraint_t, nb_cnst);
  lmm_variable_t *var = xbt_new0(lmm_variable_t, nb_var);
  int *used = xbt_new0(int, nb_cnst);
  int i, j, k,l;
  int concurrency_share;

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
    concurrency_share=1+int_random(max_share);
    lmm_variable_concurrency_share_set(var[i],concurrency_share);

    for (j = 0; j < nb_cnst; j++)
      used[j] = 0;
    for (j = 0; j < nb_elem; j++) {
      k = int_random(nb_cnst);
      if (used[k]>=concurrency_share) {
        j--;
        continue;
      }
      lmm_expand(Sys, cnst[k], var[i], float_random(1.5));
      lmm_expand_add(Sys, cnst[k], var[i], float_random(1.5));
      used[k]++;
    }
  }

  printf("Starting to solve(%i)\n",myrand()%1000);
  date = xbt_os_time() * 1000000;
  lmm_solve(Sys);
  date = xbt_os_time() * 1000000 - date;

  if(mode==2){
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
  }

  for (i = 0; i < nb_var; i++)
    lmm_variable_free(Sys, var[i]);
  lmm_system_free(Sys);
  free(cnst);
  free(var);
  free(used);
}

int TestClasses [][4]=
  //Nbcnst Nbvar Baselimit Maxlimit 
  {{  10  ,10    ,1        ,2 }, //small
   {  100 ,100   ,3        ,6 }, //medium
   {  2000,2000  ,5        ,8 }, //big
   { 20000,20000 ,7        ,10}  //huge
  }; 

int main(int argc, char **argv)
{
  int nb_cnst, nb_var,nb_elem,pw_base_limit,pw_max_limit,max_share;
  float rate_no_limit=0.2;
  float acc_date=0,acc_date2=0;
  int testclass,mode,testcount;
  int i;

  if(argc<3) {
    printf("Syntax: <small|medium|big|huge> <count> [test|debug|perf]\n");
    return -1;
  }

  //what class?
  if(!strcmp(argv[1],"small"))
      testclass=0;
  else if(!strcmp(argv[1],"medium"))
      testclass=1;
  else if(!strcmp(argv[1],"big"))
      testclass=2;
  else if(!strcmp(argv[1],"huge"))
      testclass=3;
  else {
    printf("Unknown class \"%s\", aborting!\n",argv[1]);
    return -2;
  }

  //How many times?
  testcount=atoi(argv[2]);

  //Show me everything (debug or performance)!
  mode=0;
  if(argc>=4 && strcmp(argv[3],"test")==0)
    mode=1;
  if(argc>=4 && strcmp(argv[3],"debug")==0)
    mode=2;
  if(argc>=4 && strcmp(argv[3],"perf")==0)
    mode=3;

  if(mode==1)
    xbt_log_control_set("surf/maxmin.threshold:DEBUG surf/maxmin.fmt:\'[%r]: [%c/%p] %m%n\'\
                         surf.threshold:DEBUG surf.fmt:\'[%r]: [%c/%p] %m%n\' ");

  if(mode==2)
    xbt_log_control_set("surf/maxmin.threshold:DEBUG surf.threshold:DEBUG");

  nb_cnst= TestClasses[testclass][0];
  nb_var= TestClasses[testclass][1];
  pw_base_limit= TestClasses[testclass][2];
  pw_max_limit= TestClasses[testclass][3];
  max_share=2; //1<<(pw_base_limit/2+1);

  //If you want to test concurrency, you need nb_elem >> 2^pw_base_limit:
  nb_elem= (1<<pw_base_limit)+(1<<(8*pw_max_limit/10));
  //Otherwise, just set it to a constant value (and set rate_no_limit to 1.0):
  //nb_elem=200

  xbt_init(&argc, argv);

  for(i=0;i<testcount;i++){
    seedx=i+1;
    printf("Starting %i: (%i)\n",i,myrand()%1000);
    test(nb_cnst, nb_var, nb_elem, pw_base_limit, pw_max_limit, rate_no_limit,max_share,mode);
    acc_date+=date;
    acc_date2+=date*date;
  }

  float mean_date= acc_date/(float)testcount;  
  float stdev_date= sqrt(acc_date2/(float)testcount-mean_date*mean_date);

  printf("%ix One shot execution time for a total of %d constraints, "
         "%d variables with %d active constraint each, concurrency in [%i,%i] and max concurrency share %i\n",
         testcount,nb_cnst, nb_var, nb_elem, (1<<pw_base_limit), (1<<pw_base_limit)+(1<<pw_max_limit), max_share);
  if(mode==3)
    printf("Execution time: %g +- %g  microseconds \n",mean_date, stdev_date);
  return 0;
}
