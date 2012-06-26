/* Copyright (c) 2009, 2010. The SimGrid Team.
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
#endif

void dot_add_task(Agnode_t * dag_node);
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t * edge);
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t * edge);
xbt_dynar_t SD_dotload_generic(const char * filename);

static double dot_parse_double(const char *string)
{
    if (string == NULL)
          return -1;
      double value = -1;
        char *err;

          //ret = sscanf(string, "%lg", &value);
          errno = 0;
            value = strtod(string,&err);
              if(errno)
                  {
                        XBT_WARN("Failed to convert string to double: %s\n",strerror(errno));
                            return -1;
                              }
                return value;
}


static int dot_parse_int(const char *string)
{
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

static void dump_res()
{
  unsigned int cursor;
  SD_task_t task;
  xbt_dynar_foreach(result, cursor, task) {
    XBT_INFO("Task %d", cursor);
    SD_task_dump(task);
  }
}


static void dot_task_free(void *task)
{
  SD_task_t t = task;
  SD_task_destroy(t);
}

static void TRACE_sd_dotloader (SD_task_t task, const char *category)
{
  if (category){
    if (strlen (category) != 0){
      TRACE_category (category);
      SD_task_set_category (task, category);
    }
  }
}

/** @brief loads a DOT file describing a DAG
 * 
 * See http://www.graphviz.org/doc/info/lang.html
 * for more details.
 * To obtain information about transfers and tasks, two attributes are
 * required : size on task (execution time in Flop) and size on edge
 * (the amount of data transfer in bit).
 * if they aren't here, there choose to be equal to zero.
 */
