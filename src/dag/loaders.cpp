/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include <algorithm>
#include <map>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <stdexcept>
#include <xbt/asserts.h>
#include <xbt/file.hpp>
#include <xbt/log.h>
#include <xbt/misc.h>

#include "dax_dtd.h"
#include "dax_dtd.c"

#if HAVE_GRAPHVIZ
#include <graphviz/cgraph.h>
#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_parsing, "Generation DAGs from files");

/* Ensure that transfer tasks have unique names even though a file is used several times */
static void uniq_transfer_task_name(simgrid::s4u::Comm* comm)
{
  const auto& child  = comm->get_successors().front();
  const auto& parent = *(comm->get_dependencies().begin());

  std::string new_name = parent->get_name() + "_" + comm->get_name() + "_" + child->get_name();

  comm->set_name(new_name)->vetoable_start();
}

static bool check_for_cycle(const std::vector<simgrid::s4u::ActivityPtr>& dag)
{
  std::vector<simgrid::s4u::ActivityPtr> current;

  for (const auto& a : dag)
    if (dynamic_cast<simgrid::s4u::Exec*>(a.get()) != nullptr && a->is_waited_by() == 0)
      current.push_back(a);

  while (not current.empty()) {
    std::vector<simgrid::s4u::ActivityPtr> next;
    for (auto const& a : current) {
      a->mark();
      for (auto const& pred : a->get_dependencies()) {
        if (dynamic_cast<simgrid::s4u::Comm*>(pred.get()) != nullptr) {
          pred->mark();
          // Comms have only one predecessor
          auto pred_pred = *(pred->get_dependencies().begin());
          if (std::none_of(pred_pred->get_successors().begin(), pred_pred->get_successors().end(),
                           [](const simgrid::s4u::ActivityPtr& act) { return not act->is_marked(); }))
            next.push_back(pred_pred);
        } else {
          if (std::none_of(pred->get_successors().begin(), pred->get_successors().end(),
                           [](const simgrid::s4u::ActivityPtr& act) { return not act->is_marked(); }))
            next.push_back(pred);
        }
      }
    }
    current.clear();
    current = next;
  }

  return not std::any_of(dag.begin(), dag.end(), [](const simgrid::s4u::ActivityPtr& a) { return not a->is_marked(); });
}

static YY_BUFFER_STATE input_buffer;

