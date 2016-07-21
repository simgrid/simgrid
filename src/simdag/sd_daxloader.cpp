/* Copyright (c) 2009-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simdag/simdag_private.h"
#include "simgrid/simdag.h"
#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/file.h" /* xbt_basename() */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_daxparse, sd, "Parsing DAX files");

extern "C" {
  #undef CLEANUP
  #include "dax_dtd.h"
  #define register /* g++ don't like register, so don't say it */
  #include "dax_dtd.c"
  #undef register
}

/* Ensure that transfer tasks have unique names even though a file is used several times */

void uniq_transfer_task_name(SD_task_t task)
{
  SD_task_t child = *(task->successors->begin());
  SD_task_t parent = *(task->predecessors->begin());

  char *new_name = bprintf("%s_%s_%s", SD_task_get_name(parent), SD_task_get_name(task), SD_task_get_name(child));

  SD_task_set_name(task, new_name);

  free(new_name);
}

static bool children_are_marked(SD_task_t task){
  for (std::set<SD_task_t>::iterator it=task->successors->begin(); it!=task->successors->end(); ++it)
    if ((*it)->marked == 0)
      return false;
  for (std::set<SD_task_t>::iterator it=task->outputs->begin(); it!=task->outputs->end(); ++it)
    if ((*it)->marked == 0)
      return false;
  return true;
}

static bool parents_are_marked(SD_task_t task){
  for (std::set<SD_task_t>::iterator it=task->predecessors->begin(); it!=task->predecessors->end(); ++it)
    if ((*it)->marked == 0)
      return false;
  for (std::set<SD_task_t>::iterator it=task->inputs->begin(); it!=task->inputs->end(); ++it)
    if ((*it)->marked == 0)
      return false;
  return true;
}

bool acyclic_graph_detail(xbt_dynar_t dag){
  unsigned int count, count_current=0;
  bool all_marked = true;
  SD_task_t task = nullptr;
  xbt_dynar_t next = nullptr, current = xbt_dynar_new(sizeof(SD_task_t),nullptr);

  xbt_dynar_foreach(dag,count,task){
    if(task->kind == SD_TASK_COMM_E2E) continue;
    task->marked = 0;
    if(task->successors->empty() && task->outputs->empty())
      xbt_dynar_push(current, &task);
  }
  //test if something has to be done for the next iteration
  while(!xbt_dynar_is_empty(current)){
    next = xbt_dynar_new(sizeof(SD_task_t),nullptr);
    //test if the current iteration is done
    xbt_dynar_foreach(current,count_current,task){
      if (task == nullptr)
        continue;
      //push task in next
      task->marked = 1;
      for (std::set<SD_task_t>::iterator it=task->inputs->begin(); it!=task->inputs->end(); ++it){
        (*it)->marked = 1;
        // Inputs are communication, hence they can have only one predecessor
        SD_task_t input_pred = *((*it)->predecessors->begin());
        if (children_are_marked(input_pred))
          xbt_dynar_push(next, &input_pred);
      }
      for (std::set<SD_task_t>::iterator it=task->predecessors->begin(); it!=task->predecessors->end(); ++it){
        if (children_are_marked(*it))
          xbt_dynar_push(next, &(*it));
      }
    }
    xbt_dynar_free(&current);
    current = next;
    next = nullptr;
  }
  xbt_dynar_free(&current);
  all_marked = true;
  xbt_dynar_foreach(dag,count,task){
    if(task->kind == SD_TASK_COMM_E2E)
      continue;
    //test if all tasks are marked
    if(task->marked == 0){
      XBT_WARN("the task %s is not marked",task->name);
      all_marked = false;
      break;
    }
  }
  if(!all_marked){
    XBT_VERB("there is at least one cycle in your task graph");

    current = xbt_dynar_new(sizeof(SD_task_t),nullptr);
    xbt_dynar_foreach(dag,count,task){
      if(task->kind == SD_TASK_COMM_E2E)
        continue;
      if(task->predecessors->empty() && task->inputs->empty()){
        xbt_dynar_push(current, &task);
      }
    }

    xbt_dynar_foreach(dag,count,task){
      if(task->kind == SD_TASK_COMM_E2E)
        continue;
      if(task->predecessors->empty() && task->inputs->empty()){
        task->marked = 1;
        xbt_dynar_push(current, &task);
      }
    }
    //test if something has to be done for the next iteration
    while(!xbt_dynar_is_empty(current)){
      next = xbt_dynar_new(sizeof(SD_task_t),nullptr);
      //test if the current iteration is done
      xbt_dynar_foreach(current,count_current,task){
        if (task == nullptr)
          continue;
        //push task in next
        task->marked = 1;
        for (std::set<SD_task_t>::iterator it=task->outputs->begin(); it!=task->outputs->end(); ++it){
          (*it)->marked = 1;
          // outputs are communication, hence they can have only one successor
          SD_task_t output_succ = *((*it)->successors->begin());
          if (parents_are_marked(output_succ))
            xbt_dynar_push(next, &output_succ);
        }
        for (std::set<SD_task_t>::iterator it=task->predecessors->begin(); it!=task->predecessors->end(); ++it){
          if (parents_are_marked(*it))
            xbt_dynar_push(next, &(*it));
        }
        xbt_dynar_free(&current);
        current = next;
        next = nullptr;
      }
      xbt_dynar_free(&current);
      all_marked = true;
      xbt_dynar_foreach(dag,count,task){
        if(task->kind == SD_TASK_COMM_E2E)
          continue;
        //test if all tasks are marked
        if(task->marked == 0){
          XBT_WARN("the task %s is in a cycle",task->name);
          all_marked = false;
        }
      }
    }
  }
  return all_marked;
}

