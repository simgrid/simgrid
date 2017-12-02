/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <cxxabi.h>

#include <vector>

#include "xbt/automaton.h"
#include "xbt/backtrace.hpp"
#include "xbt/dynar.h"

#include "mc_base.h"

#include "mc/mc.h"

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#include "src/simix/ActorImpl.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_safety.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_snapshot.hpp"
#include "src/mc/mc_unw.hpp"
#include <libunwind.h>
#endif

#include "src/mc/Transition.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/remote/Client.hpp"
#include "src/mc/remote/mc_protocol.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc, "Logging specific to MC (global)");

extern std::string _sg_mc_dot_output_file;

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
  dot_output = fopen(_sg_mc_dot_output_file.c_str(), "w");

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
  simgrid::mc::processes_time.resize(SIMIX_process_get_maxpid());
  MC_ignore_heap(simgrid::mc::processes_time.data(),
    simgrid::mc::processes_time.size() * sizeof(simgrid::mc::processes_time[0]));
  simgrid::mc::Client::get()->mainLoop();
  simgrid::mc::processes_time.clear();
}

void MC_show_deadlock()
{
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Counter-example execution trace:");
  for (auto const& s : mc_model_checker->getChecker()->getTextualTrace())
    XBT_INFO("%s", s.c_str());
  simgrid::mc::session->logState();
}

void MC_automaton_load(const char *file)
{
  if (simgrid::mc::property_automaton == nullptr)
    simgrid::mc::property_automaton = xbt_automaton_new();

  xbt_automaton_load(simgrid::mc::property_automaton, file);
}

namespace simgrid {
namespace mc {

void dumpStack(FILE* file, unw_cursor_t cursor)
{
  int nframe = 0;
  char buffer[100];

  unw_word_t off;
  do {
    const char* name = not unw_get_proc_name(&cursor, buffer, 100, &off) ? buffer : "?";
    // Unmangle C++ names:
    auto realname = simgrid::xbt::demangle(name);

#if defined(__x86_64__)
    unw_word_t rip = 0;
    unw_word_t rsp = 0;
    unw_get_reg(&cursor, UNW_X86_64_RIP, &rip);
    unw_get_reg(&cursor, UNW_X86_64_RSP, &rsp);
    fprintf(file, "  %i: %s (RIP=0x%" PRIx64 " RSP=0x%" PRIx64 ")\n", nframe, realname.get(), (std::uint64_t)rip,
            (std::uint64_t)rsp);
#else
    fprintf(file, "  %i: %s\n", nframe, realname.get());
#endif

    ++nframe;
  } while(unw_step(&cursor));
}

}
}

static void MC_dump_stacks(FILE* file)
{
  int nstack = 0;
  for (auto const& stack : mc_model_checker->process().stack_areas()) {
    fprintf(file, "Stack %i:\n", nstack);
    nstack++;

    simgrid::mc::UnwindContext context;
    unw_context_t raw_context =
      (unw_context_t) mc_model_checker->process().read<unw_context_t>(
        simgrid::mc::remote((unw_context_t *)stack.context));
    context.initialize(&mc_model_checker->process(), &raw_context);

    unw_cursor_t cursor = context.cursor();
    simgrid::mc::dumpStack(file, cursor);
  }
}
#endif

double MC_process_clock_get(smx_actor_t process)
{
  if (simgrid::mc::processes_time.empty())
    return 0;
  if (process != nullptr)
    return simgrid::mc::processes_time[process->pid];
  return -1;
}

void MC_process_clock_add(smx_actor_t process, double amount)
{
  simgrid::mc::processes_time[process->pid] += amount;
}
