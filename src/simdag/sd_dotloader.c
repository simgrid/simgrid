/* Copyright (c) 2009-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/misc.h"
#include "xbt/log.h"
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_dotparse, sd, "Parsing DOT files");

#undef CLEANUP

#ifdef HAVE_CGRAPH_H
#include <graphviz/cgraph.h>
#elif HAVE_AGRAPH_H
#include <graphviz/agraph.h>
#define agnxtnode(dot, node)    agnxtnode(node)
#define agfstin(dot, node)      agfstin(node)
#define agnxtin(dot, edge)      agnxtin(edge)
#define agfstout(dot, node)     agfstout(node)
#define agnxtout(dot, edge)     agnxtout(edge)
#endif

typedef enum {
  sequential =0,
  parallel
} seq_par_t;

void dot_add_task(Agnode_t * dag_node);
void dot_add_parallel_task(Agnode_t * dag_node);
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t * edge,
                                seq_par_t seq_or_par);
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t * edge,
                                 seq_par_t seq_or_par);
xbt_dynar_t SD_dotload_generic(const char * filename);
xbt_dynar_t SD_dotload_generic_alt(const char * filename, seq_par_t seq_or_par);

static double dot_parse_double(const char *string) {
  if (string == NULL)
    return -1;
  double value = -1;
  char *err;

  errno = 0;
  value = strtod(string,&err);
  if(errno) {
    XBT_WARN("Failed to convert string to double: %s\n",strerror(errno));
    return -1;
  }
  return value;
}


static int dot_parse_int(const char *string) {
  if (string == NULL)
    return -10;
  int ret = 0;
  int value = -1;

  ret = sscanf(string, "%d", &value);
  if (ret != 1)
    XBT_WARN("%s is not an integer", string);
  return value;
}

static xbt_dynar_t result;
static xbt_dict_t jobs;
static xbt_dict_t files;
static xbt_dict_t computers;
static SD_task_t root_task, end_task;
static Agraph_t *dag_dot;
static bool schedule = true;

static void dot_task_free(void *task) {
  SD_task_t t = task;
  SD_task_destroy(t);
}

static void dot_task_p_free(void *task) {
  SD_task_t *t = task;
  SD_task_destroy(*t);
}

#ifdef HAVE_TRACING
static void TRACE_sd_dotloader (SD_task_t task, const char *category) {
  if (category && strlen (category)){
    if (task->category)
      XBT_DEBUG("Change the category of %s from %s to %s",
          task->name, task->category, category);
    else
      XBT_DEBUG("Set the category of %s to %s",task->name, category);
    TRACE_category (category);
    TRACE_sd_set_task_category(task, category);
  }
}
#endif

/** @brief loads a DOT file describing a DAG
 * 
 * See http://www.graphviz.org/doc/info/lang.html
 * for more details.
 * To obtain information about transfers and tasks, two attributes are
 * required : size on task (execution time in Flop) and size on edge
 * (the amount of data transfer in bit).
 * if they aren't here, there choose to be equal to zero.
 */
xbt_dynar_t SD_dotload(const char *filename) {
  computers = xbt_dict_new_homogeneous(NULL);
//  SD_dotload_generic_alt(filename, sequential);
  SD_dotload_generic(filename);
  xbt_dynar_t computer = NULL;
  xbt_dict_cursor_t dict_cursor;
  char *computer_name;
  xbt_dict_foreach(computers,dict_cursor,computer_name,computer){
    xbt_dynar_free(&computer);
  }
  xbt_dict_free(&computers);
  return result;
}

