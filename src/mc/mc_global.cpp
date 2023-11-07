/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc.h"

#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc, "Logging specific to MC (global)");

namespace simgrid::mc {

std::vector<double> processes_time;

}

double MC_process_clock_get(const simgrid::kernel::actor::ActorImpl* process)
{
  if (process) {
    auto pid = static_cast<size_t>(process->get_pid());
    if (pid < simgrid::mc::processes_time.size())
      return simgrid::mc::processes_time[pid];
  }
  return 0.0;
}

void MC_process_clock_add(const simgrid::kernel::actor::ActorImpl* process, double amount)
{
  simgrid::mc::processes_time.at(process->get_pid()) += amount;
}
