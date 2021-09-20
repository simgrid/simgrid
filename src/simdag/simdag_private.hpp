/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/simdag.h"
#include "surf/surf.hpp"
#include <set>
#include <string>
#include <vector>

#ifndef SIMDAG_PRIVATE_HPP
#define SIMDAG_PRIVATE_HPP
#if SIMGRID_HAVE_JEDULE
#include "simgrid/jedule/jedule_sd_binding.h"
#endif

namespace simgrid{
namespace sd{
class Global {
public:
  explicit Global(int* argc, char** argv) : engine_(new simgrid::s4u::Engine(argc, argv)) {}
  bool watch_point_reached = false; /* has a task just reached a watch point? */
  std::set<SD_task_t> initial_tasks;
  std::set<SD_task_t> runnable_tasks;
  std::set<SD_task_t> completed_tasks;
  std::set<SD_task_t> return_set;
  s4u::Engine* engine_;
};

std::set<SD_task_t>* simulate (double how_long);
}
}

extern XBT_PRIVATE simgrid::sd::Global *sd_global;

/* Task */
struct s_SD_task_t {
  e_SD_task_state_t state;
  void *data;                   /* user data */
  char *name;
  e_SD_task_kind_t kind;
  double amount;
  double alpha;          /* used by typed parallel tasks */
  double start_time;
  double finish_time;
  simgrid::kernel::resource::Action* surf_action;
  unsigned short watch_points;  /* bit field xor()ed with masks */

  bool marked = false; /* used to check if the task DAG has some cycle*/

  /* dependencies -- cannot be embedded in the struct since it's not handled as a real C++ class */
  std::set<SD_task_t> *inputs;
  std::set<SD_task_t> *outputs;
  std::set<SD_task_t> *predecessors;
  std::set<SD_task_t> *successors;

  /* scheduling parameters (only exist in state SD_SCHEDULED) */
  std::vector<sg_host_t> *allocation;
  double *flops_amount;
  double *bytes_amount;
  double rate;
};

/* SimDag private functions */
XBT_PRIVATE void SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state);
XBT_PRIVATE void SD_task_run(SD_task_t task);
XBT_PRIVATE bool acyclic_graph_detail(const_xbt_dynar_t dag);
XBT_PRIVATE void uniq_transfer_task_name(SD_task_t task);
XBT_PRIVATE const char *__get_state_name(e_SD_task_state_t state);
#endif