xbt_dynar_t SD_dotload_with_sched(const char *filename) {
  computers = xbt_dict_new_homogeneous(NULL);
//  SD_dotload_generic_alt(filename, sequential);
  SD_dotload_generic(filename);

  if(schedule){
    xbt_dynar_t computer = NULL;
    xbt_dict_cursor_t dict_cursor;
    char *computer_name;
    const SD_workstation_t *workstations = SD_workstation_get_list ();
    xbt_dict_foreach(computers,dict_cursor,computer_name,computer){
      int count_computer = atoi(computer_name);
      unsigned int count=0;
      SD_task_t task;
      SD_task_t task_previous = NULL;
      xbt_dynar_foreach(computer,count,task){
        /* add dependency between the previous and the task to avoid
         * parallel execution */
        if(task != NULL ){
          if(task_previous != NULL &&
             !SD_task_dependency_exists(task_previous, task))
            SD_task_dependency_add(NULL, NULL, task_previous, task);
          SD_task_schedulel(task, 1, workstations[count_computer]);
          task_previous = task;
        }
      }
      xbt_dynar_free(&computer);
    }
    xbt_dict_free(&computers);
    if(acyclic_graph_detail(result))
      return result;
    else
      XBT_WARN("There is at least one cycle in the provided task graph");
  }else{
    XBT_WARN("The scheduling is ignored");
  }
  SD_task_t task;
  unsigned int count;
  xbt_dynar_t computer = NULL;
  xbt_dict_cursor_t dict_cursor;
  char *computer_name;
  xbt_dict_foreach(computers,dict_cursor,computer_name,computer){
    xbt_dynar_free(&computer);
  }
  xbt_dict_free(&computers);
  xbt_dynar_foreach(result,count,task){
     SD_task_destroy(task);
  }
  return NULL;
}

xbt_dynar_t SD_PTG_dotload(const char * filename) {
  xbt_dynar_t result = SD_dotload_generic_alt(filename, parallel);
  if (!acyclic_graph_detail(result)) {
    XBT_ERROR("The DOT described in %s is not a DAG. It contains a cycle.",
              basename((char*)filename));
    xbt_dynar_free(&result);
    /* (result == NULL) here */
  }
  return result;
}

