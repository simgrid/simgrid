/* Copyright (c) 2009-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "src/simdag/simdag_private.h"
#include "simgrid/simdag.h"
#include "xbt/file.h"
#include <string.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_dotparse, sd, "Parsing DOT files");

#if HAVE_GRAPHVIZ
#include <graphviz/cgraph.h>
#endif

typedef enum {
  sequential =0,
  parallel
} seq_par_t;

xbt_dynar_t SD_dotload_generic(const char * filename, seq_par_t seq_or_par, bool schedule);

static void dot_task_p_free(void *task) {
  SD_task_destroy(*(SD_task_t *)task);
}

/** @brief loads a DOT file describing a DAG
 * 
 * See http://www.graphviz.org/doc/info/lang.html  for more details.
 * The size attribute of a node describes:
 *   - for a compute task: the amount of flops to execute
 *   - for a communication task : the amount of bytes to transfer
 * If this attribute is ommited, the default value is zero.
 */
xbt_dynar_t SD_dotload(const char *filename) {
  return SD_dotload_generic(filename, sequential, false);
}

xbt_dynar_t SD_PTG_dotload(const char * filename) {
  return SD_dotload_generic(filename, parallel, false);
}

xbt_dynar_t SD_dotload_with_sched(const char *filename) {
  return SD_dotload_generic(filename, sequential, true);
}

static int edge_compare(const void *a, const void *b)
{
  unsigned va = AGSEQ(*(Agedge_t **)a);
  unsigned vb = AGSEQ(*(Agedge_t **)b);
  return va == vb ? 0 : (va < vb ? -1 : 1);
}