xbt_dynar_t SD_dotload(const char *filename)
{
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

xbt_dynar_t SD_dotload_with_sched(const char *filename){
  SD_dotload_generic(filename);

  if(schedule == true){
    xbt_dynar_t computer = NULL;
    xbt_dict_cursor_t dict_cursor;
    char *computer_name;
    const SD_workstation_t *workstations = SD_workstation_get_list ();
    xbt_dict_foreach(computers,dict_cursor,computer_name,computer){
      int count_computer = dot_parse_int(computer_name);
      unsigned int count=0;
      SD_task_t task;
      SD_task_t task_previous = NULL;
      xbt_dynar_foreach(computer,count,task){
        // add dependency between the previous and the task to avoid
        // parallel execution
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
  return NULL;
}

xbt_dynar_t SD_dotload_generic(const char * filename)
{
  xbt_assert(filename, "Unable to use a null file descriptor\n");
  //dag_dot =  agopen((char*)filename,Agstrictdirected,0);
  FILE *in_file = fopen(filename, "r");
  dag_dot = agread(in_file, NIL(Agdisc_t *));

  result = xbt_dynar_new(sizeof(SD_task_t), dot_task_free);
  files = xbt_dict_new_homogeneous(&dot_task_free);
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
  for (dag_node = agfstnode(dag_dot); dag_node;
#ifdef HAVE_CGRAPH_H
       dag_node = agnxtnode(dag_dot, dag_node)
#elif HAVE_AGRAPH_H
       dag_node = agnxtnode(dag_node)
#endif
       ) {

  dot_add_task(dag_node);
  }
  agclose(dag_dot);
  xbt_dict_free(&jobs);

  /* And now, post-process the files.
   * We want a file task per pair of computation tasks exchanging the file. Duplicate on need
   * Files not produced in the system are said to be produced by root task (top of DAG).
   * Files not consumed in the system are said to be consumed by end task (bottom of DAG).
   */
  xbt_dict_cursor_t cursor;
  SD_task_t file;
  char *name;
  xbt_dict_foreach(files, cursor, name, file) {
    unsigned int cpt1, cpt2;
    SD_task_t newfile = NULL;
    SD_dependency_t depbefore, depafter;
    if (xbt_dynar_is_empty(file->tasks_before)) {
      xbt_dynar_foreach(file->tasks_after, cpt2, depafter) {
        SD_task_t newfile =
            SD_task_create_comm_e2e(file->name, NULL, file->amount);
        SD_task_dependency_add(NULL, NULL, root_task, newfile);
        SD_task_dependency_add(NULL, NULL, newfile, depafter->dst);
        xbt_dynar_push(result, &newfile);
      }
    } else if (xbt_dynar_is_empty(file->tasks_after)) {
      xbt_dynar_foreach(file->tasks_before, cpt2, depbefore) {
        SD_task_t newfile =
            SD_task_create_comm_e2e(file->name, NULL, file->amount);
        SD_task_dependency_add(NULL, NULL, depbefore->src, newfile);
        SD_task_dependency_add(NULL, NULL, newfile, end_task);
        xbt_dynar_push(result, &newfile);
      }
    } else {
      xbt_dynar_foreach(file->tasks_before, cpt1, depbefore) {
        xbt_dynar_foreach(file->tasks_after, cpt2, depafter) {
          if (depbefore->src == depafter->dst) {
            XBT_WARN
                ("File %s is produced and consumed by task %s. This loop dependency will prevent the execution of the task.",
                 file->name, depbefore->src->name);
          }
          newfile =
              SD_task_create_comm_e2e(file->name, NULL, file->amount);
          SD_task_dependency_add(NULL, NULL, depbefore->src, newfile);
          SD_task_dependency_add(NULL, NULL, newfile, depafter->dst);
          xbt_dynar_push(result, &newfile);
        }
      }
    }
  }

  /* Push end task last */
  xbt_dynar_push(result, &end_task);

  /* Free previous copy of the files */
  xbt_dict_free(&files);
  fclose(in_file);
  if(acyclic_graph_detail(result))
    return result;
  else {
    unsigned int cpt;
    XBT_ERROR("The DOT described in %s is not a DAG. It contains a cycle.",
              basename((char*)filename));
    xbt_dynar_foreach(result, cpt, file)
      SD_task_destroy(file);
     xbt_dynar_free_container(&result);
  }
  free(dag_dot);
  return NULL;
}

/* dot_add_task create a sd_task and all transfers required for this
 * task. The execution time of the task is given by the attribute size.
 * The unit of size is the Flop.*/
void dot_add_task(Agnode_t * dag_node)
{
  char *name = agnameof(dag_node);
  SD_task_t current_job;
  double runtime = dot_parse_double(agget(dag_node, (char *) "size"));

  XBT_DEBUG("See <job id=%s runtime=%s %.0f>", name,
        agget(dag_node, (char *) "size"), runtime);
  current_job = xbt_dict_get_or_null(jobs, name);
  if (current_job == NULL) {
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

#ifdef HAVE_CGRAPH_H
  for (e = agfstin(dag_dot, dag_node); e; e = agnxtin(dag_dot, e))
#elif HAVE_AGRAPH_H
  for (e = agfstin(dag_node); e; e = agnxtin(e))
#endif
  {
  dot_add_input_dependencies(current_job, e);
  count++;
  }
  if (count == 0 && current_job != root_task) {
    SD_task_dependency_add(NULL, NULL, root_task, current_job);
  }
  count = 0;
#ifdef HAVE_CGRAPH_H
  for (e = agfstout(dag_dot, dag_node); e; e = agnxtout(dag_dot, e))
#elif HAVE_AGRAPH_H
  for (e = agfstout(dag_node); e; e = agnxtout(e))
#endif
  {

    dot_add_output_dependencies(current_job, e);
    count++;
  }
  if (count == 0 && current_job != end_task) {
    SD_task_dependency_add(NULL, NULL, current_job, end_task);
  }

  if(schedule || XBT_LOG_ISENABLED(sd_dotparse, xbt_log_priority_verbose)){
    /* try to take the information to schedule the task only if all is
     * right*/
    // performer is the computer which execute the task
    unsigned long performer = -1;
    char * char_performer = agget(dag_node, (char *) "performer");
    if (char_performer != NULL)
      performer = (long) dot_parse_int(char_performer);

    // order is giving the task order on one computer
    unsigned long order = -1;
    char * char_order = agget(dag_node, (char *) "order");
    if (char_order != NULL)
      order = (long) dot_parse_int(char_order);
    xbt_dynar_t computer = NULL;
    //XBT_INFO("performer = %d, order=%d",performer,order);
    if(performer != -1 && order != -1){
      //necessary parameters are given
      computer = xbt_dict_get_or_null(computers, char_performer);
      if(computer == NULL){
        computer = xbt_dynar_new(sizeof(SD_task_t), NULL);
        xbt_dict_set(computers, char_performer, computer, NULL);
      }
      if(performer < xbt_lib_length(host_lib)){
        // the  wanted computer is available
        SD_task_t *task_test = NULL;
        if(order < computer->used)
          task_test = xbt_dynar_get_ptr(computer,order);
        if(task_test != NULL && *task_test != NULL && *task_test != current_job){
          /*the user gives the same order to several tasks*/
          schedule = false;
          XBT_VERB("The task %s starts on the computer %s at the position : %s like the task %s",
                 (*task_test)->name, char_performer, char_order, current_job->name);
        }else{
          //the parameter seems to be ok
          xbt_dynar_set_as(computer, order, SD_task_t, current_job);
        }
      }else{
        /*the platform has not enough processors to schedule the DAG like
        *the user wants*/
        schedule = false;
        XBT_VERB("The schedule is ignored, there are not enough computers");
      }
    }
    else {
      //one of necessary parameters are not given
      schedule = false;
      XBT_VERB("The schedule is ignored, the task %s is not correctly scheduled", current_job->name);
    }
  }
}

/* dot_add_output_dependencies create the dependencies between a task
 * and a transfers. This is given by the edges in the dot file. 
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t * edge)
{
  SD_task_t file = NULL;
  char *name_tail=agnameof(agtail(edge));
  char *name_head=agnameof(aghead(edge));
  char *name = malloc((strlen(name_head)+strlen(name_tail)+6)*sizeof(char));
  sprintf(name, "%s->%s", name_tail, name_head);
  double size = dot_parse_double(agget(edge, (char *) "size"));
  XBT_DEBUG("size : %e, get size : %s", size, agget(edge, (char *) "size"));

  if (size > 0) {
    file = xbt_dict_get_or_null(files, name);
    if (file == NULL) {
      file = SD_task_create_comm_e2e(name, NULL, size);
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (file, agget (edge, (char*)"category"));
#endif
      xbt_dict_set(files, name, file, NULL);
    } else {
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
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t * edge)
{
  SD_task_t file;
  char *name_tail=agnameof(agtail(edge));
  char *name_head=agnameof(aghead(edge));
  char *name = malloc((strlen(name_head)+strlen(name_tail)+6)*sizeof(char));
  sprintf(name, "%s->%s", name_tail, name_head);
  double size = dot_parse_double(agget(edge, (char *) "size"));
  XBT_DEBUG("size : %e, get size : %s", size, agget(edge, (char *) "size"));

  if (size > 0) {
    file = xbt_dict_get_or_null(files, name);
    if (file == NULL) {
      file = SD_task_create_comm_e2e(name, NULL, size);
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (file, agget (edge, (char*)"category"));
#endif
      xbt_dict_set(files, name, file, NULL);
    } else {
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