xbt_dynar_t SD_dotload_generic_alt(const char * filename, seq_par_t seq_or_par){
  xbt_assert(filename, "Unable to use a null file descriptor\n");
  unsigned int i;
  result = xbt_dynar_new(sizeof(SD_task_t), dot_task_p_free);
  jobs = xbt_dict_new_homogeneous(NULL);
  FILE *in_file = fopen(filename, "r");
  dag_dot = agread(in_file, NIL(Agdisc_t *));
  SD_task_t root, end, task;
  /*
   * Create all the nodes
   */
  Agnode_t *node = NULL;
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {

    char *name = agnameof(node);
    double amount = atof(agget(node, (char *) "size"));
    double alpha;

    if (seq_or_par == sequential){
      XBT_DEBUG("See <job id=%s amount =%.0f>", name, amount);
    } else {
      alpha = atof(agget(node, (char *) "alpha"));
      if (alpha == -1.)
        alpha = 0.0 ;
      XBT_DEBUG("See <job id=%s amount =%.0f alpha = %.3f>",
          name, amount, alpha);
    }

    if (!(task = xbt_dict_get_or_null(jobs, name))) {
      if (seq_or_par == sequential){
        task = SD_task_create_comp_seq(name, NULL , amount);
      } else {
        task = SD_task_create_comp_par_amdahl(name, NULL , amount, alpha);
      }
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (task, agget (node, (char*)"category"));
#endif
      xbt_dict_set(jobs, name, task, NULL);
      if (!strcmp(name, "root")){
      /* by design the root task is always SCHEDULABLE */
      __SD_task_set_state(task, SD_SCHEDULABLE);
      /* Put it at the beginning of the dynar */
        xbt_dynar_insert_at(result, 0, &task);
      } else {
        if (!strcmp(name, "end")){
          XBT_DEBUG("Declaration of the 'end' node, don't store it yet.");
          end = task;
          /* Should be inserted later in the dynar */
        } else {
          xbt_dynar_push(result, &task);
        }
      }

      if(schedule && seq_or_par == sequential){
        /* try to take the information to schedule the task only if all is
         * right*/
        /* performer is the computer which execute the task */
        int performer = -1;
        char * char_performer = agget(node, (char *) "performer");
        if (char_performer)
          performer = atoi(char_performer);

        /* order is giving the task order on one computer */
        int order = -1;
        char * char_order = agget(node, (char *) "order");
        if (char_order)
          order = atoi(char_order);
        XBT_DEBUG ("Task '%s' is scheduled on workstation '%d' in position '%d'",
                    task->name, performer, order);
        xbt_dynar_t computer = NULL;
        if(performer != -1 && order != -1){
          /* required parameters are given */
          computer = xbt_dict_get_or_null(computers, char_performer);
          if(computer == NULL){
            computer = xbt_dynar_new(sizeof(SD_task_t), NULL);
            xbt_dict_set(computers, char_performer, computer, NULL);
          }
          if(performer < xbt_lib_length(host_lib)){
            /* the wanted computer is available */
            SD_task_t *task_test = NULL;
            if(order < computer->used)
              task_test = xbt_dynar_get_ptr(computer,order);
            if(task_test != NULL && *task_test != NULL && *task_test != task){
              /* the user gives the same order to several tasks */
              schedule = false;
              XBT_VERB("The task %s starts on the computer %s at the position : %s like the task %s",
                     (*task_test)->name, char_performer, char_order,
                     task->name);
            }else{
              /* the parameter seems to be ok */
              xbt_dynar_set_as(computer, order, SD_task_t, task);
            }
          }else{
            /* the platform has not enough processors to schedule the DAG like
             * the user wants*/
            schedule = false;
            XBT_VERB("The schedule is ignored, there are not enough computers");
          }
        }
        else {
          /* one of required parameters is not given */
          schedule = false;
          XBT_VERB("The schedule is ignored, the task %s is not correctly scheduled",
              task->name);
        }
      }
    } else {
      XBT_WARN("Task '%s' is defined more than once", name);
    }
  }

  /*
   * Check if 'root' and 'end' nodes have been explicitly declared.
   * If not, create them.
   */
  if (!(root = xbt_dict_get_or_null(jobs, "root"))){
    if (seq_or_par == sequential)
      root = SD_task_create_comp_seq("root", NULL, 0);
    else
      root = SD_task_create_comp_par_amdahl("root", NULL, 0, 0);
    /* by design the root task is always SCHEDULABLE */
    __SD_task_set_state(root, SD_SCHEDULABLE);
    /* Put it at the beginning of the dynar */
      xbt_dynar_insert_at(result, 0, &root);
  }

  if (!(end = xbt_dict_get_or_null(jobs, "end"))){
    if (seq_or_par == sequential)
      end = SD_task_create_comp_seq("end", NULL, 0);
    else
      end = SD_task_create_comp_par_amdahl("end", NULL, 0, 0);
    /* Should be inserted later in the dynar */
  }

  /*
   * Create edges
   */
  node = NULL;
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    Agedge_t * edge = NULL;
    for (edge = agfstout(dag_dot, node); edge; edge = agnxtout(dag_dot, edge)) {
      SD_task_t src, dst;
      char *src_name=agnameof(agtail(edge));
      char *dst_name=agnameof(aghead(edge));
      double size = atof(agget(edge, (char *) "size"));

      src = xbt_dict_get_or_null(jobs, src_name);
      dst  = xbt_dict_get_or_null(jobs, dst_name);

      if (size > 0) {
        char *name =
            xbt_malloc((strlen(src_name)+strlen(dst_name)+6)*sizeof(char));
        sprintf(name, "%s->%s", src_name, dst_name);
        XBT_DEBUG("See <transfer id=%s amount = %.0f>", name, size);
        if (!(task = xbt_dict_get_or_null(jobs, name))) {
          if (seq_or_par == sequential)
            task = SD_task_create_comm_e2e(name, NULL , size);
          else
            task = SD_task_create_comm_par_mxn_1d_block(name, NULL , size);
#ifdef HAVE_TRACING
          TRACE_sd_dotloader (task, agget (node, (char*)"category"));
#endif
          SD_task_dependency_add(NULL, NULL, src, task);
          SD_task_dependency_add(NULL, NULL, task, dst);
          xbt_dict_set(jobs, name, task, NULL);
          xbt_dynar_push(result, &task);
        } else {
          XBT_WARN("Task '%s' is defined more than once", name);
        }
      } else {
        SD_task_dependency_add(NULL, NULL, src, dst);
      }
    }
  }

  /* all compute and transfer tasks have been created, put the "end" node at
   * the end of dynar
   */
  XBT_DEBUG("All tasks have been created, put %s at the end of the dynar",
      end->name);
  xbt_dynar_push(result, &end);

  /* Connect entry tasks to 'root', and exit tasks to 'end'*/

  xbt_dynar_foreach (result, i, task){
    if (task == root || task == end)
      continue;
    if (xbt_dynar_is_empty(task->tasks_before)) {
      XBT_DEBUG("file '%s' has no source. Add dependency from 'root'",
          task->name);
      SD_task_dependency_add(NULL, NULL, root, task);
    } else if (xbt_dynar_is_empty(task->tasks_after)) {
      XBT_DEBUG("file '%s' has no destination. Add dependency to 'end'",
          task->name);
      SD_task_dependency_add(NULL, NULL, task, end);
    }
  }

  agclose(dag_dot);
  xbt_dict_free(&jobs);

  return result;
}