xbt_dynar_t SD_dotload_generic(const char * filename, seq_par_t seq_or_par, bool schedule){
  xbt_assert(filename, "Unable to use a null file descriptor\n");
  FILE *in_file = fopen(filename, "r");
  xbt_assert(in_file != NULL, "Failed to open file: %s", filename);

  unsigned int i;
  SD_task_t root, end, task;
  xbt_dict_t computers;
  xbt_dynar_t computer = NULL;
  xbt_dict_cursor_t dict_cursor;
  bool schedule_success = true;

  xbt_dict_t jobs = xbt_dict_new_homogeneous(NULL);
  xbt_dynar_t result = xbt_dynar_new(sizeof(SD_task_t), dot_task_p_free);

  Agraph_t * dag_dot = agread(in_file, NIL(Agdisc_t *));

  if (schedule)
    computers = xbt_dict_new_homogeneous(NULL);

  /* Create all the nodes */
  Agnode_t *node = NULL;
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    char *name = agnameof(node);
    double amount = atof(agget(node, (char *) "size"));

    if (!(task = (SD_task_t)xbt_dict_get_or_null(jobs, name))) {
      if (seq_or_par == sequential){
        XBT_DEBUG("See <job id=%s amount =%.0f>", name, amount);
        task = SD_task_create_comp_seq(name, NULL , amount);
      } else {
        double alpha = atof(agget(node, (char *) "alpha"));
        XBT_DEBUG("See <job id=%s amount =%.0f alpha = %.3f>", name, amount, alpha);
        task = SD_task_create_comp_par_amdahl(name, NULL , amount, alpha);
      }

      xbt_dict_set(jobs, name, task, NULL);

      if (strcmp(name,"root") && strcmp(name,"end"))
        xbt_dynar_push(result, &task);

      if((seq_or_par == sequential) &&
        ((schedule && schedule_success) || XBT_LOG_ISENABLED(sd_dotparse, xbt_log_priority_verbose))){
        /* try to take the information to schedule the task only if all is right*/
        char *char_performer = agget(node, (char *) "performer");
        char *char_order = agget(node, (char *) "order");
        /* Tasks will execute on in a given "order" on a given set of "performer" hosts */
        int performer = ((!char_performer || !strcmp(char_performer,"")) ? -1:atoi(char_performer));
        int order = ((!char_order || !strcmp(char_order, ""))? -1:atoi(char_order));

        if((performer != -1 && order != -1) && performer < (int) sg_host_count()){
          /* required parameters are given and less performers than hosts are required */
          XBT_DEBUG ("Task '%s' is scheduled on workstation '%d' in position '%d'", task->name, performer, order);
          if(!(computer = (xbt_dynar_t) xbt_dict_get_or_null(computers, char_performer))){
            computer = xbt_dynar_new(sizeof(SD_task_t), NULL);
            xbt_dict_set(computers, char_performer, computer, NULL);
          }

          if((unsigned int)order < xbt_dynar_length(computer)){
            SD_task_t *task_test = (SD_task_t *)xbt_dynar_get_ptr(computer,order);
            if(*task_test && *task_test != task){
              /* the user gave the same order to several tasks */
              schedule_success = false;
              XBT_VERB("Task '%s' wants to start on performer '%s' at the same position '%s' as task '%s'",
                       (*task_test)->name, char_performer, char_order, task->name);
              continue;
            }
          }
          /* the parameter seems to be ok */
          xbt_dynar_set_as(computer, order, SD_task_t, task);
        } else {
          /* one of required parameters is not given */
          schedule_success = false;
          XBT_VERB("The schedule is ignored, task '%s' can not be scheduled on %d hosts", task->name, performer);
        }
      }
    } else {
      XBT_WARN("Task '%s' is defined more than once", name);
    }
  }

  /*Check if 'root' and 'end' nodes have been explicitly declared.  If not, create them. */
  if (!(root = (SD_task_t)xbt_dict_get_or_null(jobs, "root")))
    root = (seq_or_par == sequential?SD_task_create_comp_seq("root", NULL, 0):
                                     SD_task_create_comp_par_amdahl("root", NULL, 0, 0));

  SD_task_set_state(root, SD_SCHEDULABLE);   /* by design the root task is always SCHEDULABLE */
  xbt_dynar_insert_at(result, 0, &root);     /* Put it at the beginning of the dynar */

  if (!(end = (SD_task_t)xbt_dict_get_or_null(jobs, "end")))
    end = (seq_or_par == sequential?SD_task_create_comp_seq("end", NULL, 0):
                                    SD_task_create_comp_par_amdahl("end", NULL, 0, 0));

  /* Create edges */
  xbt_dynar_t edges = xbt_dynar_new(sizeof(Agedge_t*), NULL);
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    Agedge_t * edge;
    xbt_dynar_reset(edges);
    for (edge = agfstout(dag_dot, node); edge; edge = agnxtout(dag_dot, edge))
      xbt_dynar_push_as(edges, Agedge_t *, edge);

    /* Be sure edges are sorted */
    xbt_dynar_sort(edges, edge_compare);

    xbt_dynar_foreach(edges, i, edge) {
      SD_task_t src, dst;
      char *src_name=agnameof(agtail(edge)), *dst_name=agnameof(aghead(edge));
      double size = atof(agget(edge, (char *) "size"));

      src = (SD_task_t)xbt_dict_get_or_null(jobs, src_name);
      dst = (SD_task_t)xbt_dict_get_or_null(jobs, dst_name);

      if (size > 0) {
        char *name = (char*)xbt_malloc((strlen(src_name)+strlen(dst_name)+6)*sizeof(char));
        sprintf(name, "%s->%s", src_name, dst_name);
        XBT_DEBUG("See <transfer id=%s amount = %.0f>", name, size);
        if (!(task = (SD_task_t)xbt_dict_get_or_null(jobs, name))) {
          if (seq_or_par == sequential)
            task = SD_task_create_comm_e2e(name, NULL , size);
          else
            task = SD_task_create_comm_par_mxn_1d_block(name, NULL , size);
          SD_task_dependency_add(NULL, NULL, src, task);
          SD_task_dependency_add(NULL, NULL, task, dst);
          xbt_dict_set(jobs, name, task, NULL);
          xbt_dynar_push(result, &task);
        } else {
          XBT_WARN("Task '%s' is defined more than once", name);
        }
        xbt_free(name);
      } else {
        SD_task_dependency_add(NULL, NULL, src, dst);
      }
    }
  }
  xbt_dynar_free(&edges);

  XBT_DEBUG("All tasks have been created, put %s at the end of the dynar", end->name);
  xbt_dynar_push(result, &end);

  /* Connect entry tasks to 'root', and exit tasks to 'end'*/
  xbt_dynar_foreach (result, i, task){
    if (xbt_dynar_is_empty(task->tasks_before) && task != root) {
      XBT_DEBUG("Task '%s' has no source. Add dependency from 'root'", task->name);
      SD_task_dependency_add(NULL, NULL, root, task);
    }

    if (xbt_dynar_is_empty(task->tasks_after) && task != end) {
      XBT_DEBUG("Task '%s' has no destination. Add dependency to 'end'", task->name);
      SD_task_dependency_add(NULL, NULL, task, end);
    }
  }

  agclose(dag_dot);
  xbt_dict_free(&jobs);
  fclose(in_file);

  if(schedule){
    char *computer_name;
    if (schedule_success) {
      const sg_host_t *workstations = sg_host_list ();
      xbt_dict_foreach(computers,dict_cursor,computer_name,computer){
        SD_task_t previous_task = NULL;
        xbt_dynar_foreach(computer, i, task){
          /* add dependency between the previous and the task to avoid parallel execution */
          if(task){
            if(previous_task && !SD_task_dependency_exists(previous_task, task))
              SD_task_dependency_add(NULL, NULL, previous_task, task);

            SD_task_schedulel(task, 1, workstations[atoi(computer_name)]);
            previous_task = task;
          }
        }
        xbt_dynar_free(&computer);
      }
      xbt_dict_free(&computers);
    } else {
      XBT_WARN("The scheduling is ignored");
      xbt_dict_foreach(computers,dict_cursor,computer_name,computer)
        xbt_dynar_free(&computer);
      xbt_dict_free(&computers);
      xbt_dynar_free(&result);
      result = NULL;
    }
  }

  if (result && !acyclic_graph_detail(result)) {
    char* base = xbt_basename(filename);
    XBT_ERROR("The DOT described in %s is not a DAG. It contains a cycle.", base);
    free(base);
    xbt_dynar_free(&result);
    result = NULL;
  }
  return result;
}
