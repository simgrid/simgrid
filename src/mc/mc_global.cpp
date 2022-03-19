/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/kernel/actor/ActorImpl.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/Session.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/inspect/mc_unw.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_safety.hpp"
#include "src/mc/remote/AppSide.hpp"
#include "src/mc/sosp/Snapshot.hpp"

#include <array>
#include <boost/core/demangle.hpp>
#include <cerrno>
#include <cstring>
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

/* Dot output */
FILE *dot_output = nullptr;

void MC_init_dot_output()
{
  dot_output = fopen(_sg_mc_dot_output_file.get().c_str(), "w");
  xbt_assert(dot_output != nullptr, "Error open dot output file: %s", strerror(errno));

  fprintf(dot_output,
          "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node [fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");
}


namespace simgrid {
namespace mc {

/* Liveness */
xbt_automaton_t property_automaton = nullptr;

/*******************************  Core of MC *******************************/
/**************************************************************************/
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
  if (process == nullptr)
    return -1;
  return simgrid::mc::processes_time.at(process->get_pid());
}

void MC_process_clock_add(const simgrid::kernel::actor::ActorImpl* process, double amount)
{
  simgrid::mc::processes_time.at(process->get_pid()) += amount;
}
