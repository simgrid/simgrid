/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"

#if HAVE_SMPI
#include "smpi_request.hpp"
#endif

#include <cstdint>

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc, "Logging specific to MC communication determinism detection");

/********** Global variables **********/

std::vector<simgrid::mc::PatternCommunicationList> initial_communications_pattern;
std::vector<std::vector<simgrid::mc::PatternCommunication*>> incomplete_communications_pattern;

/********** Static functions ***********/

static e_mc_comm_pattern_difference_t compare_comm_pattern(const simgrid::mc::PatternCommunication* comm1,
                                                           const simgrid::mc::PatternCommunication* comm2)
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
                                      const simgrid::mc::PatternCommunication* comm, unsigned int cursor)
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
    res = bprintf("%s Different data size for communication #%u", type, cursor);
    break;
  case DATA_DIFF:
    res = bprintf("%s Different data for communication #%u", type, cursor);
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
  mc_model_checker->get_remote_simulation().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  smx_actor_t src_proc =
      mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->src_actor_.get()));
  smx_actor_t dst_proc =
      mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->dst_actor_.get()));
  comm_pattern->src_proc = src_proc->get_pid();
  comm_pattern->dst_proc = dst_proc->get_pid();
  comm_pattern->src_host = MC_smx_actor_get_host_name(src_proc);
  comm_pattern->dst_host = MC_smx_actor_get_host_name(dst_proc);
  if (comm_pattern->data.empty() && comm->src_buff_ != nullptr) {
    size_t buff_size;
    mc_model_checker->get_remote_simulation().read(&buff_size, remote(comm->dst_buff_size_));
    comm_pattern->data.resize(buff_size);
    mc_model_checker->get_remote_simulation().read_bytes(comm_pattern->data.data(), comm_pattern->data.size(),
                                                         remote(comm->src_buff_));
  }
}