namespace simgrid {
namespace s4u {

static std::vector<ActivityPtr> result;
static std::map<std::string, ExecPtr, std::less<>> jobs;
static std::map<std::string, Comm*, std::less<>> files;
static ExecPtr current_job;

/** @brief loads a DAX file describing a DAG
 *
 * See https://confluence.pegasus.isi.edu/display/pegasus/WorkflowGenerator for more details.
 */
std::vector<ActivityPtr> create_DAG_from_DAX(const std::string& filename)
{
  FILE* in_file = fopen(filename.c_str(), "r");
  xbt_assert(in_file, "Unable to open \"%s\"\n", filename.c_str());
  input_buffer = dax__create_buffer(in_file, 10);
  dax__switch_to_buffer(input_buffer);
  dax_lineno = 1;

  auto root_task = Exec::init()->set_name("root")->set_flops_amount(0);
  root_task->vetoable_start();

  result.push_back(root_task);

  auto end_task = Exec::init()->set_name("end")->set_flops_amount(0);
  end_task->vetoable_start();

  xbt_assert(dax_lex() == 0, "Parse error in %s: %s", filename.c_str(), dax__parse_err_msg());
  dax__delete_buffer(input_buffer);
  fclose(in_file);
  dax_lex_destroy();

  /* And now, post-process the files.
   * We want a file task per pair of computation tasks exchanging the file. Duplicate on need
   * Files not produced in the system are said to be produced by root task (top of DAG).
   * Files not consumed in the system are said to be consumed by end task (bottom of DAG).
   */
  CommPtr file;

  for (auto const& elm : files) {
    file = elm.second;
    CommPtr newfile;
    if (file->dependencies_solved()) {
      for (auto const& it : file->get_successors()) {
        newfile = Comm::sendto_init()->set_name(file->get_name())->set_payload_size(file->get_remaining());
        root_task->add_successor(newfile);
        newfile->add_successor(it);
        result.push_back(newfile);
      }
    }
    if (file->is_waited_by() == 0) {
      for (auto const& it : file->get_dependencies()) {
        newfile = Comm::sendto_init()->set_name(file->get_name())->set_payload_size(file->get_remaining());
        it->add_successor(newfile);
        newfile->add_successor(end_task);
        result.push_back(newfile);
      }
    }
    for (auto const& it : file->get_dependencies()) {
      for (auto const& it2 : file->get_successors()) {
        if (it == it2) {
          XBT_WARN("File %s is produced and consumed by task %s."
                   "This loop dependency will prevent the execution of the task.",
                   file->get_cname(), it->get_cname());
        }
        newfile = Comm::sendto_init()->set_name(file->get_name())->set_payload_size(file->get_remaining());
        it->add_successor(newfile);
        newfile->add_successor(it2);
        result.push_back(newfile);
      }
    }
    /* Free previous copy of the files */
    file->destroy();
  }

  /* Push end task last */
  result.push_back(end_task);

  for (const auto& a : result) {
    auto* comm = dynamic_cast<Comm*>(a.get());
    if (comm != nullptr) {
      uniq_transfer_task_name(comm);
    } else {
      /* If some tasks do not take files as input, connect them to the root
       * if they don't produce files, connect them to the end node.
       */
      if ((a != root_task) && (a != end_task)) {
        if (a->dependencies_solved())
          root_task->add_successor(a);
        if (a->is_waited_by() == 0)
          a->add_successor(end_task);
      }
    }
  }

  if (not check_for_cycle(result)) {
    XBT_ERROR("The DAX described in %s is not a DAG. It contains a cycle.",
              simgrid::xbt::Path(filename).get_base_name().c_str());
    for (const auto& a : result)
      a->destroy();
    result.clear();
  }

  return result;
}

#if HAVE_GRAPHVIZ
std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename)
{
  FILE* in_file = fopen(filename.c_str(), "r");
  xbt_assert(in_file != nullptr, "Failed to open file: %s", filename.c_str());

  Agraph_t* dag_dot = agread(in_file, NIL(Agdisc_t*));

  std::unordered_map<std::string, ActivityPtr> activities;
  std::vector<ActivityPtr> dag;

  ActivityPtr root;
  ActivityPtr end;
  ActivityPtr act;
  /* Create all the nodes */
  Agnode_t* node = nullptr;
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    char* name    = agnameof(node);
    double amount = atof(agget(node, (char*)"size"));

    if (activities.find(name) == activities.end()) {
      XBT_DEBUG("See <Exec id = %s amount = %.0f>", name, amount);
      act = Exec::init()->set_name(name)->set_flops_amount(amount)->vetoable_start();
      activities.insert({std::string(name), act});
      if (strcmp(name, "root") && strcmp(name, "end"))
        dag.push_back(act);
    } else {
      XBT_WARN("Exec '%s' is defined more than once", name);
    }
  }
  /*Check if 'root' and 'end' nodes have been explicitly declared.  If not, create them. */
  if (activities.find("root") == activities.end())
    root = Exec::init()->set_name("root")->set_flops_amount(0)->vetoable_start();
  else
    root = activities.at("root");

  if (activities.find("end") == activities.end())
    end = Exec::init()->set_name("end")->set_flops_amount(0)->vetoable_start();
  else
    end = activities.at("end");

  /* Create edges */
  std::vector<Agedge_t*> edges;
  for (node = agfstnode(dag_dot); node; node = agnxtnode(dag_dot, node)) {
    edges.clear();
    for (Agedge_t* edge = agfstout(dag_dot, node); edge; edge = agnxtout(dag_dot, edge))
      edges.push_back(edge);

    /* Be sure edges are sorted */
    std::sort(edges.begin(), edges.end(), [](const Agedge_t* a, const Agedge_t* b) { return AGSEQ(a) < AGSEQ(b); });

    for (Agedge_t* edge : edges) {
      const char* src_name = agnameof(agtail(edge));
      const char* dst_name = agnameof(aghead(edge));
      double size          = atof(agget(edge, (char*)"size"));

      ActivityPtr src = activities.at(src_name);
      ActivityPtr dst = activities.at(dst_name);
      if (size > 0) {
        std::string name = std::string(src_name) + "->" + dst_name;
        XBT_DEBUG("See <Comm id=%s amount = %.0f>", name.c_str(), size);
        if (activities.find(name) == activities.end()) {
          act = Comm::sendto_init()->set_name(name)->set_payload_size(size)->vetoable_start();
          src->add_successor(act);
          act->add_successor(dst);
          activities.insert({name, act});
          dag.push_back(act);
        } else {
          XBT_WARN("Comm '%s' is defined more than once", name.c_str());
        }
      } else {
        src->add_successor(dst);
      }
    }
  }

