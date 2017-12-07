/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>

#include <xbt/dynar.h>
#include <xbt/dynar.hpp>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/mc/Transition.hpp"
#include "src/mc/VisitedState.hpp"
#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_state.hpp"
#include "src/mc/remote/Client.hpp"

#include "smpi_request.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc, "Logging specific to MC communication determinism detection");

/********** Global variables **********/

xbt_dynar_t initial_communications_pattern;
xbt_dynar_t incomplete_communications_pattern;

/********** Static functions ***********/

static e_mc_comm_pattern_difference_t compare_comm_pattern(simgrid::mc::PatternCommunication* comm1,
                                                           simgrid::mc::PatternCommunication* comm2)
{
  if(comm1->type != comm2->type)
    return TYPE_DIFF;
  if (comm1->rdv != comm2->rdv)
    return RDV_DIFF;
  if (comm1->src_proc != comm2->src_proc)
    return SRC_PROC_DIFF;
  if (comm1->dst_proc != comm2->dst_proc)
    return DST_PROC_DIFF;
  if (comm1->tag != comm2->tag)
    return TAG_DIFF;
  if (comm1->data.size() != comm2->data.size())
    return DATA_SIZE_DIFF;
  if (comm1->data != comm2->data)
    return DATA_DIFF;
  return NONE_DIFF;
}

static char* print_determinism_result(e_mc_comm_pattern_difference_t diff, int process,
                                      simgrid::mc::PatternCommunication* comm, unsigned int cursor)
{
  char* type;
  char* res;

  if (comm->type == simgrid::mc::PatternCommunicationType::send)
    type = bprintf("The send communications pattern of the process %d is different!", process - 1);
  else
    type = bprintf("The recv communications pattern of the process %d is different!", process - 1);

  switch(diff) {
  case TYPE_DIFF:
    res = bprintf("%s Different type for communication #%u", type, cursor);
    break;
  case RDV_DIFF:
    res = bprintf("%s Different rdv for communication #%u", type, cursor);
    break;
  case TAG_DIFF:
    res = bprintf("%s Different tag for communication #%u", type, cursor);
    break;
  case SRC_PROC_DIFF:
    res = bprintf("%s Different source for communication #%u", type, cursor);
    break;
  case DST_PROC_DIFF:
    res = bprintf("%s Different destination for communication #%u", type, cursor);
    break;
  case DATA_SIZE_DIFF:
    res = bprintf("%s\n Different data size for communication #%u", type, cursor);
    break;
  case DATA_DIFF:
    res = bprintf("%s\n Different data for communication #%u", type, cursor);
    break;
  default:
    res = nullptr;
    break;
  }

  return res;
}

static void update_comm_pattern(simgrid::mc::PatternCommunication* comm_pattern,
                                simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr)
{
  // HACK, type punning
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->process().read(temp_comm, comm_addr);
  simgrid::kernel::activity::CommImpl* comm = temp_comm.getBuffer();

  smx_actor_t src_proc   = mc_model_checker->process().resolveActor(simgrid::mc::remote(comm->src_proc));
  smx_actor_t dst_proc   = mc_model_checker->process().resolveActor(simgrid::mc::remote(comm->dst_proc));
  comm_pattern->src_proc = src_proc->pid;
  comm_pattern->dst_proc = dst_proc->pid;
  comm_pattern->src_host = MC_smx_actor_get_host_name(src_proc);
  comm_pattern->dst_host = MC_smx_actor_get_host_name(dst_proc);
  if (comm_pattern->data.size() == 0 && comm->src_buff != nullptr) {
    size_t buff_size;
    mc_model_checker->process().read(&buff_size, remote(comm->dst_buff_size));
    comm_pattern->data.resize(buff_size);
    mc_model_checker->process().read_bytes(comm_pattern->data.data(), comm_pattern->data.size(),
                                           remote(comm->src_buff));
  }
}