namespace simgrid {
namespace mc {

void CommunicationDeterminismChecker::deterministic_comm_pattern(int process, const PatternCommunication* comm,
                                                                 int backtracking)
{
  if (not backtracking) {
    PatternCommunicationList& list      = initial_communications_pattern[process];
    e_mc_comm_pattern_difference_t diff = compare_comm_pattern(list.list[list.index_comm].get(), comm);

    if (diff != NONE_DIFF) {
      if (comm->type == PatternCommunicationType::send) {
        this->send_deterministic = false;
        if (this->send_diff != nullptr)
          xbt_free(this->send_diff);
        this->send_diff = print_determinism_result(diff, process, comm, list.index_comm + 1);
      } else {
        this->recv_deterministic = false;
        if (this->recv_diff != nullptr)
          xbt_free(this->recv_diff);
        this->recv_diff = print_determinism_result(diff, process, comm, list.index_comm + 1);
      }
      if (_sg_mc_send_determinism && not this->send_deterministic) {
        XBT_INFO("*********************************************************");
        XBT_INFO("***** Non-send-deterministic communications pattern *****");
        XBT_INFO("*********************************************************");
        XBT_INFO("%s", this->send_diff);
        xbt_free(this->send_diff);
        this->send_diff = nullptr;
        mc::session->log_state();
        mc_model_checker->exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      } else if (_sg_mc_comms_determinism && (not this->send_deterministic && not this->recv_deterministic)) {
        XBT_INFO("****************************************************");
        XBT_INFO("***** Non-deterministic communications pattern *****");
        XBT_INFO("****************************************************");
        if (this->send_diff) {
          XBT_INFO("%s", this->send_diff);
          xbt_free(this->send_diff);
          this->send_diff = nullptr;
        }
        if (this->recv_diff) {
          XBT_INFO("%s", this->recv_diff);
          xbt_free(this->recv_diff);
          this->recv_diff = nullptr;
        }
        mc::session->log_state();
        mc_model_checker->exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      }
    }
  }
}

/********** Non Static functions ***********/

void CommunicationDeterminismChecker::get_comm_pattern(smx_simcall_t request, e_mc_call_type_t call_type,
                                                       int backtracking)
{
  const smx_actor_t issuer = MC_smx_simcall_get_issuer(request);
  const mc::PatternCommunicationList& initial_pattern          = initial_communications_pattern[issuer->get_pid()];
  const std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[issuer->get_pid()];

  auto pattern   = std::make_unique<PatternCommunication>();
  pattern->index = initial_pattern.index_comm + incomplete_pattern.size();

  if (call_type == MC_CALL_TYPE_SEND) {
    /* Create comm pattern */
    pattern->type      = PatternCommunicationType::send;
    pattern->comm_addr = static_cast<kernel::activity::CommImpl*>(simcall_comm_isend__getraw__result(request));

    Remote<kernel::activity::CommImpl> temp_synchro;
    mc_model_checker->get_remote_simulation().read(temp_synchro, remote(pattern->comm_addr));
    const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();

    char* remote_name = mc_model_checker->get_remote_simulation().read<char*>(RemotePtr<char*>(
        (uint64_t)(synchro->get_mailbox() ? &synchro->get_mailbox()->name_ : &synchro->mbox_cpy->name_)));
    pattern->rdv      = mc_model_checker->get_remote_simulation().read_string(RemotePtr<char>(remote_name));
    pattern->src_proc =
        mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(synchro->src_actor_.get()))->get_pid();
    pattern->src_host = MC_smx_actor_get_host_name(issuer);

#if HAVE_SMPI
    simgrid::smpi::Request mpi_request;
    mc_model_checker->get_remote_simulation().read(
        &mpi_request, remote(static_cast<smpi::Request*>(simcall_comm_isend__get__data(request))));
    pattern->tag = mpi_request.tag();
#endif

    if (synchro->src_buff_ != nullptr) {
      pattern->data.resize(synchro->src_buff_size_);
      mc_model_checker->get_remote_simulation().read_bytes(pattern->data.data(), pattern->data.size(),
                                                           remote(synchro->src_buff_));
    }
#if HAVE_SMPI
    if(mpi_request.detached()){
      if (this->initial_communications_pattern_done) {
        /* Evaluate comm determinism */
        this->deterministic_comm_pattern(pattern->src_proc, pattern.get(), backtracking);
        initial_communications_pattern[pattern->src_proc].index_comm++;
      } else {
        /* Store comm pattern */
        initial_communications_pattern[pattern->src_proc].list.push_back(std::move(pattern));
      }
      return;
    }
#endif
  } else if (call_type == MC_CALL_TYPE_RECV) {
    pattern->type      = PatternCommunicationType::receive;
    pattern->comm_addr = static_cast<kernel::activity::CommImpl*>(simcall_comm_irecv__getraw__result(request));

#if HAVE_SMPI
    smpi::Request mpi_request;
    mc_model_checker->get_remote_simulation().read(
        &mpi_request, remote(static_cast<smpi::Request*>(simcall_comm_irecv__get__data(request))));
    pattern->tag = mpi_request.tag();
#endif

    Remote<kernel::activity::CommImpl> temp_comm;
    mc_model_checker->get_remote_simulation().read(temp_comm, remote(pattern->comm_addr));
    const kernel::activity::CommImpl* comm = temp_comm.get_buffer();

    char* remote_name;
    mc_model_checker->get_remote_simulation().read(
        &remote_name, remote(comm->get_mailbox() ? &xbt::string::to_string_data(comm->get_mailbox()->name_).data
                                                 : &xbt::string::to_string_data(comm->mbox_cpy->name_).data));
    pattern->rdv = mc_model_checker->get_remote_simulation().read_string(RemotePtr<char>(remote_name));
    pattern->dst_proc =
        mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(comm->dst_actor_.get()))->get_pid();
    pattern->dst_host = MC_smx_actor_get_host_name(issuer);
  } else
    xbt_die("Unexpected call_type %i", (int) call_type);

  XBT_DEBUG("Insert incomplete comm pattern %p for process %ld", pattern.get(), issuer->get_pid());
  incomplete_communications_pattern[issuer->get_pid()].push_back(pattern.release());
}

void CommunicationDeterminismChecker::complete_comm_pattern(RemotePtr<kernel::activity::CommImpl> comm_addr,
                                                            unsigned int issuer, int backtracking)
{
  /* Complete comm pattern */
  std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[issuer];
  auto current_comm_pattern =
      std::find_if(begin(incomplete_pattern), end(incomplete_pattern),
                   [&comm_addr](const PatternCommunication* comm) { return remote(comm->comm_addr) == comm_addr; });
  if (current_comm_pattern == std::end(incomplete_pattern))
    xbt_die("Corresponding communication not found!");

  update_comm_pattern(*current_comm_pattern, comm_addr);
  std::unique_ptr<PatternCommunication> comm_pattern(*current_comm_pattern);
  XBT_DEBUG("Remove incomplete comm pattern for process %u at cursor %zd", issuer,
            std::distance(begin(incomplete_pattern), current_comm_pattern));
  incomplete_pattern.erase(current_comm_pattern);

  if (this->initial_communications_pattern_done) {
    /* Evaluate comm determinism */
    this->deterministic_comm_pattern(issuer, comm_pattern.get(), backtracking);
    initial_communications_pattern[issuer].index_comm++;
  } else {
    /* Store comm pattern */
    initial_communications_pattern[issuer].list.push_back(std::move(comm_pattern));
  }
}

