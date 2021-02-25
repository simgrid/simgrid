/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/kernel/actor/ActorImpl.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/inspect/mc_unw.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_safety.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/remote/AppSide.hpp"
#include "src/mc/sosp/Snapshot.hpp"

#include <array>
#include <boost/core/demangle.hpp>
#include <libunwind.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc, "Logging specific to MC (global)");

namespace simgrid {
namespace mc {

std::vector<double> processes_time;

}
}

#if SIMGRID_HAVE_MC

/* Liveness */

namespace simgrid {
namespace mc {

xbt_automaton_t property_automaton = nullptr;

}
}

/* Dot output */
FILE *dot_output = nullptr;


/*******************************  Initialization of MC *******************************/
/*********************************************************************************/

void MC_init_dot_output()
{
  dot_output = fopen(_sg_mc_dot_output_file.get().c_str(), "w");

  if (dot_output == nullptr) {
    perror("Error open dot output file");
    xbt_abort();
  }

  fprintf(dot_output,
          "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node [fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");
}

/*******************************  Core of MC *******************************/
/**************************************************************************/

void MC_run()
{
  simgrid::mc::processes_time.resize(simgrid::kernel::actor::get_maxpid());
  MC_ignore_heap(simgrid::mc::processes_time.data(),
    simgrid::mc::processes_time.size() * sizeof(simgrid::mc::processes_time[0]));
  simgrid::mc::AppSide::get()->main_loop();
}

void MC_show_deadlock()
{
  XBT_INFO("**************************");
  XBT_INFO("*** DEADLOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Counter-example execution trace:");
  for (auto const& s : mc_model_checker->getChecker()->get_textual_trace())
    XBT_INFO("  %s", s.c_str());
  simgrid::mc::dumpRecordPath();
  simgrid::mc::session->log_state();
}

void MC_automaton_load(const char *file)
{
  if (simgrid::mc::property_automaton == nullptr)
    simgrid::mc::property_automaton = xbt_automaton_new();

  xbt_automaton_load(simgrid::mc::property_automaton, file);
}

namespace simgrid {
namespace mc {

void dumpStack(FILE* file, unw_cursor_t* cursor)
{
  int nframe = 0;
  std::array<char, 100> buffer;

  unw_word_t off;
  do {
    const char* name = not unw_get_proc_name(cursor, buffer.data(), buffer.size(), &off) ? buffer.data() : "?";
    // Unmangle C++ names:
    std::string realname = boost::core::demangle(name);

#if defined(__x86_64__)
    unw_word_t rip = 0;
    unw_word_t rsp = 0;
    unw_get_reg(cursor, UNW_X86_64_RIP, &rip);
    unw_get_reg(cursor, UNW_X86_64_RSP, &rsp);
    fprintf(file, "  %i: %s (RIP=0x%" PRIx64 " RSP=0x%" PRIx64 ")\n", nframe, realname.c_str(), (std::uint64_t)rip,
            (std::uint64_t)rsp);
#else
    fprintf(file, "  %i: %s\n", nframe, realname.c_str());
#endif

    ++nframe;
  } while (unw_step(cursor));
}

}
}
#endif

double MC_process_clock_get(const simgrid::kernel::actor::ActorImpl* process)
{
  if (simgrid::mc::processes_time.empty())
    return 0;
  if (process != nullptr)
    return simgrid::mc::processes_time[process->get_pid()];
  return -1;
}

void MC_process_clock_add(const simgrid::kernel::actor::ActorImpl* process, double amount)
{
  simgrid::mc::processes_time[process->get_pid()] += amount;
}
