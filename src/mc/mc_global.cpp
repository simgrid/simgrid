/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <cxxabi.h>

#include <vector>

#include <xbt/dynar.h>
#include <xbt/automaton.h>
#include <xbt/swag.h>

#include "mc_base.h"

#include "mc/mc.h"

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#include "src/simix/smx_process_private.h"

#if HAVE_MC
#include <libunwind.h>
#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_request.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_snapshot.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_unw.h"
#include "src/mc/mc_smx.h"
#include "src/mc/Checker.hpp"
#endif

#include "src/mc/mc_record.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/Client.hpp"
#include "src/mc/Transition.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc, "Logging specific to MC (global)");

e_mc_mode_t mc_mode;

namespace simgrid {
namespace mc {

std::vector<double> processes_time;

}
}

#if HAVE_MC

/* MC global data structures */
simgrid::mc::State* mc_current_state = nullptr;
char mc_replay_mode = false;

/* Liveness */

namespace simgrid {
namespace mc {

std::unique_ptr<s_mc_global_t> initial_global_state = nullptr;
xbt_automaton_t property_automaton = nullptr;

}
}

/* Dot output */
FILE *dot_output = nullptr;


/*******************************  Initialisation of MC *******************************/
/*********************************************************************************/

void MC_init_dot_output()
{
  dot_output = fopen(_sg_mc_dot_output_file, "w");

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
  simgrid::mc::processes_time.resize(simix_process_maxpid);
  MC_ignore_heap(simgrid::mc::processes_time.data(),
    simgrid::mc::processes_time.size() * sizeof(simgrid::mc::processes_time[0]));
  smx_process_t process;
  xbt_swag_foreach(process, simix_global->process_list)
    MC_ignore_heap(&(process->process_hookup), sizeof(process->process_hookup));
  simgrid::mc::Client::get()->mainLoop();
  simgrid::mc::processes_time.clear();
}

namespace simgrid {
namespace mc {

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void replay(std::list<std::unique_ptr<simgrid::mc::State>> const& stack)
{
  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0 || _sg_mc_termination || _sg_mc_visited > 0) {
    simgrid::mc::State* state = stack.back().get();
    if (state->system_state) {
      simgrid::mc::restore_snapshot(state->system_state);
      if(_sg_mc_comms_determinism || _sg_mc_send_determinism) 
        MC_restore_communications_pattern(state);
      return;
    }
  }


  /* Restore the initial state */
  simgrid::mc::restore_snapshot(simgrid::mc::initial_global_state->snapshot);

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    // int n = xbt_dynar_length(incomplete_communications_pattern);
    unsigned n = MC_smx_get_maxpid();
    assert(n == xbt_dynar_length(incomplete_communications_pattern));
    assert(n == xbt_dynar_length(initial_communications_pattern));
    for (unsigned j=0; j < n ; j++) {
      xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, j, xbt_dynar_t));
      xbt_dynar_get_as(initial_communications_pattern, j, simgrid::mc::PatternCommunicationList*)->index_comm = 0;
    }
  }

  int count = 1;

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack) {
    if (state == stack.back())
      break;

    int req_num = state->transition.argument;
    smx_simcall_t saved_req = &state->executed_req;
    
    if (saved_req) {
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */

      const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
      smx_simcall_t req = &issuer->simcall;

      /* Debug information */
      XBT_DEBUG("Replay: %s (%p)",
        simgrid::mc::request_to_string(
          req, req_num, simgrid::mc::RequestType::simix).c_str(),
        state.get());

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      mc_model_checker->handle_simcall(state->transition);
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        MC_handle_comm_pattern(call, req, req_num, nullptr, 1);
      mc_model_checker->wait_for_requests();

      count++;
    }

    /* Update statistics */
    mc_model_checker->visited_states++;
    mc_model_checker->executed_transitions++;

  }

  XBT_DEBUG("**** End Replay ****");
}

}
}

void MC_show_deadlock(void)
{
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Counter-example execution trace:");
  for (auto& s : mc_model_checker->getChecker()->getTextualTrace())
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
    const char * name = !unw_get_proc_name(&cursor, buffer, 100, &off) ? buffer : "?";

    int status;

    // Demangle C++ names:
    char* realname = abi::__cxa_demangle(name, 0, 0, &status);

#if defined(__x86_64__)
    unw_word_t rip = 0;
    unw_word_t rsp = 0;
    unw_get_reg(&cursor, UNW_X86_64_RIP, &rip);
    unw_get_reg(&cursor, UNW_X86_64_RSP, &rsp);
    fprintf(file, "  %i: %s (RIP=0x%" PRIx64 " RSP=0x%" PRIx64 ")\n",
      nframe, realname ? realname : name, (std::uint64_t) rip, (std::uint64_t) rsp);
#else
    fprintf(file, "  %i: %s\n", nframe, realname ? realname : name);
#endif

    free(realname);
    ++nframe;
  } while(unw_step(&cursor));
}

}
}

static void MC_dump_stacks(FILE* file)
{
  int nstack = 0;
  for (auto const& stack : mc_model_checker->process().stack_areas()) {
    fprintf(file, "Stack %i:\n", nstack++);

    simgrid::mc::UnwindContext context;
    unw_context_t raw_context =
      mc_model_checker->process().read<unw_context_t>(
        simgrid::mc::remote((unw_context_t *)stack.context));
    context.initialize(&mc_model_checker->process(), &raw_context);

    unw_cursor_t cursor = context.cursor();
    simgrid::mc::dumpStack(file, cursor);
  }
}
#endif

double MC_process_clock_get(smx_process_t process)
{
  if (simgrid::mc::processes_time.empty())
    return 0;
  if (process != nullptr)
    return simgrid::mc::processes_time[process->pid];
  return -1;
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  simgrid::mc::processes_time[process->pid] += amount;
}
