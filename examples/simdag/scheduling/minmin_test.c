/* simple test to schedule a DAX file with the Min-Min algorithm.           */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
                             "Logging specific to this SimDag example");

typedef struct _WorkstationAttribute *WorkstationAttribute;
struct _WorkstationAttribute {
  /* Earliest time at wich a workstation is ready to execute a task */
  double available_at;
};

static void SD_workstation_allocate_attribute(SD_workstation_t workstation)
{
  void *data;
  data = calloc(1, sizeof(struct _WorkstationAttribute));
  SD_workstation_set_data(workstation, data);
}

static void SD_workstation_free_attribute(SD_workstation_t workstation)
{
  free(SD_workstation_get_data(workstation));
  SD_workstation_set_data(workstation, NULL);
}

static double SD_workstation_get_available_at(SD_workstation_t workstation)
{
  WorkstationAttribute attr =
      (WorkstationAttribute) SD_workstation_get_data(workstation);
  return attr->available_at;
}

static void SD_workstation_set_available_at(SD_workstation_t workstation,
                                            double time)
{
  WorkstationAttribute attr =
      (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->available_at = time;
  SD_workstation_set_data(workstation, attr);
}


static xbt_dynar_t get_ready_tasks(xbt_dynar_t dax)
{
  unsigned int i;
  xbt_dynar_t ready_tasks;
  SD_task_t task;

  ready_tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);
  xbt_dynar_foreach(dax, i, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ &&
        SD_task_get_state(task) == SD_SCHEDULABLE) {
      xbt_dynar_push(ready_tasks, &task);
    }
  }
  DEBUG1("There are %lu ready tasks", xbt_dynar_length(ready_tasks));

  return ready_tasks;
}

static double finish_on_at(SD_task_t task, SD_workstation_t workstation)
{
  volatile double result;
  unsigned int i;
  double data_available = 0.;
  double redist_time = 0;
  double last_data_available;
  SD_task_t parent, grand_parent;
  xbt_dynar_t parents, grand_parents;

  int grand_parent_nworkstations;
  SD_workstation_t *grand_parent_workstation_list;

  parents = SD_task_get_parents(task);

  if (xbt_dynar_length(parents)) {
    /* compute last_data_available */
    last_data_available = -1.0;
    xbt_dynar_foreach(parents, i, parent) {

      /* normal case */
      if (SD_task_get_kind(parent) == SD_TASK_COMM_E2E) {
        grand_parents = SD_task_get_parents(parent);

        if (xbt_dynar_length(grand_parents) > 1) {
          ERROR1("Warning: transfer %s has 2 parents",
                 SD_task_get_name(parent));
        }
        xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);

        grand_parent_nworkstations =
            SD_task_get_workstation_count(grand_parent);
        grand_parent_workstation_list =
            SD_task_get_workstation_list(grand_parent);
        /* Estimate the redistribution time from this parent */
        redist_time =
            SD_route_get_communication_time(grand_parent_workstation_list
                                            [0], workstation,
                                            SD_task_get_amount(parent));
        data_available =
            SD_task_get_finish_time(grand_parent) + redist_time;

        xbt_dynar_free_container(&grand_parents);
      }

      /* no transfer, control dependency */
      if (SD_task_get_kind(parent) == SD_TASK_COMP_SEQ) {
        data_available = SD_task_get_finish_time(parent);
      }

      if (last_data_available < data_available)
        last_data_available = data_available;

    }

    xbt_dynar_free_container(&parents);

    result = MAX(SD_workstation_get_available_at(workstation),
               last_data_available) +
        SD_workstation_get_computation_time(workstation,
                                            SD_task_get_amount(task));
  } else {
    xbt_dynar_free_container(&parents);

    result = SD_workstation_get_available_at(workstation) +
        SD_workstation_get_computation_time(workstation,
                                            SD_task_get_amount(task));
  }
  return result;
}

static SD_workstation_t SD_task_get_best_workstation(SD_task_t task)
{
  int i;
  double EFT, min_EFT = -1.0;
  const SD_workstation_t *workstations = SD_workstation_get_list();
  int nworkstations = SD_workstation_get_number();
  SD_workstation_t best_workstation;

  best_workstation = workstations[0];
  min_EFT = finish_on_at(task, workstations[0]);

  for (i = 1; i < nworkstations; i++) {
    EFT = finish_on_at(task, workstations[i]);
    DEBUG3("%s finishes on %s at %f",
           SD_task_get_name(task),
           SD_workstation_get_name(workstations[i]), EFT);

    if (EFT < min_EFT) {
      min_EFT = EFT;
      best_workstation = workstations[i];
    }
  }

  return best_workstation;
}