xbt_dynar_t SD_dotload_generic(const char * filename) {
  xbt_assert(filename, "Unable to use a null file descriptor\n");
  FILE *in_file = fopen(filename, "r");
  dag_dot = agread(in_file, NIL(Agdisc_t *));

  result = xbt_dynar_new(sizeof(SD_task_t), dot_task_p_free);
  files = xbt_dict_new_homogeneous(NULL);
  jobs = xbt_dict_new_homogeneous(NULL);
  computers = xbt_dict_new_homogeneous(NULL);
  root_task = SD_task_create_comp_seq("root", NULL, 0);
  /* by design the root task is always SCHEDULABLE */
  __SD_task_set_state(root_task, SD_SCHEDULABLE);

  xbt_dict_set(jobs, "root", root_task, NULL);
  xbt_dynar_push(result, &root_task);
  end_task = SD_task_create_comp_seq("end", NULL, 0);
  xbt_dict_set(jobs, "end", end_task, NULL);

  Agnode_t *dag_node = NULL;
  for (dag_node = agfstnode(dag_dot); dag_node; dag_node = agnxtnode(dag_dot, dag_node)) {
    dot_add_task(dag_node);
  }
  agclose(dag_dot);
  xbt_dict_free(&jobs);
  /* And now, post-process the files.
   * We want a file task per pair of computation tasks exchanging the file.
   * Duplicate on need
   * Files not produced in the system are said to be produced by root task
   * (top of DAG).
   * Files not consumed in the system are said to be consumed by end task
   * (bottom of DAG).
   */
  xbt_dict_cursor_t cursor;
  SD_task_t file;
  char *name;
  xbt_dict_foreach(files, cursor, name, file) {
    XBT_DEBUG("Considering file '%s' stored in the dictionary",
        file->name);
    if (xbt_dynar_is_empty(file->tasks_before)) {
      XBT_DEBUG("file '%s' has no source. Add dependency from 'root'",
          file->name);
      SD_task_dependency_add(NULL, NULL, root_task, file);
    } else if (xbt_dynar_is_empty(file->tasks_after)) {
      XBT_DEBUG("file '%s' has no destination. Add dependency to 'end'",
          file->name);
      SD_task_dependency_add(NULL, NULL, file, end_task);
    }
    xbt_dynar_push(result, &file);
  }

  /* Push end task last */
  xbt_dynar_push(result, &end_task);

  xbt_dict_free(&files);
  fclose(in_file);
  if (!acyclic_graph_detail(result)) {
    XBT_ERROR("The DOT described in %s is not a DAG. It contains a cycle.",
              basename((char*)filename));
    xbt_dynar_free(&result);
    /* (result == NULL) here */
  }
  return result;
}