static YY_BUFFER_STATE input_buffer;

static xbt_dynar_t result;
static xbt_dict_t jobs;
static xbt_dict_t files;
static SD_task_t current_job;
static SD_task_t root_task, end_task;

static void dax_task_free(void *task)
{
  SD_task_destroy(static_cast<SD_task_t>(task));
}

/** @brief loads a DAX file describing a DAG
 * 
 * See https://confluence.pegasus.isi.edu/display/pegasus/WorkflowGenerator for more details.
 */
xbt_dynar_t SD_daxload(const char *filename)
{
  xbt_dict_cursor_t cursor;
  SD_task_t file;
  char *name;
  FILE *in_file = fopen(filename, "r");
  xbt_assert(in_file, "Unable to open \"%s\"\n", filename);
  input_buffer = dax__create_buffer(in_file, 10);
  dax__switch_to_buffer(input_buffer);
  dax_lineno = 1;

  result = xbt_dynar_new(sizeof(SD_task_t), dax_task_free);
  files = xbt_dict_new_homogeneous(&dax_task_free);
  jobs = xbt_dict_new_homogeneous(nullptr);
  root_task = SD_task_create_comp_seq("root", nullptr, 0);
  /* by design the root task is always SCHEDULABLE */
  SD_task_set_state(root_task, SD_SCHEDULABLE);

  xbt_dynar_push(result, &root_task);
  end_task = SD_task_create_comp_seq("end", nullptr, 0);

  int res = dax_lex();
  if (res != 0)
    xbt_die("Parse error in %s: %s", filename, dax__parse_err_msg());
  dax__delete_buffer(input_buffer);
  fclose(in_file);
  dax_lex_destroy();
  xbt_dict_free(&jobs);

  /* And now, post-process the files.
   * We want a file task per pair of computation tasks exchanging the file. Duplicate on need
   * Files not produced in the system are said to be produced by root task (top of DAG).
   * Files not consumed in the system are said to be consumed by end task (bottom of DAG).
   */

  xbt_dict_foreach(files, cursor, name, file) {
    SD_task_t newfile;
    if (file->predecessors->empty()) {
      for (std::set<SD_task_t>::iterator it=file->successors->begin(); it!=file->successors->end(); ++it){
        newfile = SD_task_create_comm_e2e(file->name, nullptr, file->amount);
        SD_task_dependency_add(nullptr, nullptr, root_task, newfile);
        SD_task_dependency_add(nullptr, nullptr, newfile, (*it));
        xbt_dynar_push(result, &newfile);
      }
    } else if (file->successors->empty()) {
      for (std::set<SD_task_t>::iterator it=file->predecessors->begin(); it!=file->predecessors->end(); ++it){
        newfile = SD_task_create_comm_e2e(file->name, nullptr, file->amount);
        SD_task_dependency_add(nullptr, nullptr, (*it), newfile);
        SD_task_dependency_add(nullptr, nullptr, newfile, end_task);
        xbt_dynar_push(result, &newfile);
      }
    } else {
      for (std::set<SD_task_t>::iterator it=file->predecessors->begin(); it!=file->predecessors->end(); ++it){
        for (std::set<SD_task_t>::iterator it2=file->successors->begin(); it2!=file->successors->end(); ++it2){
          if (*it == *it2) {
            XBT_WARN ("File %s is produced and consumed by task %s."
                      "This loop dependency will prevent the execution of the task.", file->name, (*it)->name);
          }
          newfile = SD_task_create_comm_e2e(file->name, nullptr, file->amount);
          SD_task_dependency_add(nullptr, nullptr, (*it), newfile);
          SD_task_dependency_add(nullptr, nullptr, newfile, (*it2));
          xbt_dynar_push(result, &newfile);
        }
      }
    }
  }

  /* Push end task last */
  xbt_dynar_push(result, &end_task);

  /* Free previous copy of the files */
  xbt_dict_free(&files);
  unsigned int cpt;
  xbt_dynar_foreach(result, cpt, file) {
    if (SD_task_get_kind(file) == SD_TASK_COMM_E2E) {
      uniq_transfer_task_name(file);
    } else if (SD_task_get_kind(file) == SD_TASK_COMP_SEQ){
      /* If some tasks do not take files as input, connect them to the root
       * if they don't produce files, connect them to the end node.
       */
      if ((file != root_task) && file->inputs->empty())
        SD_task_dependency_add(nullptr, nullptr, root_task, file);
      if ((file != end_task) && file->outputs->empty())
        SD_task_dependency_add(nullptr, nullptr, file, end_task);
    } else {
       THROW_IMPOSSIBLE;
    }
  }

  if (!acyclic_graph_detail(result)) {
    char* base = xbt_basename(filename);
    XBT_ERROR("The DAX described in %s is not a DAG. It contains a cycle.", base);
    free(base);
    xbt_dynar_foreach(result, cpt, file)
      SD_task_destroy(file);
    xbt_dynar_free_container(&result);
    return nullptr;
  } else {
    return result;
  }
}

