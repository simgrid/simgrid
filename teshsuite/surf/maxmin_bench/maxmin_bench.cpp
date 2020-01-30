/* A few crash tests for the maxmin library                                 */

/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "xbt/module.h"
#include "xbt/sysdep.h" /* time manipulation for benchmarking */
#include "xbt/xbt_os_time.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

double date;
int64_t seedx = 0;

static int myrand() {
  seedx=seedx * 16807 % 2147483647;
  return static_cast<int32_t>(seedx%1000);
}

static double float_random(double max)
{
  constexpr double MYRANDMAX = 1000.0;
  return ((max * myrand()) / (MYRANDMAX + 1.0));
}

static unsigned int int_random(int max)
{
  return static_cast<uint32_t>(float_random(max));
}

static void test(int nb_cnst, int nb_var, int nb_elem, unsigned int pw_base_limit, unsigned int pw_max_limit,
                 float rate_no_limit, int max_share, int mode)
{
  simgrid::kernel::lmm::Constraint** cnst = new simgrid::kernel::lmm::Constraint*[nb_cnst];
  simgrid::kernel::lmm::Variable** var = new simgrid::kernel::lmm::Variable*[nb_var];
  int* used = new int[nb_cnst];

  /* We cannot activate the selective update as we pass nullptr as an Action when creating the variables */
  simgrid::kernel::lmm::System* Sys = new simgrid::kernel::lmm::System(false);

  for (int i = 0; i < nb_cnst; i++) {
    cnst[i] = Sys->constraint_new(NULL, float_random(10.0));
    int l;
    if(rate_no_limit>float_random(1.0))
      //Look at what happens when there is no concurrency limit
      l=-1;
    else
      //Badly logarithmically random concurrency limit in [2^pw_base_limit+1,2^pw_base_limit+2^pw_max_limit]
      l=(1<<pw_base_limit)+(1<<int_random(pw_max_limit));

    cnst[i]->set_concurrency_limit(l);
  }

  for (int i = 0; i < nb_var; i++) {
    var[i] = Sys->variable_new(NULL, 1.0, -1.0, nb_elem);
    //Have a few variables with a concurrency share of two (e.g. cross-traffic in some cases)
    int concurrency_share = 1 + int_random(max_share);
    var[i]->set_concurrency_share(concurrency_share);

    for (int j = 0; j < nb_cnst; j++)
      used[j] = 0;
    for (int j = 0; j < nb_elem; j++) {
      int k = int_random(nb_cnst);
      if (used[k]>=concurrency_share) {
        j--;
        continue;
      }
      Sys->expand(cnst[k], var[i], float_random(1.5));
      Sys->expand_add(cnst[k], var[i], float_random(1.5));
      used[k]++;
    }
  }

  fprintf(stderr,"Starting to solve(%i)\n",myrand()%1000);
  date = xbt_os_time() * 1000000;
  Sys->solve();
  date = xbt_os_time() * 1000000 - date;

  if(mode==2){
    fprintf(stderr,"Max concurrency:\n");
    int l=0;
    for (int i = 0; i < nb_cnst; i++) {
      int j = cnst[i]->get_concurrency_maximum();
      int k = cnst[i]->get_concurrency_limit();
      xbt_assert(k<0 || j<=k);
      if(j>l)
        l=j;
      fprintf(stderr,"(%i):%i/%i ",i,j,k);
      cnst[i]->reset_concurrency_maximum();
      xbt_assert(not cnst[i]->get_concurrency_maximum());
      if(i%10==9)
        fprintf(stderr,"\n");
    }
    fprintf(stderr,"\nTotal maximum concurrency is %i\n",l);

    Sys->print();
  }

  for (int i = 0; i < nb_var; i++)
    Sys->variable_free(var[i]);
  delete Sys;
  delete[] cnst;
  delete[] var;
  delete[] used;
}

unsigned int TestClasses [][4]=
  //Nbcnst Nbvar Baselimit Maxlimit
  {{  10  ,10    ,1        ,2 }, //small
   {  100 ,100   ,3        ,6 }, //medium
   {  2000,2000  ,5        ,8 }, //big
   { 20000,20000 ,7        ,10}  //huge
  };

int main(int argc, char **argv)
{
  simgrid::s4u::Engine e(&argc, argv);

  float rate_no_limit=0.2;
  float acc_date=0;
  float acc_date2=0;
  int testclass;

  if(argc<3) {
    fprintf(stderr, "Syntax: <small|medium|big|huge> <count> [test|debug|perf]\n");
    return -1;
  }

  //what class?
  if (not strcmp(argv[1], "small"))
    testclass = 0;
  else if (not strcmp(argv[1], "medium"))
    testclass = 1;
  else if (not strcmp(argv[1], "big"))
    testclass = 2;
  else if (not strcmp(argv[1], "huge"))
    testclass = 3;
  else {
    fprintf(stderr, "Unknown class \"%s\", aborting!\n",argv[1]);
    return -2;
  }

  //How many times?
  int testcount=atoi(argv[2]);

  //Show me everything (debug or performance)!
  int mode=0;
  if(argc>=4 && strcmp(argv[3],"test")==0)
    mode=1;
  if(argc>=4 && strcmp(argv[3],"debug")==0)
    mode=2;
  if(argc>=4 && strcmp(argv[3],"perf")==0)
    mode=3;

  if(mode==1)
    xbt_log_control_set("surf/maxmin.threshold:DEBUG surf/maxmin.fmt:\'[%r]: [%c/%p] %m%n\' "
                        "surf.threshold:DEBUG surf.fmt:\'[%r]: [%c/%p] %m%n\' ");

  if(mode==2)
    xbt_log_control_set("surf/maxmin.threshold:DEBUG surf.threshold:DEBUG");

  unsigned int nb_cnst= TestClasses[testclass][0];
  unsigned int nb_var= TestClasses[testclass][1];
  unsigned int pw_base_limit= TestClasses[testclass][2];
  unsigned int pw_max_limit= TestClasses[testclass][3];
  unsigned int max_share    = 2; // 1<<(pw_base_limit/2+1)

  //If you want to test concurrency, you need nb_elem >> 2^pw_base_limit:
  unsigned int nb_elem= (1<<pw_base_limit)+(1<<(8*pw_max_limit/10));
  //Otherwise, just set it to a constant value (and set rate_no_limit to 1.0):
  //nb_elem=200

  for(int i=0;i<testcount;i++){
    seedx=i+1;
    fprintf(stderr, "Starting %i: (%i)\n",i,myrand()%1000);
    test(nb_cnst, nb_var, nb_elem, pw_base_limit, pw_max_limit, rate_no_limit,max_share,mode);
    acc_date+=date;
    acc_date2+=date*date;
  }

  float mean_date= acc_date/(float)testcount;
  float stdev_date= sqrt(acc_date2/(float)testcount-mean_date*mean_date);

  fprintf(stderr, "%ix One shot execution time for a total of %u constraints, "
                  "%u variables with %u active constraint each, concurrency in [%i,%i] and max concurrency share %u\n",
          testcount, nb_cnst, nb_var, nb_elem, (1 << pw_base_limit), (1 << pw_base_limit) + (1 << pw_max_limit),
          max_share);
  if(mode==3)
    fprintf(stderr, "Execution time: %g +- %g  microseconds \n",mean_date, stdev_date);

  return 0;
}
