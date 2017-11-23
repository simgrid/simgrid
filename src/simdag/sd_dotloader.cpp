/* Copyright (c) 2009-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simdag_private.hpp"
#include "simgrid/simdag.h"
#include "src/internal_config.h"
#include "xbt/file.hpp"
#include <cstring>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_dotparse, sd, "Parsing DOT files");

#if HAVE_GRAPHVIZ
#include <graphviz/cgraph.h>

xbt_dynar_t SD_dotload_generic(const char* filename, bool sequential, bool schedule);

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
  return SD_dotload_generic(filename, true, false);
}

xbt_dynar_t SD_PTG_dotload(const char * filename) {
  return SD_dotload_generic(filename, false, false);
}

xbt_dynar_t SD_dotload_with_sched(const char *filename) {
  return SD_dotload_generic(filename, true, true);
}

static int edge_compare(const void *a, const void *b)
{
  unsigned va = AGSEQ(*(Agedge_t **)a);
  unsigned vb = AGSEQ(*(Agedge_t **)b);
  if (va == vb)
    return 0;
  else
    return (va < vb ? -1 : 1);
}

xbt_dynar_t SD_dotload_generic(const char* filename, bool sequential, bool schedule)
{
  xbt_assert(filename, "Unable to use a null file descriptor\n");
  FILE *in_file = fopen(filename, "r");
  xbt_assert(in_file != nullptr, "Failed to open file: %s", filename);

  unsigned int i;
  SD_task_t root;
  SD_task_t end;
  SD_task_t task;
  std::vector<SD_task_t>* computer;
  std::unordered_map<std::string, std::vector<SD_task_t>*> computers;
  bool schedule_success = true;

  std::unordered_map<std::string, SD_task_t> jobs;
  xbt_dynar_t result = xbt_dynar_new(sizeof(SD_task_t), dot_task_p_free);

  Agraph_t * dag_dot = agread(in_file, NIL(Agdisc_t *));

  /* Create all the nodes */
  Agnode_t *node = nullptr;
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    char *name = agnameof(node);
    double amount = atof(agget(node, (char*)"size"));
    if (jobs.find(name) == jobs.end()) {
      if (sequential) {
        XBT_DEBUG("See <job id=%s amount =%.0f>", name, amount);
        task = SD_task_create_comp_seq(name, nullptr , amount);
      } else {
        double alpha = atof(agget(node, (char *) "alpha"));
        XBT_DEBUG("See <job id=%s amount =%.0f alpha = %.3f>", name, amount, alpha);
        task = SD_task_create_comp_par_amdahl(name, nullptr , amount, alpha);
      }

      jobs.insert({std::string(name), task});

      if (strcmp(name,"root") && strcmp(name,"end"))
        xbt_dynar_push(result, &task);

      if ((sequential) &&
          ((schedule && schedule_success) || XBT_LOG_ISENABLED(sd_dotparse, xbt_log_priority_verbose))) {
        /* try to take the information to schedule the task only if all is right*/
        char *char_performer = agget(node, (char *) "performer");
        char *char_order = agget(node, (char *) "order");
        /* Tasks will execute on in a given "order" on a given set of "performer" hosts */
        int performer = ((not char_performer || not strcmp(char_performer, "")) ? -1 : atoi(char_performer));
        int order     = ((not char_order || not strcmp(char_order, "")) ? -1 : atoi(char_order));

        if ((performer != -1 && order != -1) && performer < static_cast<int>(sg_host_count())) {
          /* required parameters are given and less performers than hosts are required */
          XBT_DEBUG ("Task '%s' is scheduled on workstation '%d' in position '%d'", task->name, performer, order);
          auto comp = computers.find(char_performer);
          if (comp != computers.end()) {
            computer = comp->second;
          } else {
            computer = new std::vector<SD_task_t>;
            computers.insert({char_performer, computer});
          }
          if (static_cast<unsigned int>(order) < computer->size()) {
            SD_task_t task_test = computer->at(order);
            if (task_test && task_test != task) {
              /* the user gave the same order to several tasks */
              schedule_success = false;
              XBT_VERB("Task '%s' wants to start on performer '%s' at the same position '%s' as task '%s'",
                       task_test->name, char_performer, char_order, task->name);
              continue;
            }
          } else
            computer->resize(order);

          computer->insert(computer->begin() + order, task);
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
  if (jobs.find("root") == jobs.end())
    root = (sequential ? SD_task_create_comp_seq("root", nullptr, 0)
                       : SD_task_create_comp_par_amdahl("root", nullptr, 0, 0));
  else
    root = jobs.at("root");

  SD_task_set_state(root, SD_SCHEDULABLE);   /* by design the root task is always SCHEDULABLE */
  xbt_dynar_insert_at(result, 0, &root);     /* Put it at the beginning of the dynar */

  if (jobs.find("end") == jobs.end())
    end = (sequential ? SD_task_create_comp_seq("end", nullptr, 0)
                      : SD_task_create_comp_par_amdahl("end", nullptr, 0, 0));
  else
    end = jobs.at("end");

  /* Create edges */
  xbt_dynar_t edges = xbt_dynar_new(sizeof(Agedge_t*), nullptr);
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    Agedge_t * edge;
    xbt_dynar_reset(edges);
    for (edge = agfstout(dag_dot, node); edge; edge = agnxtout(dag_dot, edge))
      xbt_dynar_push_as(edges, Agedge_t *, edge);

    /* Be sure edges are sorted */
    xbt_dynar_sort(edges, edge_compare);

    xbt_dynar_foreach(edges, i, edge) {
      char *src_name=agnameof(agtail(edge));
      char *dst_name=agnameof(aghead(edge));
      double size = atof(agget(edge, (char *) "size"));

      SD_task_t src = jobs.at(src_name);
      SD_task_t dst = jobs.at(dst_name);

      if (size > 0) {
        std::string name = std::string(src_name) + "->" + dst_name;
        XBT_DEBUG("See <transfer id=%s amount = %.0f>", name.c_str(), size);
        if (jobs.find(name) == jobs.end()) {
          if (sequential)
            task = SD_task_create_comm_e2e(name.c_str(), nullptr, size);
          else
            task = SD_task_create_comm_par_mxn_1d_block(name.c_str(), nullptr, size);
          SD_task_dependency_add(nullptr, nullptr, src, task);
          SD_task_dependency_add(nullptr, nullptr, task, dst);
          jobs.insert({name, task});
          xbt_dynar_push(result, &task);
        } else {
          XBT_WARN("Task '%s' is defined more than once", name.c_str());
        }
      } else {
        SD_task_dependency_add(nullptr, nullptr, src, dst);
      }
    }
  }
  xbt_dynar_free(&edges);

  XBT_DEBUG("All tasks have been created, put %s at the end of the dynar", end->name);
  xbt_dynar_push(result, &end);

  /* Connect entry tasks to 'root', and exit tasks to 'end'*/
  xbt_dynar_foreach (result, i, task){
    if (task->predecessors->empty() && task->inputs->empty() && task != root) {
      XBT_DEBUG("Task '%s' has no source. Add dependency from 'root'", task->name);
      SD_task_dependency_add(nullptr, nullptr, root, task);
    }

    if (task->successors->empty() && task->outputs->empty() && task != end) {
      XBT_DEBUG("Task '%s' has no destination. Add dependency to 'end'", task->name);
      SD_task_dependency_add(nullptr, nullptr, task, end);
    }
  }

  agclose(dag_dot);
  fclose(in_file);

  if(schedule){
    if (schedule_success) {
      sg_host_t* workstations = sg_host_list();
      for (auto const& elm : computers) {
        SD_task_t previous_task = nullptr;
        for (auto const& task : *elm.second) {
          /* add dependency between the previous and the task to avoid parallel execution */
          if(task){
            if (previous_task && not SD_task_dependency_exists(previous_task, task))
              SD_task_dependency_add(nullptr, nullptr, previous_task, task);

            SD_task_schedulel(task, 1, workstations[atoi(elm.first.c_str())]);
            previous_task = task;
          }
        }
        delete elm.second;
      }
      xbt_free(workstations);
    } else {
      XBT_WARN("The scheduling is ignored");
      for (auto const& elm : computers)
        delete elm.second;
      xbt_dynar_free(&result);
      result = nullptr;
    }
  }

  if (result && not acyclic_graph_detail(result)) {
    std::string base = simgrid::xbt::Path(filename).getBasename();
    XBT_ERROR("The DOT described in %s is not a DAG. It contains a cycle.", base.c_str());
    xbt_dynar_free(&result);
    result = nullptr;
  }
  return result;
}
#else
xbt_dynar_t SD_dotload(const char *filename) {
  xbt_die("SD_dotload_generic() is not usable because graphviz was not found.\n"
      "Please install graphviz, graphviz-dev, and libgraphviz-dev (and erase CMakeCache.txt) before recompiling.");
}
xbt_dynar_t SD_dotload_with_sched(const char *filename) {
  return SD_dotload(filename);
}
xbt_dynar_t SD_PTG_dotload(const char * filename) {
  return SD_dotload(filename);
}
#endif