  XBT_DEBUG("All activities have been created, put %s at the beginning and %s at the end", root->get_cname(),
            end->get_cname());
  dag.insert(dag.begin(), root);
  dag.push_back(end);

  /* Connect entry tasks to 'root', and exit tasks to 'end'*/
  for (const auto& a : dag) {
    if (a->dependencies_solved() && a != root) {
      XBT_DEBUG("Activity '%s' has no dependencies. Add dependency from 'root'", a->get_cname());
      root->add_successor(a);
    }

    if (a->is_waited_by() == 0 && a != end) {
      XBT_DEBUG("Activity '%s' has no successors. Add dependency to 'end'", a->get_cname());
      a->add_successor(end);
    }
  }
  agclose(dag_dot);
  fclose(in_file);

  if (not check_for_cycle(dag)) {
    std::string base = simgrid::xbt::Path(filename).get_base_name();
    XBT_ERROR("The DOT described in %s is not a DAG. It contains a cycle.", base.c_str());
    for (const auto& a : dag)
      a->destroy();
    dag.clear();
    dag.shrink_to_fit();
  }

  return dag;
}
#else
std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename)
{
  xbt_die("create_DAG_from_dot() is not usable because graphviz was not found.\n"
          "Please install graphviz, graphviz-dev, and libgraphviz-dev (and erase CMakeCache.txt) before recompiling.");
}
#endif
} // namespace s4u
} // namespace simgrid

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
    simgrid::s4u::current_job = simgrid::s4u::Exec::init()->set_name(name)->set_flops_amount(runtime)->vetoable_start();
    simgrid::s4u::jobs.insert({A_dax__job_id, simgrid::s4u::current_job});
    simgrid::s4u::result.push_back(simgrid::s4u::current_job);
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

  XBT_DEBUG("See <uses file=%s %s>", A_dax__uses_file, (is_input ? "in" : "out"));
  auto it = simgrid::s4u::files.find(A_dax__uses_file);
  simgrid::s4u::CommPtr file;
  if (it == simgrid::s4u::files.end()) {
    file = simgrid::s4u::Comm::sendto_init()->set_name(A_dax__uses_file)->set_payload_size(size);
    simgrid::s4u::files[A_dax__uses_file] = file.get();
  } else {
    file = it->second;
    if (file->get_remaining() < size || file->get_remaining() > size) {
      XBT_WARN("Ignore file %s size redefinition from %.0f to %.0f", A_dax__uses_file, file->get_remaining(), size);
    }
  }
  if (is_input) {
    file->add_successor(simgrid::s4u::current_job);
  } else {
    simgrid::s4u::current_job->add_successor(file);
    if (file->get_dependencies().size() > 1) {
      XBT_WARN("File %s created at more than one location...", file->get_cname());
    }
  }
}

static simgrid::s4u::ExecPtr current_child;
void STag_dax__child()
{
  auto job = simgrid::s4u::jobs.find(A_dax__child_ref);
  if (job != simgrid::s4u::jobs.end()) {
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
  auto job = simgrid::s4u::jobs.find(A_dax__parent_ref);
  if (job != simgrid::s4u::jobs.end()) {
    auto parent = job->second;
    parent->add_successor(current_child);
    XBT_DEBUG("Control-flow dependency from %s to %s", current_child->get_cname(), parent->get_cname());
  } else {
    throw std::out_of_range(std::string("Parse error on line ") + std::to_string(dax_lineno) +
                            ": Asked to add a dependency from " + current_child->get_name() + " to " +
                            A_dax__parent_ref + ", but " + A_dax__parent_ref + " does not exist");
  }
}

void ETag_dax__adag()
{
  XBT_DEBUG("See </adag>");
}

void ETag_dax__job()
{
  simgrid::s4u::current_job = nullptr;
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
