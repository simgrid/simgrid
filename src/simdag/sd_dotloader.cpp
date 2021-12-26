/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simdag_private.hpp"
#include "simgrid/s4u/Activity.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/internal_config.h"
#include "xbt/file.hpp"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

#if HAVE_GRAPHVIZ
#include <graphviz/cgraph.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_dotparse, sd, "Parsing DOT files");

namespace simgrid {
namespace s4u {

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
      a->cancel();
    dag.clear();
  }

  return dag;
}

} // namespace s4u
} // namespace simgrid

#else
namespace simgrid {
namespace s4u {
std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename)
{
  xbt_die("create_DAG_from_dot() is not usable because graphviz was not found.\n"
          "Please install graphviz, graphviz-dev, and libgraphviz-dev (and erase CMakeCache.txt) before recompiling.");
}
} // namespace s4u
} // namespace simgrid

#endif