void STag_dax__adag()
{
  XBT_ATTRIB_UNUSED double version;
  version = xbt_str_parse_double(A_dax__adag_version, "Parse error: %s is not a double");

  xbt_assert(version == 2.1, "Expected version 2.1 in <adag> tag, got %f. Fix the parser or your file", version);
}

void STag_dax__job()
{
  double runtime = xbt_str_parse_double(A_dax__job_runtime, "Parse error: %s is not a double");
  char *name = bprintf("%s@%s", A_dax__job_id, A_dax__job_name);
  runtime *= 4200000000.;       /* Assume that timings were done on a 4.2GFlops machine. I mean, why not? */
  XBT_DEBUG("See <job id=%s runtime=%s %.0f>",A_dax__job_id,A_dax__job_runtime,runtime);
  current_job = SD_task_create_comp_seq(name, nullptr, runtime);
  xbt_dict_set(jobs, A_dax__job_id, current_job, nullptr);
  free(name);
  xbt_dynar_push(result, &current_job);
}

void STag_dax__uses(void)
{
  double size = xbt_str_parse_double(A_dax__uses_size, "Parse error: %s is not a double");
  int is_input = (A_dax__uses_link == A_dax__uses_link_input);

  XBT_DEBUG("See <uses file=%s %s>",A_dax__uses_file,(is_input?"in":"out"));
  SD_task_t file = static_cast<SD_task_t>(xbt_dict_get_or_null(files, A_dax__uses_file));
  if (file == nullptr) {
    file = SD_task_create_comm_e2e(A_dax__uses_file, nullptr, size);
    sd_global->initial_tasks->erase(file);
    xbt_dict_set(files, A_dax__uses_file, file, nullptr);
  } else {
    if (SD_task_get_amount(file) != size) {
      XBT_WARN("Ignore file %s size redefinition from %.0f to %.0f", A_dax__uses_file, SD_task_get_amount(file), size);
    }
  }
  if (is_input) {
    SD_task_dependency_add(nullptr, nullptr, file, current_job);
  } else {
    SD_task_dependency_add(nullptr, nullptr, current_job, file);
    if ((file->predecessors->size() + file->inputs->size()) > 1) {
      XBT_WARN("File %s created at more than one location...", file->name);
    }
  }
}

static SD_task_t current_child;
void STag_dax__child()
{
  current_child = static_cast<SD_task_t>(xbt_dict_get_or_null(jobs, A_dax__child_ref));
  xbt_assert(current_child != nullptr,"Parse error on line %d: Asked to add dependencies to the non-existent %s task",
             dax_lineno, A_dax__child_ref);
}

void ETag_dax__child()
{
  current_child = nullptr;
}

void STag_dax__parent()
{
  SD_task_t parent = static_cast<SD_task_t>(xbt_dict_get_or_null(jobs, A_dax__parent_ref));
  xbt_assert(parent != nullptr, "Parse error on line %d: Asked to add a dependency from %s to %s, but %s does not exist",
             dax_lineno, current_child->name, A_dax__parent_ref, A_dax__parent_ref);
  SD_task_dependency_add(nullptr, nullptr, parent, current_child);
  XBT_DEBUG("Control-flow dependency from %s to %s", current_child->name, parent->name);
}

void ETag_dax__adag()
{
  XBT_DEBUG("See </adag>");
}

void ETag_dax__job(void)
{
  current_job = nullptr;
  XBT_DEBUG("See </job>");
}

void ETag_dax__parent(void)
{
  XBT_DEBUG("See </parent>");
}

void ETag_dax__uses(void)
{
  XBT_DEBUG("See </uses>");
}