namespace simgrid {
namespace mc {

void CommunicationDeterminismChecker::deterministic_comm_pattern(int process, simgrid::mc::PatternCommunication* comm,
                                                                 int backtracking)
{
  simgrid::mc::PatternCommunicationList* list =
    xbt_dynar_get_as(initial_communications_pattern, process, simgrid::mc::PatternCommunicationList*);

  if (not backtracking) {
    e_mc_comm_pattern_difference_t diff = compare_comm_pattern(list->list[list->index_comm].get(), comm);

    if (diff != NONE_DIFF) {
      if (comm->type == simgrid::mc::PatternCommunicationType::send) {
        this->send_deterministic = 0;
        if (this->send_diff != nullptr)
          xbt_free(this->send_diff);
        this->send_diff = print_determinism_result(diff, process, comm, list->index_comm + 1);
      } else {
        this->recv_deterministic = 0;
        if (this->recv_diff != nullptr)
          xbt_free(this->recv_diff);
        this->recv_diff = print_determinism_result(diff, process, comm, list->index_comm + 1);
      }
      if (_sg_mc_send_determinism && not this->send_deterministic) {
        XBT_INFO("*********************************************************");
        XBT_INFO("***** Non-send-deterministic communications pattern *****");
        XBT_INFO("*********************************************************");
        XBT_INFO("%s", this->send_diff);
        xbt_free(this->send_diff);
        this->send_diff = nullptr;
        simgrid::mc::session->logState();
        mc_model_checker->exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      } else if (_sg_mc_comms_determinism && (not this->send_deterministic && not this->recv_deterministic)) {
        XBT_INFO("****************************************************");
        XBT_INFO("***** Non-deterministic communications pattern *****");
        XBT_INFO("****************************************************");
        XBT_INFO("%s", this->send_diff);
        XBT_INFO("%s", this->recv_diff);
        xbt_free(this->send_diff);
        this->send_diff = nullptr;
        xbt_free(this->recv_diff);
        this->recv_diff = nullptr;
        simgrid::mc::session->logState();
        mc_model_checker->exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      }
    }
  }
}

/********** Non Static functions ***********/

void CommunicationDeterminismChecker::get_comm_pattern(xbt_dynar_t list, smx_simcall_t request,
                                                       e_mc_call_type_t call_type, int backtracking)
{
  const smx_actor_t issuer = MC_smx_simcall_get_issuer(request);
  simgrid::mc::PatternCommunicationList* initial_pattern =
      xbt_dynar_get_as(initial_communications_pattern, issuer->pid, simgrid::mc::PatternCommunicationList*);
  xbt_dynar_t incomplete_pattern = xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t);

  std::unique_ptr<simgrid::mc::PatternCommunication> pattern =
      std::unique_ptr<simgrid::mc::PatternCommunication>(new simgrid::mc::PatternCommunication());
  pattern->index = initial_pattern->index_comm + xbt_dynar_length(incomplete_pattern);

  if (call_type == MC_CALL_TYPE_SEND) {
    /* Create comm pattern */
    pattern->type = simgrid::mc::PatternCommunicationType::send;
    pattern->comm_addr = static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_isend__getraw__result(request));

    simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_synchro;
    mc_model_checker->process().read(temp_synchro,
                                     remote(static_cast<simgrid::kernel::activity::CommImpl*>(pattern->comm_addr)));
    simgrid::kernel::activity::CommImpl* synchro =
        static_cast<simgrid::kernel::activity::CommImpl*>(temp_synchro.getBuffer());

    char* remote_name = mc_model_checker->process().read<char*>(
        RemotePtr<char*>((uint64_t)(synchro->mbox ? &synchro->mbox->name_ : &synchro->mbox_cpy->name_)));
    pattern->rdv      = mc_model_checker->process().read_string(RemotePtr<char>(remote_name));
    pattern->src_proc = mc_model_checker->process().resolveActor(simgrid::mc::remote(synchro->src_proc))->pid;
    pattern->src_host = MC_smx_actor_get_host_name(issuer);

    simgrid::smpi::Request mpi_request = mc_model_checker->process().read<simgrid::smpi::Request>(
        RemotePtr<simgrid::smpi::Request>((std::uint64_t)simcall_comm_isend__get__data(request)));
    pattern->tag = mpi_request.tag();

    if (synchro->src_buff != nullptr) {
      pattern->data.resize(synchro->src_buff_size);
      mc_model_checker->process().read_bytes(pattern->data.data(), pattern->data.size(), remote(synchro->src_buff));
    }
    if(mpi_request.detached()){
      if (not this->initial_communications_pattern_done) {
        /* Store comm pattern */
        simgrid::mc::PatternCommunicationList* list =
            xbt_dynar_get_as(initial_communications_pattern, pattern->src_proc, simgrid::mc::PatternCommunicationList*);
        list->list.push_back(std::move(pattern));
      } else {
        /* Evaluate comm determinism */
        this->deterministic_comm_pattern(pattern->src_proc, pattern.get(), backtracking);
        xbt_dynar_get_as(initial_communications_pattern, pattern->src_proc, simgrid::mc::PatternCommunicationList*)
            ->index_comm++;
      }
      return;
    }
  } else if (call_type == MC_CALL_TYPE_RECV) {
    pattern->type = simgrid::mc::PatternCommunicationType::receive;
    pattern->comm_addr = static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_irecv__getraw__result(request));

