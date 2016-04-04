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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc, "Logging specific to MC (global)");

e_mc_mode_t mc_mode;

namespace simgrid {
namespace mc {

std::vector<double> processes_time;

}
}

#if HAVE_MC
int user_max_depth_reached = 0;

/* MC global data structures */
simgrid::mc::State* mc_current_state = nullptr;
char mc_replay_mode = false;

mc_stats_t mc_stats = nullptr;

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
  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    // int n = xbt_dynar_length(incomplete_communications_pattern);
    unsigned n = MC_smx_get_maxpid();
    assert(n == xbt_dynar_length(incomplete_communications_pattern));
    assert(n == xbt_dynar_length(initial_communications_pattern));
    for (unsigned j=0; j < n ; j++) {
      xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, j, xbt_dynar_t));
      xbt_dynar_get_as(initial_communications_pattern, j, mc_list_comm_pattern_t)->index_comm = 0;
    }
  }

  int count = 1;

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack) {
    if (state == stack.back())
      break;

    int value;
    smx_simcall_t saved_req = MC_state_get_executed_request(state.get(), &value);
    
    if (saved_req) {
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */

      const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
      smx_simcall_t req = &issuer->simcall;

      /* Debug information */
      if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
        char* req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::simix);
        XBT_DEBUG("Replay: %s (%p)", req_str, state.get());
        xbt_free(req_str);
      }

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      simgrid::mc::handle_simcall(req, value);

      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        MC_handle_comm_pattern(call, req, value, nullptr, 1);

      mc_model_checker->wait_for_requests();

      count++;
    }

    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

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
  MC_print_statistics(mc_stats);
}

void MC_print_statistics(mc_stats_t stats)
{
  if(_sg_mc_comms_determinism) {
    if (!simgrid::mc::initial_global_state->recv_deterministic &&
        simgrid::mc::initial_global_state->send_deterministic){
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-send-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", simgrid::mc::initial_global_state->recv_diff);
    }else if(!simgrid::mc::initial_global_state->send_deterministic &&
        simgrid::mc::initial_global_state->recv_deterministic) {
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-recv-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", simgrid::mc::initial_global_state->send_diff);
    }
  }

  if (stats->expanded_pairs == 0) {
    XBT_INFO("Expanded states = %lu", stats->expanded_states);
    XBT_INFO("Visited states = %lu", stats->visited_states);
  } else {
    XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  if ((_sg_mc_dot_output_file != nullptr) && (_sg_mc_dot_output_file[0] != '\0')) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (simgrid::mc::initial_global_state != nullptr
      && (_sg_mc_comms_determinism || _sg_mc_send_determinism)) {
    XBT_INFO("Send-deterministic : %s",
      !simgrid::mc::initial_global_state->send_deterministic ? "No" : "Yes");
    if (_sg_mc_comms_determinism)
      XBT_INFO("Recv-deterministic : %s",
        !simgrid::mc::initial_global_state->recv_deterministic ? "No" : "Yes");
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS")){
    int ret=system("free");
    if(ret!=0)XBT_WARN("system call did not return 0, but %d",ret);
  }
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

#if HAVE_MC
void MC_report_assertion_error(void)
{
  XBT_INFO("**************************");
  XBT_INFO("*** PROPERTY NOT VALID ***");
  XBT_INFO("**************************");
  XBT_INFO("Counter-example execution trace:");
  simgrid::mc::dumpRecordPath();
  for (auto& s : mc_model_checker->getChecker()->getTextualTrace())
    XBT_INFO("%s", s.c_str());
  MC_print_statistics(mc_stats);
}

void MC_report_crash(int status)
{
  XBT_INFO("**************************");
  XBT_INFO("** CRASH IN THE PROGRAM **");
  XBT_INFO("**************************");
  if (WIFSIGNALED(status))
    XBT_INFO("From signal: %s", strsignal(WTERMSIG(status)));
  else if (WIFEXITED(status))
    XBT_INFO("From exit: %i", WEXITSTATUS(status));
  if (WCOREDUMP(status))
    XBT_INFO("A core dump was generated by the system.");
  else
    XBT_INFO("No core dump was generated by the system.");
  XBT_INFO("Counter-example execution trace:");
  simgrid::mc::dumpRecordPath();
  for (auto& s : mc_model_checker->getChecker()->getTextualTrace())
    XBT_INFO("%s", s.c_str());
  MC_print_statistics(mc_stats);
  XBT_INFO("Stack trace:");
  mc_model_checker->process().dumpStack();
}

#endif