CommunicationDeterminismChecker::CommunicationDeterminismChecker(Session& s) : Checker(s)
{
}

CommunicationDeterminismChecker::~CommunicationDeterminismChecker() = default;

RecordTrace CommunicationDeterminismChecker::get_record_trace() // override
{
  RecordTrace res;
  for (auto const& state : stack_)
    res.push_back(state->get_transition());
  return res;
}

std::vector<std::string> CommunicationDeterminismChecker::get_textual_trace() // override
{
  std::vector<std::string> trace;
  for (auto const& state : stack_) {
    smx_simcall_t req = &state->executed_req_;
    if (req)
      trace.push_back(request_to_string(req, state->transition_.argument_, RequestType::executed));
  }
  return trace;
}

void CommunicationDeterminismChecker::log_state() // override
{
  if (_sg_mc_comms_determinism) {
    if (this->send_deterministic && not this->recv_deterministic) {
      XBT_INFO("*******************************************************");
      XBT_INFO("**** Only-send-deterministic communication pattern ****");
      XBT_INFO("*******************************************************");
      XBT_INFO("%s", this->recv_diff);
    }
    if (not this->send_deterministic && this->recv_deterministic) {
      XBT_INFO("*******************************************************");
      XBT_INFO("**** Only-recv-deterministic communication pattern ****");
      XBT_INFO("*******************************************************");
      XBT_INFO("%s", this->send_diff);
    }
  }
  XBT_INFO("Expanded states = %lu", expanded_states_count_);
  XBT_INFO("Visited states = %lu", mc_model_checker->visited_states);
  XBT_INFO("Executed transitions = %lu", mc_model_checker->executed_transitions);
  XBT_INFO("Send-deterministic : %s", this->send_deterministic ? "Yes" : "No");
  if (_sg_mc_comms_determinism)
    XBT_INFO("Recv-deterministic : %s", this->recv_deterministic ? "Yes" : "No");
}

void CommunicationDeterminismChecker::prepare()
{
  const int maxpid = MC_smx_get_maxpid();

  initial_communications_pattern.resize(maxpid);
  incomplete_communications_pattern.resize(maxpid);

  ++expanded_states_count_;
  auto initial_state = std::make_unique<State>(expanded_states_count_);

  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Get an enabled actor and insert it in the interleave set of the initial state */
  for (auto& actor : mc_model_checker->get_remote_simulation().actors())
    if (mc::actor_is_enabled(actor.copy.get_buffer()))
      initial_state->add_interleaving_set(actor.copy.get_buffer());

  stack_.push_back(std::move(initial_state));
}

static inline bool all_communications_are_finished()
{
  for (size_t current_actor = 1; current_actor < MC_smx_get_maxpid(); current_actor++) {
    if (not incomplete_communications_pattern[current_actor].empty()) {
      XBT_DEBUG("Some communications are not finished, cannot stop the exploration! State not visited.");
      return false;
    }
  }
  return true;
}

void CommunicationDeterminismChecker::restoreState()
{
  /* Intermediate backtracking */
  State* last_state = stack_.back().get();
  if (last_state->system_state_) {
    last_state->system_state_->restore(&mc_model_checker->get_remote_simulation());
    MC_restore_communications_pattern(last_state);
    return;
  }

  /* Restore the initial state */
  mc::session->restore_initial_state();

  unsigned n = MC_smx_get_maxpid();
  assert(n == incomplete_communications_pattern.size());
  assert(n == initial_communications_pattern.size());
  for (unsigned j=0; j < n ; j++) {
    incomplete_communications_pattern[j].clear();
    initial_communications_pattern[j].index_comm = 0;
  }

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack_) {
    if (state == stack_.back())
      break;

    int req_num             = state->transition_.argument_;
    const s_smx_simcall* saved_req = &state->executed_req_;
    xbt_assert(saved_req);

    /* because we got a copy of the executed request, we have to fetch the
       real one, pointed by the request field of the issuer process */

    const smx_actor_t issuer = MC_smx_simcall_get_issuer(saved_req);
    smx_simcall_t req        = &issuer->simcall_;

    /* TODO : handle test and testany simcalls */
    e_mc_call_type_t call = MC_get_call_type(req);
    mc_model_checker->handle_simcall(state->transition_);
    MC_handle_comm_pattern(call, req, req_num, 1);
    mc_model_checker->wait_for_requests();

    /* Update statistics */
    mc_model_checker->visited_states++;
    mc_model_checker->executed_transitions++;
  }
}