    simgrid::smpi::Request mpi_request;
    mc_model_checker->process().read(&mpi_request,
                                     remote((simgrid::smpi::Request*)simcall_comm_irecv__get__data(request)));
    pattern->tag = mpi_request.tag();

    simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
    mc_model_checker->process().read(temp_comm,
                                     remote(static_cast<simgrid::kernel::activity::CommImpl*>(pattern->comm_addr)));
    simgrid::kernel::activity::CommImpl* comm = temp_comm.getBuffer();

    char* remote_name;
    mc_model_checker->process().read(
        &remote_name, remote(comm->mbox ? &simgrid::xbt::string::to_string_data(comm->mbox->name_).data
                                        : &simgrid::xbt::string::to_string_data(comm->mbox_cpy->name_).data));
    pattern->rdv      = mc_model_checker->process().read_string(RemotePtr<char>(remote_name));
    pattern->dst_proc = mc_model_checker->process().resolveActor(simgrid::mc::remote(comm->dst_proc))->pid;
    pattern->dst_host = MC_smx_actor_get_host_name(issuer);
  } else
    xbt_die("Unexpected call_type %i", (int) call_type);

  XBT_DEBUG("Insert incomplete comm pattern %p for process %lu", pattern.get(), issuer->pid);
  xbt_dynar_t dynar = xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t);
  simgrid::mc::PatternCommunication* pattern2 = pattern.release();
  xbt_dynar_push(dynar, &pattern2);
}

void CommunicationDeterminismChecker::complete_comm_pattern(
    xbt_dynar_t list, simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr, unsigned int issuer,
    int backtracking)
{
  simgrid::mc::PatternCommunication* current_comm_pattern;
  unsigned int cursor = 0;
  std::unique_ptr<simgrid::mc::PatternCommunication> comm_pattern;
  int completed = 0;

  /* Complete comm pattern */
  xbt_dynar_foreach(xbt_dynar_get_as(incomplete_communications_pattern, issuer, xbt_dynar_t), cursor, current_comm_pattern)
    if (remote(current_comm_pattern->comm_addr) == comm_addr) {
      update_comm_pattern(current_comm_pattern, comm_addr);
      completed = 1;
      simgrid::mc::PatternCommunication* temp;
      xbt_dynar_remove_at(xbt_dynar_get_as(incomplete_communications_pattern, issuer, xbt_dynar_t), cursor, &temp);
      comm_pattern = std::unique_ptr<simgrid::mc::PatternCommunication>(temp);
      XBT_DEBUG("Remove incomplete comm pattern for process %u at cursor %u", issuer, cursor);
      break;
    }

  if (not completed)
    xbt_die("Corresponding communication not found!");

  simgrid::mc::PatternCommunicationList* pattern =
      xbt_dynar_get_as(initial_communications_pattern, issuer, simgrid::mc::PatternCommunicationList*);

  if (not this->initial_communications_pattern_done)
    /* Store comm pattern */
    pattern->list.push_back(std::move(comm_pattern));
  else {
    /* Evaluate comm determinism */
    this->deterministic_comm_pattern(issuer, comm_pattern.get(), backtracking);
    pattern->index_comm++;
  }
}

CommunicationDeterminismChecker::CommunicationDeterminismChecker(Session& session) : Checker(session)
{
}

CommunicationDeterminismChecker::~CommunicationDeterminismChecker() = default;

RecordTrace CommunicationDeterminismChecker::getRecordTrace() // override
{
  RecordTrace res;
  for (auto const& state : stack_)
    res.push_back(state->getTransition());
  return res;
}

std::vector<std::string> CommunicationDeterminismChecker::getTextualTrace() // override
{
  std::vector<std::string> trace;
  for (auto const& state : stack_) {
    smx_simcall_t req = &state->executed_req;
    if (req)
      trace.push_back(
          simgrid::mc::request_to_string(req, state->transition.argument, simgrid::mc::RequestType::executed));
  }
  return trace;
}

