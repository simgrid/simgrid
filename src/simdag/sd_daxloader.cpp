/* Copyright (c) 2009-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simdag_private.hpp"
#include "simgrid/simdag.h"
#include "xbt/file.hpp"
#include "xbt/log.h"
#include "xbt/misc.h"
#include <algorithm>
#include <map>
#include <stdexcept>

#include "dax_dtd.h"
#include "dax_dtd.c"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_daxparse, sd, "Parsing DAX files");

/* Ensure that transfer tasks have unique names even though a file is used several times */
void uniq_transfer_task_name(SD_task_t task)
{
  SD_task_t child = *(task->successors->begin());
  SD_task_t parent = *(task->predecessors->begin());

  std::string new_name =
      std::string(SD_task_get_name(parent)) + "_" + SD_task_get_name(task) + "_" + SD_task_get_name(child);

  SD_task_set_name(task, new_name.c_str());
}

static bool children_are_marked(const_SD_task_t task)
{
  return std::none_of(task->successors->begin(), task->successors->end(),
                      [](const SD_task_t& elm) { return not elm->marked; }) &&
         std::none_of(task->outputs->begin(), task->outputs->end(),
                      [](const SD_task_t& elm) { return not elm->marked; });
}

static bool parents_are_marked(const_SD_task_t task)
{
  return std::none_of(task->predecessors->begin(), task->predecessors->end(),
                      [](const SD_task_t& elm) { return not elm->marked; }) &&
         std::none_of(task->inputs->begin(), task->inputs->end(), [](const SD_task_t& elm) { return not elm->marked; });
}

bool acyclic_graph_detail(const_xbt_dynar_t dag)
{
  unsigned int count;
  bool all_marked = true;
  SD_task_t task = nullptr;
  std::vector<SD_task_t> current;
  xbt_dynar_foreach (dag, count, task)
    if (task->kind != SD_TASK_COMM_E2E && task->successors->empty() && task->outputs->empty())
      current.push_back(task);

  while (not current.empty()) {
    std::vector<SD_task_t> next;
    for (auto const& t : current) {
      //Mark task
      t->marked = true;
      for (SD_task_t const& input : *t->inputs) {
        input->marked = true;
        // Inputs are communication, hence they can have only one predecessor
        SD_task_t input_pred = *(input->predecessors->begin());
        if (children_are_marked(input_pred))
          next.push_back(input_pred);
      }
      for (SD_task_t const& pred : *t->predecessors) {
        if (children_are_marked(pred))
          next.push_back(pred);
      }
    }
    current.clear();
    current = next;
  }

  all_marked = true;
  //test if all tasks are marked
  xbt_dynar_foreach(dag,count,task){
    if (task->kind != SD_TASK_COMM_E2E && not task->marked) {
      XBT_WARN("the task %s is not marked",task->name);
      all_marked = false;
      break;
    }
  }

  if (not all_marked) {
    XBT_VERB("there is at least one cycle in your task graph");
    xbt_dynar_foreach(dag,count,task){
      if(task->kind != SD_TASK_COMM_E2E && task->predecessors->empty() && task->inputs->empty()){
        task->marked = true;
        current.push_back(task);
      }
    }
    //test if something has to be done for the next iteration
    while (not current.empty()) {
      std::vector<SD_task_t> next;
      //test if the current iteration is done
      for (auto const& t : current) {
        t->marked = true;
        for (SD_task_t const& output : *t->outputs) {
          output->marked = true;
          // outputs are communication, hence they can have only one successor
          SD_task_t output_succ = *(output->successors->begin());
          if (parents_are_marked(output_succ))
            next.push_back(output_succ);
        }
        for (SD_task_t const& succ : *t->successors) {
          if (parents_are_marked(succ))
            next.push_back(succ);
        }
      }
      current.clear();
      current = next;
    }

    all_marked = true;
    xbt_dynar_foreach(dag,count,task){
      if (task->kind != SD_TASK_COMM_E2E && not task->marked) {
        XBT_WARN("the task %s is in a cycle",task->name);
        all_marked = false;
      }
    }
  }
  return all_marked;
}