/* dot_add_task create a sd_task and all transfers required for this
 * task. The execution time of the task is given by the attribute size.
 * The unit of size is the Flop.*/
void dot_add_task(Agnode_t * dag_node) {
  char *name = agnameof(dag_node);
  SD_task_t current_job;
  double runtime = dot_parse_double(agget(dag_node, (char *) "size"));

  XBT_DEBUG("See <job id=%s runtime=%s %.0f>", name,
        agget(dag_node, (char *) "size"), runtime);

  if (!strcmp(name, "root")){
    XBT_WARN("'root' node is explicitly declared in the DOT file. Update it");
    root_task->amount = runtime;
    root_task->computation_amount[0]= runtime;
#ifdef HAVE_TRACING
    TRACE_sd_dotloader (root_task, agget (dag_node, (char*)"category"));
#endif
  }

  if (!strcmp(name, "end")){
    XBT_WARN("'end' node is explicitly declared in the DOT file. Update it");
    end_task->amount = runtime;
    end_task->computation_amount[0]= runtime;
#ifdef HAVE_TRACING
    TRACE_sd_dotloader (end_task, agget (dag_node, (char*)"category"));
#endif
  }

  current_job = xbt_dict_get_or_null(jobs, name);
  if (!current_job) {
    current_job =
        SD_task_create_comp_seq(name, NULL , runtime);
#ifdef HAVE_TRACING
    TRACE_sd_dotloader (current_job, agget (dag_node, (char*)"category"));
#endif
    xbt_dict_set(jobs, name, current_job, NULL);
    xbt_dynar_push(result, &current_job);
  }
  Agedge_t *e;
  int count = 0;

  for (e = agfstin(dag_dot, dag_node); e; e = agnxtin(dag_dot, e)) {
    dot_add_input_dependencies(current_job, e, sequential);
    count++;
  }
  if (count == 0 && current_job != root_task) {
    SD_task_dependency_add(NULL, NULL, root_task, current_job);
  }
  count = 0;
  for (e = agfstout(dag_dot, dag_node); e; e = agnxtout(dag_dot, e)) {
    dot_add_output_dependencies(current_job, e, sequential);
    count++;
  }
  if (count == 0 && current_job != end_task) {
    SD_task_dependency_add(NULL, NULL, current_job, end_task);
  }

  if(schedule || XBT_LOG_ISENABLED(sd_dotparse, xbt_log_priority_verbose)){
    /* try to take the information to schedule the task only if all is
     * right*/
    /* performer is the computer which execute the task */
    unsigned long performer = -1;
    char * char_performer = agget(dag_node, (char *) "performer");
    if (char_performer != NULL)
      performer = (long) dot_parse_int(char_performer);

    /* order is giving the task order on one computer */
    unsigned long order = -1;
    char * char_order = agget(dag_node, (char *) "order");
    if (char_order != NULL)
      order = (long) dot_parse_int(char_order);
    xbt_dynar_t computer = NULL;
    if(performer != -1 && order != -1){
      /* required parameters are given */
      computer = xbt_dict_get_or_null(computers, char_performer);
      if(computer == NULL){
        computer = xbt_dynar_new(sizeof(SD_task_t), NULL);
        xbt_dict_set(computers, char_performer, computer, NULL);
      }
      if(performer < xbt_lib_length(host_lib)){
        /* the wanted computer is available */
        SD_task_t *task_test = NULL;
        if(order < computer->used)
          task_test = xbt_dynar_get_ptr(computer,order);
        if(task_test != NULL && *task_test != NULL && *task_test != current_job){
          /* the user gives the same order to several tasks */
          schedule = false;
          XBT_VERB("The task %s starts on the computer %s at the position : %s like the task %s",
                 (*task_test)->name, char_performer, char_order,
                 current_job->name);
        }else{
          /* the parameter seems to be ok */
          xbt_dynar_set_as(computer, order, SD_task_t, current_job);
        }
      }else{
        /* the platform has not enough processors to schedule the DAG like
         * the user wants*/
        schedule = false;
        XBT_VERB("The schedule is ignored, there are not enough computers");
      }
    }
    else {
      /* one of required parameters is not given */
      schedule = false;
      XBT_VERB("The schedule is ignored, the task %s is not correctly scheduled",
          current_job->name);
    }
  }
}

