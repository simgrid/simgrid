/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/misc.h"
#include "xbt/log.h"
#include <stdbool.h>

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
xbt_dynar_t SD_dotload_FILE(FILE * in_file);

static double dot_parse_double(const char *string)
{
  if (string == NULL)
    return -1;
  int ret = 0;
  double value = -1;

  ret = sscanf(string, "%lg", &value);
  if (ret != 1)
    WARN1("%s is not a double", string);
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
    WARN1("%s is not an integer", string);
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
    INFO1("Task %d", cursor);
    SD_task_dump(task);
  }
}

bool child_are_marked(SD_task_t task){
  SD_task_t child_task = NULL;
  bool all_marked = true;
  SD_dependency_t depafter = NULL;
  int count;
  xbt_dynar_foreach(task->tasks_after,count,depafter){
    child_task = depafter->dst;
    //test marked
    if(child_task->marked == 0) {
      all_marked = false;
      break;
    }
    child_task = NULL;
  }
  return all_marked;
}

bool acyclic_graph_detection(xbt_dynar_t dag){
  int count=0, count_current=0;
  bool all_marked = true;
  SD_task_t task = NULL, parent_task = NULL;
  SD_dependency_t depbefore = NULL;
  xbt_dynar_t next = NULL, current = xbt_dynar_new(sizeof(SD_task_t),NULL);

  xbt_dynar_foreach(dag,count,task){
    if(task->kind == SD_TASK_COMM_E2E) continue;
    task->marked = 0;
    if(xbt_dynar_length(task->tasks_after) == 0){
      xbt_dynar_push(current, &task);
    }
  }
  task = NULL;
  count = 0;
  //test if something has to be done for the next iteration
  while(xbt_dynar_length(current) != 0){
    next = xbt_dynar_new(sizeof(SD_task_t),NULL);
    //test if the current iteration is done
    count_current=0;
    xbt_dynar_foreach(current,count_current,task){
      if (task == NULL) continue;
      count = 0;
      //push task in next
      task->marked = 1;
      int count = 0;
      xbt_dynar_foreach(task->tasks_before,count,depbefore){
        parent_task = depbefore->src;
        if(parent_task->kind == SD_TASK_COMM_E2E){
          int j=0;
          parent_task->marked = 1;
          xbt_dynar_foreach(parent_task->tasks_before,j,depbefore){
            parent_task = depbefore->src;
            if(child_are_marked(parent_task))
              xbt_dynar_push(next, &parent_task);
          }
        } else{
          if(child_are_marked(parent_task))
            xbt_dynar_push(next, &parent_task);
        }
        parent_task = NULL;
      }
      task = NULL;
      count = 0;
    }
    xbt_dynar_free(&current);
    current = next;
    next = NULL;
  }
  xbt_dynar_free(&current);
  current = NULL;
  all_marked = true;
  xbt_dynar_foreach(dag,count,task){
    if(task->kind == SD_TASK_COMM_E2E) continue;
    //test if all tasks are marked
    if(task->marked == 0){
      WARN1("test %d",task->name);
      all_marked = false;
      break;
    }
  }
  task = NULL;
  if(all_marked){
    WARN0("there are no cycle in your DAG");
  }else{
    WARN0("there are a cycle in your DAG");
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
      TRACE_sd_set_task_category (task, category);
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
  FILE *in_file = fopen(filename, "r");
  xbt_assert1(in_file, "Unable to open \"%s\"\n", filename);
  SD_dotload_FILE(in_file);
  fclose(in_file);
  return result;
}

xbt_dynar_t SD_dotload_FILE(FILE * in_file)
{
  xbt_assert0(in_file, "Unable to use a null file descriptor\n");
  dag_dot = agread(in_file, NIL(Agdisc_t *));

  result = xbt_dynar_new(sizeof(SD_task_t), dot_task_free);
  files = xbt_dict_new();
  jobs = xbt_dict_new();
  computers = xbt_dict_new();
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
    if (xbt_dynar_length(file->tasks_before) == 0) {
      xbt_dynar_foreach(file->tasks_after, cpt2, depafter) {
        SD_task_t newfile =
            SD_task_create_comm_e2e(file->name, NULL, file->amount);
        SD_task_dependency_add(NULL, NULL, root_task, newfile);
        SD_task_dependency_add(NULL, NULL, newfile, depafter->dst);
        xbt_dynar_push(result, &newfile);
      }
    } else if (xbt_dynar_length(file->tasks_after) == 0) {
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
            WARN2
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
  if(schedule == false){
    WARN0("No scheduling provided");
  }else{
    xbt_dynar_t computer = NULL;
    xbt_dict_cursor_t dict_cursor;
    char *computer_name;
    const SD_workstation_t *workstations = SD_workstation_get_list ();
    xbt_dict_foreach(computers,dict_cursor,computer_name,computer){
      int count_computer = dot_parse_int(computer_name);
      int count=0;
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
  }
  acyclic_graph_detection(result);

  return result;
}

/* dot_add_task create a sd_task and all transfers required for this
 * task. The execution time of the task is given by the attribute size.
 * The unit of size is the Flop.*/
void dot_add_task(Agnode_t * dag_node)
{
  char *name = agnameof(dag_node);
  SD_task_t current_job;
  double runtime = dot_parse_double(agget(dag_node, (char *) "size"));

  DEBUG3("See <job id=%s runtime=%s %.0f>", name,
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

  if(schedule || true){
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
    //INFO2("performer = %d, order=%d",performer,order);
    if(performer != -1 && order != -1){
      //necessary parameters are given
      computer = xbt_dict_get_or_null(computers, char_performer);
      if(computer == NULL){
        computer = xbt_dynar_new(sizeof(SD_task_t), NULL);
        xbt_dict_set(computers, char_performer, computer, NULL);
      }
      if(performer < sd_global->workstation_count){
        // the  wanted computer is available
        SD_task_t *task_test = NULL;
        if(order < computer->used)
          task_test = xbt_dynar_get_ptr(computer,order);
        if(task_test != NULL && *task_test != NULL && *task_test != current_job){
          /*the user gives the same order to several tasks*/
          schedule = false;
          WARN0("scheduling does not take into account, several task has\
                the same order");
        }else{
          //the parameter seems to be ok
          xbt_dynar_set_as(computer, order, SD_task_t, current_job);
        }
      }else{
        /*the platform has not enough processors to schedule the DAG like
        *the user wants*/
        schedule = false;
        WARN0("scheduling does not take into account, not enough computers");
      }
    }
    else if((performer == -1 && order != -1) ||
            (performer != -1 && order == -1)){
      //one of necessary parameters are not given
      schedule = false;
      WARN0("scheduling does not take into account");
    } else {
      //No schedule available
      schedule = false;
    }
  }
}

/* dot_add_output_dependencies create the dependencies between a task
 * and a transfers. This is given by the edges in the dot file. 
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t * edge)
{
  SD_task_t file;

  char name[80];
  sprintf(name, "%s->%s", agnameof(agtail(edge)), agnameof(aghead(edge)));
  double size = dot_parse_double(agget(edge, (char *) "size"));
  DEBUG2("size : %e, get size : %s", size, agget(edge, (char *) "size"));

  if (size > 0) {
    file = xbt_dict_get_or_null(files, name);
    if (file == NULL) {
      file = SD_task_create_comm_e2e(name, NULL, size);
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (file, agget (edge, (char*)"category"));
#endif
      xbt_dict_set(files, name, file, &dot_task_free);
    } else {
      if (SD_task_get_amount(file) != size) {
        WARN3("Ignoring file %s size redefinition from %.0f to %.0f",
              name, SD_task_get_amount(file), size);
      }
    }
    SD_task_dependency_add(NULL, NULL, file, current_job);
  } else {
    file = xbt_dict_get_or_null(jobs, agnameof(agtail(edge)));
    if (file != NULL) {
      SD_task_dependency_add(NULL, NULL, file, current_job);
    }
  }
}

/* dot_add_output_dependencies create the dependencies between a
 * transfers and a task. This is given by the edges in the dot file.
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t * edge)
{
  SD_task_t file;
  char name[80];
  sprintf(name, "%s->%s", agnameof(agtail(edge)), agnameof(aghead(edge)));
  double size = dot_parse_double(agget(edge, (char *) "size"));
  DEBUG2("size : %e, get size : %s", size, agget(edge, (char *) "size"));

  if (size > 0) {
    file = xbt_dict_get_or_null(files, name);
    if (file == NULL) {
      file = SD_task_create_comm_e2e(name, NULL, size);
#ifdef HAVE_TRACING
      TRACE_sd_dotloader (file, agget (edge, (char*)"category"));
#endif
      xbt_dict_set(files, name, file, &dot_task_free);
    } else {
      if (SD_task_get_amount(file) != size) {
        WARN3("Ignoring file %s size redefinition from %.0f to %.0f",
              name, SD_task_get_amount(file), size);
      }
    }
    SD_task_dependency_add(NULL, NULL, current_job, file);
    if (xbt_dynar_length(file->tasks_before) > 1) {
      WARN1("File %s created at more than one location...", file->name);
    }
  } else {
    file = xbt_dict_get_or_null(jobs, agnameof(aghead(edge)));
    if (file != NULL) {
      SD_task_dependency_add(NULL, NULL, current_job, file);
    }
  }
}
