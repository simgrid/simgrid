#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdag/simdag.h"
#include "xbt/log.h"

#include "xbt/sysdep.h"         /* calloc, printf */

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test,
                             "Logging specific to this SimDag example");

static int nameCompareHosts(const void *n1, const void *n2)
{
  return strcmp(SD_workstation_get_name(*((SD_workstation_t *) n1)),
                SD_workstation_get_name(*((SD_workstation_t *) n2)));
}

int main(int argc, char **argv)
{
  int i, j;
  SD_task_t *changed_tasks;
  int n_hosts;
  const SD_workstation_t *hosts;
  SD_task_t taskInit;
  SD_task_t PtoPComm1;
  SD_task_t PtoPComm2;
  SD_task_t ParComp_wocomm;
  SD_task_t IntraRedist;
  SD_task_t ParComp_wcomm1;
  SD_task_t InterRedist;
  SD_task_t taskFinal;
  SD_task_t ParComp_wcomm2;
  SD_workstation_t PtoPcomm1_hosts[2];
  SD_workstation_t PtoPcomm2_hosts[2];
  double PtoPcomm1_table[] = { 0, 12500000, 0, 0 };     /* 100Mb */
  double PtoPcomm2_table[] = { 0, 1250000, 0, 0 };      /* 10Mb */
  double ParComp_wocomm_cost[] = { 1e+9, 1e+9, 1e+9, 1e+9, 1e+9 };      /* 1 Gflop per Proc */
  double *ParComp_wocomm_table;
  SD_workstation_t ParComp_wocomm_hosts[5];
  double *IntraRedist_cost;
  double *IntraRedist_table;
  SD_workstation_t IntraRedist_hosts[5];
  double ParComp_wcomm1_cost[] = { 1e+9, 1e+9, 1e+9, 1e+9, 1e+9 };      /* 1 Gflop per Proc */
  double *ParComp_wcomm1_table;
  SD_workstation_t ParComp_wcomm1_hosts[5];
  double *InterRedist_cost;
  double *InterRedist_table;
  double ParComp_wcomm2_cost[] = { 1e+8, 1e+8, 1e+8, 1e+8, 1e+8 };      /* 1 Gflop per Proc (0.02sec duration) */
  SD_workstation_t ParComp_wcomm2_hosts[5];
  double final_cost = 5e+9;
  double *ParComp_wcomm2_table;

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* getting platform infos */
  n_hosts = SD_workstation_get_number();
  hosts = SD_workstation_get_list();

  /* sorting hosts by hostname */
  qsort((void *) hosts, n_hosts, sizeof(SD_workstation_t), nameCompareHosts);

  /* creation of the tasks */
  taskInit = SD_task_create("Initial", NULL, 1.0);
  PtoPComm1 = SD_task_create("PtoP Comm 1", NULL, 1.0);
  PtoPComm2 = SD_task_create("PtoP Comm 2", NULL, 1.0);
  ParComp_wocomm = SD_task_create("Par Comp without comm", NULL, 1.0);
  IntraRedist = SD_task_create("intra redist", NULL, 1.0);
  ParComp_wcomm1 = SD_task_create("Par Comp with comm 1", NULL, 1.0);
  InterRedist = SD_task_create("inter redist", NULL, 1.0);
  taskFinal = SD_task_create("Final", NULL, 1.0);
  ParComp_wcomm2 = SD_task_create("Par Comp with comm 2", NULL, 1.0);


  /* creation of the dependencies */
  SD_task_dependency_add(NULL, NULL, taskInit, PtoPComm1);
  SD_task_dependency_add(NULL, NULL, taskInit, PtoPComm2);
  SD_task_dependency_add(NULL, NULL, PtoPComm1, ParComp_wocomm);
  SD_task_dependency_add(NULL, NULL, ParComp_wocomm, IntraRedist);
  SD_task_dependency_add(NULL, NULL, IntraRedist, ParComp_wcomm1);
  SD_task_dependency_add(NULL, NULL, ParComp_wcomm1, InterRedist);
  SD_task_dependency_add(NULL, NULL, InterRedist, ParComp_wcomm2);
  SD_task_dependency_add(NULL, NULL, ParComp_wcomm2, taskFinal);
  SD_task_dependency_add(NULL, NULL, PtoPComm2, taskFinal);


  /* scheduling parameters */

  /* large point-to-point communication (0.1 sec duration) */
  PtoPcomm1_hosts[0] = hosts[0];
  PtoPcomm1_hosts[1] = hosts[1];

  /* small point-to-point communication (0.01 sec duration) */
  PtoPcomm2_hosts[0] = hosts[0];
  PtoPcomm2_hosts[1] = hosts[2];

  /* parallel task without intra communications (1 sec duration) */
  ParComp_wocomm_table = xbt_new0(double, 25);

  for (i = 0; i < 5; i++) {
    ParComp_wocomm_hosts[i] = hosts[i];
  }

  /* redistribution within a cluster (small latencies) */
  /* each host send (4*2.5Mb =) 10Mb */
  /* bandwidth is shared between 5 flows (0.05sec duration) */
  IntraRedist_cost = xbt_new0(double, 5);
  IntraRedist_table = xbt_new0(double, 25);
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (i == j)
        IntraRedist_table[i * 5 + j] = 0.;
      else
        IntraRedist_table[i * 5 + j] = 312500.; /* 2.5Mb */
    }
  }

  for (i = 0; i < 5; i++) {
    IntraRedist_hosts[i] = hosts[i];
  }

  /* parallel task with intra communications */
  /* Computation domination (1 sec duration) */
  ParComp_wcomm1_table = xbt_new0(double, 25);

  for (i = 0; i < 5; i++) {
    ParComp_wcomm1_hosts[i] = hosts[i];
  }

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (i == j)
        ParComp_wcomm1_table[i * 5 + j] = 0.;
      else
        ParComp_wcomm1_table[i * 5 + j] = 312500.;      /* 2.5Mb */
    }
  }

  /* inter cluster redistribution (big latency on the backbone) */
  /* (0.5sec duration without latency impact) */
  InterRedist_cost = xbt_new0(double, 10);
  InterRedist_table = xbt_new0(double, 100);
  for (i = 0; i < 5; i++) {
    InterRedist_table[i * 10 + i + 5] = 1250000.;       /* 10Mb */
  }

  /* parallel task with intra communications */
  /* Communication domination (0.1 sec duration) */

  ParComp_wcomm2_table = xbt_new0(double, 25);

  for (i = 0; i < 5; i++) {
    ParComp_wcomm2_hosts[i] = hosts[i + 5];
  }

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (i == j)
        ParComp_wcomm2_table[i * 5 + j] = 0.;
      else
        ParComp_wcomm2_table[i * 5 + j] = 625000.;      /* 5Mb */
    }
  }

  /* Sequential task */


  /* scheduling the tasks */
  SD_task_schedule(taskInit, 1, hosts, SD_SCHED_NO_COST, SD_SCHED_NO_COST, -1.0);
  SD_task_schedule(PtoPComm1, 2, PtoPcomm1_hosts, SD_SCHED_NO_COST, PtoPcomm1_table,
                   -1.0);
  SD_task_schedule(PtoPComm2, 2, PtoPcomm2_hosts, SD_SCHED_NO_COST, PtoPcomm2_table,
                   -1.0);
  SD_task_schedule(ParComp_wocomm, 5, ParComp_wocomm_hosts,
                   ParComp_wocomm_cost, ParComp_wocomm_table, -1.0);
  SD_task_schedule(IntraRedist, 5, IntraRedist_hosts, IntraRedist_cost,
                   IntraRedist_table, -1.0);
  SD_task_schedule(ParComp_wcomm1, 5, ParComp_wcomm1_hosts,
                   ParComp_wcomm1_cost, ParComp_wcomm1_table, -1.0);
  SD_task_schedule(InterRedist, 10, hosts, InterRedist_cost,
                   InterRedist_table, -1.0);
  SD_task_schedule(ParComp_wcomm2, 5, ParComp_wcomm2_hosts,
                   ParComp_wcomm2_cost, ParComp_wcomm2_table, -1.0);
  SD_task_schedule(taskFinal, 1, &(hosts[9]), &final_cost, SD_SCHED_NO_COST, -1.0);

  /* let's launch the simulation! */
  changed_tasks = SD_simulate(-1.0);

  free(changed_tasks);

  free(ParComp_wocomm_table);
  free(IntraRedist_cost);
  free(IntraRedist_table);
  free(ParComp_wcomm1_table);
  free(InterRedist_cost);
  free(InterRedist_table);
  free(ParComp_wcomm2_table);

  SD_task_destroy(taskInit);
  SD_task_destroy(PtoPComm1);
  SD_task_destroy(PtoPComm2);
  SD_task_destroy(ParComp_wocomm);
  SD_task_destroy(IntraRedist);
  SD_task_destroy(ParComp_wcomm1);
  SD_task_destroy(InterRedist);
  SD_task_destroy(ParComp_wcomm2);
  SD_task_destroy(taskFinal);

  SD_exit();
  return 0;
}