static YY_BUFFER_STATE input_buffer;

static xbt_dynar_t result;
static std::map<std::string, SD_task_t> jobs;
static std::map<std::string, SD_task_t> files;
static SD_task_t current_job;

/** @brief loads a DAX file describing a DAG
 *
 * See https://confluence.pegasus.isi.edu/display/pegasus/WorkflowGenerator for more details.
 */
xbt_dynar_t SD_daxload(const char *filename)
{
  SD_task_t file;
  FILE* in_file = fopen(filename, "r");
  xbt_assert(in_file, "Unable to open \"%s\"\n", filename);
  input_buffer = dax__create_buffer(in_file, 10);
  dax__switch_to_buffer(input_buffer);
  dax_lineno = 1;

  result              = xbt_dynar_new(sizeof(SD_task_t), nullptr);
  SD_task_t root_task = SD_task_create_comp_seq("root", nullptr, 0);
  /* by design the root task is always SCHEDULABLE */
  SD_task_set_state(root_task, SD_SCHEDULABLE);

  xbt_dynar_push(result, &root_task);
  SD_task_t end_task = SD_task_create_comp_seq("end", nullptr, 0);

  int res = dax_lex();
  if (res != 0)
    xbt_die("Parse error in %s: %s", filename, dax__parse_err_msg());
  dax__delete_buffer(input_buffer);
  fclose(in_file);
  dax_lex_destroy();

  /* And now, post-process the files.
   * We want a file task per pair of computation tasks exchanging the file. Duplicate on need
   * Files not produced in the system are said to be produced by root task (top of DAG).
   * Files not consumed in the system are said to be consumed by end task (bottom of DAG).
   */

  for (auto const& elm : files) {
    file = elm.second;
    SD_task_t newfile;
    if (file->predecessors->empty()) {
      for (SD_task_t const& it : *file->successors) {
        newfile = SD_task_create_comm_e2e(file->name, nullptr, file->amount);
        SD_task_dependency_add(root_task, newfile);
        SD_task_dependency_add(newfile, it);
        xbt_dynar_push(result, &newfile);
      }
    }
    if (file->successors->empty()) {
      for (SD_task_t const& it : *file->predecessors) {
        newfile = SD_task_create_comm_e2e(file->name, nullptr, file->amount);
        SD_task_dependency_add(it, newfile);
        SD_task_dependency_add(newfile, end_task);
        xbt_dynar_push(result, &newfile);
      }
    }
    for (SD_task_t const& it : *file->predecessors) {
      for (SD_task_t const& it2 : *file->successors) {
        if (it == it2) {
          XBT_WARN("File %s is produced and consumed by task %s."
                   "This loop dependency will prevent the execution of the task.",
                   file->name, it->name);
        }
        newfile = SD_task_create_comm_e2e(file->name, nullptr, file->amount);
        SD_task_dependency_add(it, newfile);
        SD_task_dependency_add(newfile, it2);
        xbt_dynar_push(result, &newfile);
      }
    }
    /* Free previous copy of the files */
    SD_task_destroy(file);
  }

  /* Push end task last */
  xbt_dynar_push(result, &end_task);

  unsigned int cpt;
  xbt_dynar_foreach(result, cpt, file) {
    if (SD_task_get_kind(file) == SD_TASK_COMM_E2E) {
      uniq_transfer_task_name(file);
    } else {
      /* If some tasks do not take files as input, connect them to the root
       * if they don't produce files, connect them to the end node.
       */
      if ((file != root_task) && (file != end_task)) {
        if (file->inputs->empty())
          SD_task_dependency_add(root_task, file);
        if (file->outputs->empty())
          SD_task_dependency_add(file, end_task);
      }
    }
  }

  if (not acyclic_graph_detail(result)) {
    XBT_ERROR("The DAX described in %s is not a DAG. It contains a cycle.",
              simgrid::xbt::Path(filename).get_base_name().c_str());
    xbt_dynar_foreach(result, cpt, file)
      SD_task_destroy(file);
    xbt_dynar_free_container(&result);
    result = nullptr;
  }

  return result;
}