void CommunicationDeterminismChecker::logState() // override
{
  if (_sg_mc_comms_determinism && not this->recv_deterministic && this->send_deterministic) {
    XBT_INFO("******************************************************");
    XBT_INFO("**** Only-send-deterministic communication pattern ****");
    XBT_INFO("******************************************************");
    XBT_INFO("%s", this->recv_diff);
  } else if (_sg_mc_comms_determinism && not this->send_deterministic && this->recv_deterministic) {
    XBT_INFO("******************************************************");
    XBT_INFO("**** Only-recv-deterministic communication pattern ****");
    XBT_INFO("******************************************************");
    XBT_INFO("%s", this->send_diff);
  }
  XBT_INFO("Expanded states = %lu", expandedStatesCount_);
  XBT_INFO("Visited states = %lu", mc_model_checker->visited_states);
  XBT_INFO("Executed transitions = %lu", mc_model_checker->executed_transitions);
  XBT_INFO("Send-deterministic : %s", not this->send_deterministic ? "No" : "Yes");
  if (_sg_mc_comms_determinism)
    XBT_INFO("Recv-deterministic : %s", not this->recv_deterministic ? "No" : "Yes");
}

void CommunicationDeterminismChecker::prepare()
{
  const int maxpid = MC_smx_get_maxpid();

  // Create initial_communications_pattern elements:
  initial_communications_pattern = simgrid::xbt::newDeleteDynar<simgrid::mc::PatternCommunicationList*>();
  for (int i = 0; i < maxpid; i++) {
    simgrid::mc::PatternCommunicationList* process_list_pattern = new simgrid::mc::PatternCommunicationList();
    xbt_dynar_insert_at(initial_communications_pattern, i, &process_list_pattern);
  }

  // Create incomplete_communications_pattern elements:
  incomplete_communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (int i = 0; i < maxpid; i++) {
    xbt_dynar_t process_pattern = xbt_dynar_new(sizeof(simgrid::mc::PatternCommunication*), nullptr);
    xbt_dynar_insert_at(incomplete_communications_pattern, i, &process_pattern);
  }

  std::unique_ptr<simgrid::mc::State> initial_state =
      std::unique_ptr<simgrid::mc::State>(new simgrid::mc::State(++expandedStatesCount_));

  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  for (auto& actor : mc_model_checker->process().actors())
    if (simgrid::mc::actor_is_enabled(actor.copy.getBuffer()))
      initial_state->addInterleavingSet(actor.copy.getBuffer());

  stack_.push_back(std::move(initial_state));
}

static inline bool all_communications_are_finished()
{
  for (size_t current_actor = 1; current_actor < MC_smx_get_maxpid(); current_actor++) {
    xbt_dynar_t pattern = xbt_dynar_get_as(incomplete_communications_pattern, current_actor, xbt_dynar_t);
    if (not xbt_dynar_is_empty(pattern)) {
      XBT_DEBUG("Some communications are not finished, cannot stop the exploration! State not visited.");
      return false;
    }
  }
  return true;
}

void CommunicationDeterminismChecker::restoreState()
{
  /* Intermediate backtracking */
  simgrid::mc::State* state = stack_.back().get();
  if (state->system_state) {
    simgrid::mc::restore_snapshot(state->system_state);
    MC_restore_communications_pattern(state);
    return;
  }

  /* Restore the initial state */
  simgrid::mc::session->restoreInitialState();

  unsigned n = MC_smx_get_maxpid();
  assert(n == xbt_dynar_length(incomplete_communications_pattern));
  assert(n == xbt_dynar_length(initial_communications_pattern));
  for (unsigned j=0; j < n ; j++) {
    xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, j, xbt_dynar_t));
    xbt_dynar_get_as(initial_communications_pattern, j, simgrid::mc::PatternCommunicationList*)->index_comm = 0;
  }

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack_) {
    if (state == stack_.back())
      break;

    int req_num = state->transition.argument;
    smx_simcall_t saved_req = &state->executed_req;
    xbt_assert(saved_req);

    /* because we got a copy of the executed request, we have to fetch the
       real one, pointed by the request field of the issuer process */

    const smx_actor_t issuer = MC_smx_simcall_get_issuer(saved_req);
    smx_simcall_t req = &issuer->simcall;

    /* TODO : handle test and testany simcalls */
    e_mc_call_type_t call = MC_get_call_type(req);
    mc_model_checker->handle_simcall(state->transition);
    MC_handle_comm_pattern(call, req, req_num, nullptr, 1);
    mc_model_checker->wait_for_requests();

    /* Update statistics */
    mc_model_checker->visited_states++;
    mc_model_checker->executed_transitions++;
  }
}