static void output_xml(FILE * out, xbt_dynar_t dax)
{
  unsigned int i, j, k;
  int current_nworkstations;
  const int nworkstations = SD_workstation_get_number();
  const SD_workstation_t *workstations = SD_workstation_get_list();
  SD_task_t task;
  SD_workstation_t *list;

  fprintf(out, "<?xml version=\"1.0\"?>\n");
  fprintf(out, "<grid_schedule>\n");
  fprintf(out, "   <grid_info>\n");
  fprintf(out, "      <info name=\"nb_clusters\" value=\"1\"/>\n");
  fprintf(out, "         <clusters>\n");
  fprintf(out,
          "            <cluster id=\"1\" hosts=\"%d\" first_host=\"0\"/>\n",
          nworkstations);
  fprintf(out, "         </clusters>\n");
  fprintf(out, "      </grid_info>\n");
  fprintf(out, "   <node_infos>\n");

  xbt_dynar_foreach(dax, i, task) {
    fprintf(out, "      <node_statistics>\n");
    fprintf(out, "         <node_property name=\"id\" value=\"%s\"/>\n",
            SD_task_get_name(task));
    fprintf(out, "         <node_property name=\"type\" value=\"");
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ)
      fprintf(out, "computation\"/>\n");
    if (SD_task_get_kind(task) == SD_TASK_COMM_E2E)
      fprintf(out, "transfer\"/>\n");

    fprintf(out,
            "         <node_property name=\"start_time\" value=\"%.3f\"/>\n",
            SD_task_get_start_time(task));
    fprintf(out,
            "         <node_property name=\"end_time\" value=\"%.3f\"/>\n",
            SD_task_get_finish_time(task));
    fprintf(out, "         <configuration>\n");

    current_nworkstations = SD_task_get_workstation_count(task);

    fprintf(out,
            "            <conf_property name=\"host_nb\" value=\"%d\"/>\n",
            current_nworkstations);

    fprintf(out, "            <host_lists>\n");
    list = SD_task_get_workstation_list(task);
    for (j = 0; j < current_nworkstations; j++) {
      for (k = 0; k < nworkstations; k++) {
        if (!strcmp(SD_workstation_get_name(workstations[k]),
                    SD_workstation_get_name(list[j]))) {
          fprintf(out, "               <hosts start=\"%u\" nb=\"1\"/>\n",
                  k);
          fprintf(out,
                  "            <conf_property name=\"cluster_id\" value=\"0\"/>\n");
          break;
        }
      }
    }
    fprintf(out, "            </host_lists>\n");
    fprintf(out, "         </configuration>\n");
    fprintf(out, "      </node_statistics>\n");
  }
  fprintf(out, "   </node_infos>\n");
  fprintf(out, "</grid_schedule>\n");
}

int main(int argc, char **argv)
{
  unsigned int cursor, selected_idx = 0;
  double finish_time, min_finish_time = -1.0;
  SD_task_t task, selected_task = NULL;
  xbt_dynar_t ready_tasks;
  SD_workstation_t workstation, selected_workstation = NULL;
  int total_nworkstations = 0;
  const SD_workstation_t *workstations = NULL;
  xbt_dynar_t dax, changed;
  FILE *out = NULL;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  if (argc < 3) {
    INFO1("Usage: %s platform_file dax_file [jedule_file]", argv[0]);
    INFO1
        ("example: %s simulacrum_7_hosts.xml Montage_25.xml Montage_25.jed",
         argv[0]);
    exit(1);
  }
  char *tracefilename;
  if (argc == 3) {
    char *last = strrchr(argv[2], '.');

    tracefilename = bprintf("%.*s.jed",
                            (int) (last ==
                                   NULL ? strlen(argv[2]) : last -
                                   argv[2]), argv[2]);
  } else {
    tracefilename = xbt_strdup(argv[3]);
  }

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /*  Allocating the workstation attribute */
  total_nworkstations = SD_workstation_get_number();
  workstations = SD_workstation_get_list();

  for (cursor = 0; cursor < total_nworkstations; cursor++)
    SD_workstation_allocate_attribute(workstations[cursor]);


  /* load the DAX file */
  dax = SD_daxload(argv[2]);

  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_watch(task, SD_DONE);
  }

  /* Schedule the root first */
  xbt_dynar_get_cpy(dax, 0, &task);
  workstation = SD_task_get_best_workstation(task);
  SD_task_schedulel(task, 1, workstation);

  while (!xbt_dynar_is_empty((changed = SD_simulate(-1.0)))) {
    /* Get the set of ready tasks */
    ready_tasks = get_ready_tasks(dax);
    if (!xbt_dynar_length(ready_tasks)) {
      xbt_dynar_free_container(&ready_tasks);
      xbt_dynar_free_container(&changed);
      /* there is no ready task, let advance the simulation */
      continue;
    }
    /* For each ready task:
     * get the workstation that minimizes the completion time.
     * select the task that has the minimum completion time on
     * its best workstation.
     */
    xbt_dynar_foreach(ready_tasks, cursor, task) {
      DEBUG1("%s is ready", SD_task_get_name(task));
      workstation = SD_task_get_best_workstation(task);
      finish_time = finish_on_at(task, workstation);
      if (min_finish_time == -1. || finish_time < min_finish_time) {
        min_finish_time = finish_time;
        selected_task = task;
        selected_workstation = workstation;
        selected_idx = cursor;
      }
    }

    INFO2("Schedule %s on %s", SD_task_get_name(selected_task),
          SD_workstation_get_name(selected_workstation));

    SD_task_schedulel(selected_task, 1, selected_workstation);
    SD_workstation_set_available_at(selected_workstation, min_finish_time);

    xbt_dynar_free_container(&ready_tasks);
    /* reset the min_finish_time for the next set of ready tasks */
    min_finish_time = -1.;
    xbt_dynar_free_container(&changed);
  }

  INFO1("Simulation Time: %f", SD_get_clock());




  INFO0
      ("------------------- Produce the trace file---------------------------");
  INFO1("Producing the trace of the run into %s", tracefilename);
  out = fopen(tracefilename, "w");
  xbt_assert1(out, "Cannot write to %s", tracefilename);
  free(tracefilename);

  output_xml(out, dax);

  fclose(out);


  xbt_dynar_free_container(&ready_tasks);
  xbt_dynar_free_container(&changed);

  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_destroy(task);
  }

  for (cursor = 0; cursor < total_nworkstations; cursor++)
    SD_workstation_free_attribute(workstations[cursor]);

  /* exit */
  SD_exit();
  return 0;
}