void STag_dax__adag()
{
  try {
    double version = std::stod(std::string(A_dax__adag_version));
    xbt_assert(version == 2.1, "Expected version 2.1 in <adag> tag, got %f. Fix the parser or your file", version);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Parse error: ") + A_dax__adag_version + " is not a double");
  }
}

void STag_dax__job()
{
  try {
    double runtime = std::stod(std::string(A_dax__job_runtime));

    std::string name = std::string(A_dax__job_id) + "@" + A_dax__job_name;
    runtime *= 4200000000.; /* Assume that timings were done on a 4.2GFlops machine. I mean, why not? */
    XBT_DEBUG("See <job id=%s runtime=%s %.0f>", A_dax__job_id, A_dax__job_runtime, runtime);
    current_job = SD_task_create_comp_seq(name.c_str(), nullptr, runtime);
    jobs.insert({A_dax__job_id, current_job});
    xbt_dynar_push(result, &current_job);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Parse error: ") + A_dax__job_runtime + " is not a double");
  }
}

void STag_dax__uses()
{
  double size;
  try {
    size = std::stod(std::string(A_dax__uses_size));
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Parse error: ") + A_dax__uses_size + " is not a double");
  }
  bool is_input = (A_dax__uses_link == A_dax__uses_link_input);

  XBT_DEBUG("See <uses file=%s %s>",A_dax__uses_file,(is_input?"in":"out"));
  auto it = files.find(A_dax__uses_file);
  SD_task_t file;
  if (it == files.end()) {
    file = SD_task_create_comm_e2e(A_dax__uses_file, nullptr, size);
    sd_global->initial_tasks.erase(file);
    files[A_dax__uses_file] = file;
  } else {
    file = it->second;
    if (file->amount < size || file->amount > size) {
      XBT_WARN("Ignore file %s size redefinition from %.0f to %.0f", A_dax__uses_file, SD_task_get_amount(file), size);
    }
  }
  if (is_input) {
    SD_task_dependency_add(file, current_job);
  } else {
    SD_task_dependency_add(current_job, file);
    if ((file->predecessors->size() + file->inputs->size()) > 1) {
      XBT_WARN("File %s created at more than one location...", file->name);
    }
  }
}

static SD_task_t current_child;
void STag_dax__child()
{
  auto job = jobs.find(A_dax__child_ref);
  if (job != jobs.end()) {
    current_child = job->second;
  } else {
    throw std::out_of_range(std::string("Parse error on line ") + std::to_string(dax_lineno) +
                            ": Asked to add dependencies to the non-existent " + A_dax__child_ref + "task");
  }
}

void ETag_dax__child()
{
  current_child = nullptr;
}

void STag_dax__parent()
{
  auto job = jobs.find(A_dax__parent_ref);
  if (job != jobs.end()) {
    SD_task_t parent = job->second;
    SD_task_dependency_add(parent, current_child);
    XBT_DEBUG("Control-flow dependency from %s to %s", current_child->name, parent->name);
  } else {
    throw std::out_of_range(std::string("Parse error on line ") + std::to_string(dax_lineno) +
                            ": Asked to add a dependency from " + current_child->name + " to " + A_dax__parent_ref +
                            ", but " + A_dax__parent_ref + " does not exist");
  }
}

void ETag_dax__adag()
{
  XBT_DEBUG("See </adag>");
}

void ETag_dax__job()
{
  current_job = nullptr;
  XBT_DEBUG("See </job>");
}

void ETag_dax__parent()
{
  XBT_DEBUG("See </parent>");
}

void ETag_dax__uses()
{
  XBT_DEBUG("See </uses>");
}