void CommunicationDeterminismChecker::main()
{
  std::unique_ptr<simgrid::mc::VisitedState> visited_state = nullptr;
  smx_simcall_t req = nullptr;

  while (not stack_.empty()) {
    /* Get current state */
    simgrid::mc::State* state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth = %zu (state = %d, interleaved processes = %zu)", stack_.size(), state->num,
              state->interleaveSize());

    /* Update statistics */
    mc_model_checker->visited_states++;

    if (stack_.size() <= (std::size_t)_sg_mc_max_depth)
      req = MC_state_get_request(state);
    else
      req = nullptr;

    if (req != nullptr && visited_state == nullptr) {

      int req_num = state->transition.argument;

      XBT_DEBUG("Execute: %s", simgrid::mc::request_to_string(req, req_num, simgrid::mc::RequestType::simix).c_str());

      std::string req_str;
      if (dot_output != nullptr)
        req_str = simgrid::mc::request_get_dot_output(req, req_num);

      mc_model_checker->executed_transitions++;

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      /* Answer the request */
      mc_model_checker->handle_simcall(state->transition);
      /* After this call req is no longer useful */

      if (not this->initial_communications_pattern_done)
        MC_handle_comm_pattern(call, req, req_num, initial_communications_pattern, 0);
      else
        MC_handle_comm_pattern(call, req, req_num, nullptr, 0);

      /* Wait for requests (schedules processes) */
      mc_model_checker->wait_for_requests();

      /* Create the new expanded state */
      std::unique_ptr<simgrid::mc::State> next_state =
          std::unique_ptr<simgrid::mc::State>(new simgrid::mc::State(++expandedStatesCount_));

      /* If comm determinism verification, we cannot stop the exploration if some communications are not finished (at
       * least, data are transferred). These communications  are incomplete and they cannot be analyzed and compared
       * with the initial pattern. */
      bool compare_snapshots = all_communications_are_finished() && this->initial_communications_pattern_done;

      if (_sg_mc_max_visited_states != 0)
        visited_state = visitedStates_.addVisitedState(expandedStatesCount_, next_state.get(), compare_snapshots);
      else
        visited_state = nullptr;

      if (visited_state == nullptr) {

        /* Get enabled actors and insert them in the interleave set of the next state */
        for (auto& actor : mc_model_checker->process().actors())
          if (simgrid::mc::actor_is_enabled(actor.copy.getBuffer()))
            next_state->addInterleavingSet(actor.copy.getBuffer());

        if (dot_output != nullptr)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n",
            state->num,  next_state->num, req_str.c_str());

      } else if (dot_output != nullptr)
        fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num,
                visited_state->original_num == -1 ? visited_state->num : visited_state->original_num, req_str.c_str());

      stack_.push_back(std::move(next_state));

    } else {

      if (stack_.size() > (std::size_t) _sg_mc_max_depth)
        XBT_WARN("/!\\ Max depth reached! /!\\ ");
      else if (visited_state != nullptr)
        XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.",
            visited_state->original_num == -1 ? visited_state->num : visited_state->original_num);
      else
        XBT_DEBUG("There are no more processes to interleave. (depth %zu)", stack_.size());

      if (not this->initial_communications_pattern_done)
        this->initial_communications_pattern_done = 1;

      /* Trash the current state, no longer needed */
      XBT_DEBUG("Delete state %d at depth %zu", state->num, stack_.size());
      stack_.pop_back();

      visited_state = nullptr;

      /* Check for deadlocks */
      if (mc_model_checker->checkDeadlock()) {
        MC_show_deadlock();
        throw simgrid::mc::DeadlockError();
      }

      while (not stack_.empty()) {
        std::unique_ptr<simgrid::mc::State> state = std::move(stack_.back());
        stack_.pop_back();
        if (state->interleaveSize() && stack_.size() < (std::size_t)_sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %zu", state->num, stack_.size() + 1);
          stack_.push_back(std::move(state));

          this->restoreState();

          XBT_DEBUG("Back-tracking to state %d at depth %zu done", stack_.back()->num, stack_.size());

          break;
        } else {
          XBT_DEBUG("Delete state %d at depth %zu", state->num, stack_.size() + 1);
        }
      }
    }
  }

  simgrid::mc::session->logState();
}

void CommunicationDeterminismChecker::run()
{
  XBT_INFO("Check communication determinism");
  simgrid::mc::session->initialize();

  this->prepare();

  this->main();
}

Checker* createCommunicationDeterminismChecker(Session& session)
{
  return new CommunicationDeterminismChecker(session);
}

}
}