/* dot_add_output_dependencies create the dependencies between a task
 * and a transfers. This is given by the edges in the dot file. 
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t * edge,
                                seq_par_t seq_or_par) {
  SD_task_t file = NULL;
  char *name_tail=agnameof(agtail(edge));
  char *name_head=agnameof(aghead(edge));
  char *name = xbt_malloc((strlen(name_head)+strlen(name_tail)+6)*sizeof(char));
  sprintf(name, "%s->%s", name_tail, name_head);
  double size = dot_parse_double(agget(edge, (char *) "size"));
  XBT_DEBUG("add input -- edge: %s, size : %e, get size : %s",
      name, size, agget(edge, (char *) "size"));

  if (size > 0) {
    file = xbt_dict_get_or_null(files, name);
    if (file == NULL) {
      if (seq_or_par == sequential){
          file = SD_task_create_comm_e2e(name, NULL, size);
      } else {
          file = SD_task_create_comm_par_mxn_1d_block(name, NULL, size);
      }
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (file, agget (edge, (char*)"category"));
#endif
      XBT_DEBUG("add input -- adding %s to the dict as new file", name);
      xbt_dict_set(files, name, file, NULL);
    } else {
      XBT_WARN("%s already exists", name);
      if (SD_task_get_amount(file) != size) {
        XBT_WARN("Ignoring file %s size redefinition from %.0f to %.0f",
              name, SD_task_get_amount(file), size);
      }
    }
    SD_task_dependency_add(NULL, NULL, file, current_job);
  } else {
    file = xbt_dict_get_or_null(jobs, name_tail);
    if (file != NULL) {
      SD_task_dependency_add(NULL, NULL, file, current_job);
    }
  }
  free(name);
}

/* dot_add_output_dependencies create the dependencies between a
 * transfers and a task. This is given by the edges in the dot file.
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t * edge,
                                 seq_par_t seq_or_par) {
  SD_task_t file;
  char *name_tail=agnameof(agtail(edge));
  char *name_head=agnameof(aghead(edge));
  char *name = xbt_malloc((strlen(name_head)+strlen(name_tail)+6)*sizeof(char));
  sprintf(name, "%s->%s", name_tail, name_head);
  double size = dot_parse_double(agget(edge, (char *) "size"));
  XBT_DEBUG("add_output -- edge: %s, size : %e, get size : %s",
      name, size, agget(edge, (char *) "size"));

  if (size > 0) {
    file = xbt_dict_get_or_null(files, name);
    if (file == NULL) {
      if (seq_or_par == sequential){
          file = SD_task_create_comm_e2e(name, NULL, size);
      } else {
          file = SD_task_create_comm_par_mxn_1d_block(name, NULL, size);
      }
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (file, agget (edge, (char*)"category"));
#endif
      XBT_DEBUG("add output -- adding %s to the dict as new file", name);
      xbt_dict_set(files, name, file, NULL);
    } else {
      XBT_WARN("%s already exists", name);
      if (SD_task_get_amount(file) != size) {
        XBT_WARN("Ignoring file %s size redefinition from %.0f to %.0f",
              name, SD_task_get_amount(file), size);
      }
    }
    SD_task_dependency_add(NULL, NULL, current_job, file);
    if (xbt_dynar_length(file->tasks_before) > 1) {
      XBT_WARN("File %s created at more than one location...", file->name);
    }
  } else {
    file = xbt_dict_get_or_null(jobs, name_head);
    if (file != NULL) {
      SD_task_dependency_add(NULL, NULL, current_job, file);
    }
  }
  free(name);
}