void CommunicationDeterminismChecker::real_run()
{
  std::unique_ptr<VisitedState> visited_state = nullptr;
  smx_simcall_t req = nullptr;

  while (not stack_.empty()) {
    /* Get current state */
    State* cur_state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth = %zu (state = %d, interleaved processes = %zu)", stack_.size(), cur_state->num_,
              cur_state->interleave_size());

    /* Update statistics */
    mc_model_checker->visited_states++;

    if (stack_.size() <= (std::size_t)_sg_mc_max_depth)
      req = MC_state_choose_request(cur_state);
    else
      req = nullptr;

    if (req != nullptr && visited_state == nullptr) {
      int req_num = cur_state->transition_.argument_;

      XBT_DEBUG("Execute: %s", request_to_string(req, req_num, RequestType::simix).c_str());

      std::string req_str;
      if (dot_output != nullptr)
        req_str = request_get_dot_output(req, req_num);

      mc_model_checker->executed_transitions++;

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      /* Answer the request */
      mc_model_checker->handle_simcall(cur_state->transition_);
      /* After this call req is no longer useful */

      MC_handle_comm_pattern(call, req, req_num, 0);

      /* Wait for requests (schedules processes) */
      mc_model_checker->wait_for_requests();

      /* Create the new expanded state */
      ++expanded_states_count_;
      auto next_state = std::make_unique<State>(expanded_states_count_);

      /* If comm determinism verification, we cannot stop the exploration if some communications are not finished (at
       * least, data are transferred). These communications  are incomplete and they cannot be analyzed and compared
       * with the initial pattern. */
      bool compare_snapshots = this->initial_communications_pattern_done && all_communications_are_finished();

      if (_sg_mc_max_visited_states != 0)
        visited_state = visited_states_.addVisitedState(expanded_states_count_, next_state.get(), compare_snapshots);
      else
        visited_state = nullptr;

      if (visited_state == nullptr) {
        /* Get enabled actors and insert them in the interleave set of the next state */
        for (auto& actor : mc_model_checker->get_remote_simulation().actors())
          if (simgrid::mc::actor_is_enabled(actor.copy.get_buffer()))
            next_state->add_interleaving_set(actor.copy.get_buffer());

        if (dot_output != nullptr)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", cur_state->num_, next_state->num_, req_str.c_str());

      } else if (dot_output != nullptr)
        fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", cur_state->num_,
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

      this->initial_communications_pattern_done = true;

      /* Trash the current state, no longer needed */
      XBT_DEBUG("Delete state %d at depth %zu", cur_state->num_, stack_.size());
      stack_.pop_back();

      visited_state = nullptr;

      /* Check for deadlocks */
      if (mc_model_checker->checkDeadlock()) {
        MC_show_deadlock();
        throw simgrid::mc::DeadlockError();
      }

      while (not stack_.empty()) {
        std::unique_ptr<State> state(std::move(stack_.back()));
        stack_.pop_back();
        if (state->interleave_size() && stack_.size() < (std::size_t)_sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %zu", state->num_, stack_.size() + 1);
          stack_.push_back(std::move(state));

          this->restoreState();

          XBT_DEBUG("Back-tracking to state %d at depth %zu done", stack_.back()->num_, stack_.size());

          break;
        } else {
          XBT_DEBUG("Delete state %d at depth %zu", state->num_, stack_.size() + 1);
        }
      }
    }
  }

  mc::session->log_state();
}

void CommunicationDeterminismChecker::run()
{
  XBT_INFO("Check communication determinism");
  mc::session->initialize();

  this->prepare();
  this->real_run();
}

Checker* createCommunicationDeterminismChecker(Session& s)
{
  return new CommunicationDeterminismChecker(s);
}

} // namespace mc
} // namespace simgrid
