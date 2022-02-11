/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"

#include <cstdint>

using api = simgrid::mc::Api;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc, "Logging specific to MC communication determinism detection");

/********** Global variables **********/

std::vector<simgrid::mc::PatternCommunicationList> initial_communications_pattern;
std::vector<std::vector<simgrid::mc::PatternCommunication*>> incomplete_communications_pattern;

/********** Static functions ***********/

static simgrid::mc::CommPatternDifference compare_comm_pattern(const simgrid::mc::PatternCommunication* comm1,
                                                               const simgrid::mc::PatternCommunication* comm2)
{
  using simgrid::mc::CommPatternDifference;
  if (comm1->type != comm2->type)
    return CommPatternDifference::TYPE;
  if (comm1->rdv != comm2->rdv)
    return CommPatternDifference::RDV;
  if (comm1->src_proc != comm2->src_proc)
    return CommPatternDifference::SRC_PROC;
  if (comm1->dst_proc != comm2->dst_proc)
    return CommPatternDifference::DST_PROC;
  if (comm1->tag != comm2->tag)
    return CommPatternDifference::TAG;
  if (comm1->data.size() != comm2->data.size())
    return CommPatternDifference::DATA_SIZE;
  if (comm1->data != comm2->data)
    return CommPatternDifference::DATA;
  return CommPatternDifference::NONE;
}

static void patterns_copy(std::vector<simgrid::mc::PatternCommunication*>& dest,
                             std::vector<simgrid::mc::PatternCommunication> const& source)
{
  dest.clear();
  for (simgrid::mc::PatternCommunication const& comm : source) {
    auto* copy_comm = new simgrid::mc::PatternCommunication(comm.dup());
    dest.push_back(copy_comm);
  }
}

static void restore_communications_pattern(simgrid::mc::State* state)
{
  for (size_t i = 0; i < initial_communications_pattern.size(); i++)
    initial_communications_pattern[i].index_comm = state->communication_indices_[i];

  const unsigned long maxpid = api::get().get_maxpid();
  for (unsigned long i = 0; i < maxpid; i++)
    patterns_copy(incomplete_communications_pattern[i], state->incomplete_comm_pattern_[i]);
}

static char* print_determinism_result(simgrid::mc::CommPatternDifference diff, aid_t process,
                                      const simgrid::mc::PatternCommunication* comm, unsigned int cursor)
{
  char* type;
  char* res;

  if (comm->type == simgrid::mc::PatternCommunicationType::send)
    type = bprintf("The send communications pattern of the process %ld is different!", process - 1);
  else
    type = bprintf("The recv communications pattern of the process %ld is different!", process - 1);

  using simgrid::mc::CommPatternDifference;
  switch (diff) {
    case CommPatternDifference::TYPE:
      res = bprintf("%s Different type for communication #%u", type, cursor);
      break;
    case CommPatternDifference::RDV:
      res = bprintf("%s Different rdv for communication #%u", type, cursor);
      break;
    case CommPatternDifference::TAG:
      res = bprintf("%s Different tag for communication #%u", type, cursor);
      break;
    case CommPatternDifference::SRC_PROC:
      res = bprintf("%s Different source for communication #%u", type, cursor);
      break;
    case CommPatternDifference::DST_PROC:
      res = bprintf("%s Different destination for communication #%u", type, cursor);
      break;
    case CommPatternDifference::DATA_SIZE:
      res = bprintf("%s Different data size for communication #%u", type, cursor);
      break;
    case CommPatternDifference::DATA:
      res = bprintf("%s Different data for communication #%u", type, cursor);
      break;
    default:
      res = nullptr;
      break;
  }

  return res;
}

static void update_comm_pattern(simgrid::mc::PatternCommunication* comm_pattern,
                                simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> const& comm_addr)
{
  auto src_proc = api::get().get_src_actor(comm_addr);
  auto dst_proc = api::get().get_dst_actor(comm_addr);
  comm_pattern->src_proc = src_proc->get_pid();
  comm_pattern->dst_proc = dst_proc->get_pid();
  comm_pattern->src_host = &api::get().get_actor_host_name(src_proc);
  comm_pattern->dst_host = &api::get().get_actor_host_name(dst_proc);

  if (comm_pattern->data.empty()) {
    comm_pattern->data = api::get().get_pattern_comm_data(comm_addr);
  }
}

namespace simgrid {
namespace mc {

void CommunicationDeterminismChecker::deterministic_comm_pattern(aid_t process, const PatternCommunication* comm,
                                                                 bool backtracking)
{
  if (not backtracking) {
    PatternCommunicationList& list = initial_communications_pattern[process];
    CommPatternDifference diff     = compare_comm_pattern(list.list[list.index_comm].get(), comm);

    if (diff != CommPatternDifference::NONE) {
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
        api::get().log_state();
        api::get().mc_exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
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
        api::get().log_state();
        api::get().mc_exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      }
    }
  }
}

/********** Non Static functions ***********/

void CommunicationDeterminismChecker::get_comm_pattern(smx_simcall_t request, CallType call_type, bool backtracking)
{
  const smx_actor_t issuer                                     = api::get().simcall_get_issuer(request);
  const mc::PatternCommunicationList& initial_pattern          = initial_communications_pattern[issuer->get_pid()];
  const std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[issuer->get_pid()];

  auto pattern   = std::make_unique<PatternCommunication>();
  pattern->index = initial_pattern.index_comm + incomplete_pattern.size();

  if (call_type == CallType::SEND) {
    /* Create comm pattern */
    pattern->type      = PatternCommunicationType::send;
    pattern->comm_addr = api::get().get_comm_isend_raw_addr(request);
    pattern->rdv       = api::get().get_pattern_comm_rdv(pattern->comm_addr);
    pattern->src_proc  = api::get().get_pattern_comm_src_proc(pattern->comm_addr);
    pattern->src_host  = &api::get().get_actor_host_name(issuer);

#if HAVE_SMPI
    pattern->tag = api::get().get_smpi_request_tag(request, simgrid::simix::Simcall::COMM_ISEND);
#endif
    pattern->data = api::get().get_pattern_comm_data(pattern->comm_addr);

#if HAVE_SMPI
    auto send_detached = api::get().check_send_request_detached(request);
    if (send_detached) {
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
  } else if (call_type == CallType::RECV) {
    pattern->type = PatternCommunicationType::receive;
    pattern->comm_addr = api::get().get_comm_isend_raw_addr(request);

#if HAVE_SMPI
    pattern->tag = api::get().get_smpi_request_tag(request, simgrid::simix::Simcall::COMM_IRECV);
#endif
    pattern->rdv = api::get().get_pattern_comm_rdv(pattern->comm_addr);
    pattern->dst_proc = api::get().get_pattern_comm_dst_proc(pattern->comm_addr);
    pattern->dst_host = &api::get().get_actor_host_name(issuer);
  } else
    xbt_die("Unexpected call_type %i", (int)call_type);

  XBT_DEBUG("Insert incomplete comm pattern %p for process %ld", pattern.get(), issuer->get_pid());
  incomplete_communications_pattern[issuer->get_pid()].push_back(pattern.release());
}

void CommunicationDeterminismChecker::complete_comm_pattern(RemotePtr<kernel::activity::CommImpl> const& comm_addr,
                                                            aid_t issuer, bool backtracking)
{
  /* Complete comm pattern */
  std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[issuer];
  auto current_comm_pattern =
      std::find_if(begin(incomplete_pattern), end(incomplete_pattern),
                   [&comm_addr](const PatternCommunication* comm) { return (comm->comm_addr == comm_addr); });
  xbt_assert(current_comm_pattern != std::end(incomplete_pattern), "Corresponding communication not found!");

  update_comm_pattern(*current_comm_pattern, comm_addr);
  std::unique_ptr<PatternCommunication> comm_pattern(*current_comm_pattern);
  XBT_DEBUG("Remove incomplete comm pattern for process %ld at cursor %zd", issuer,
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

CommunicationDeterminismChecker::CommunicationDeterminismChecker(Session* session) : Checker(session) {}

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
  for (auto const& state : stack_)
    trace.push_back(state->get_transition()->to_string());
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
  XBT_INFO("Expanded states = %ld", State::get_expanded_states());
  XBT_INFO("Visited states = %lu", api::get().mc_get_visited_states());
  XBT_INFO("Executed transitions = %lu", Transition::get_executed_transitions());
  XBT_INFO("Send-deterministic : %s", this->send_deterministic ? "Yes" : "No");
  if (_sg_mc_comms_determinism)
    XBT_INFO("Recv-deterministic : %s", this->recv_deterministic ? "Yes" : "No");
}

void CommunicationDeterminismChecker::prepare()
{
  const unsigned long maxpid = api::get().get_maxpid();

  initial_communications_pattern.resize(maxpid);
  incomplete_communications_pattern.resize(maxpid);

  auto initial_state = std::make_unique<State>();

  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Add all enabled actors to the interleave set of the initial state */
  for (auto& act : api::get().get_actors()) {
    auto actor = act.copy.get_buffer();
    if (get_session().actor_is_enabled(actor->get_pid()))
      initial_state->mark_todo(actor->get_pid());
  }

  stack_.push_back(std::move(initial_state));
}

static inline bool all_communications_are_finished()
{
  const unsigned long maxpid = api::get().get_maxpid();
  for (size_t current_actor = 1; current_actor < maxpid; current_actor++) {
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
    api::get().restore_state(last_state->system_state_);
    restore_communications_pattern(last_state);
    return;
  }

  get_session().restore_initial_state();

  const unsigned long maxpid = api::get().get_maxpid();
  assert(maxpid == incomplete_communications_pattern.size());
  assert(maxpid == initial_communications_pattern.size());
  for (unsigned long j = 0; j < maxpid; j++) {
    incomplete_communications_pattern[j].clear();
    initial_communications_pattern[j].index_comm = 0;
  }

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack_) {
    if (state == stack_.back())
      break;

    int req_num                    = state->get_transition()->times_considered_;
    const s_smx_simcall* saved_req = &state->executed_req_;
    xbt_assert(saved_req);

    /* because we got a copy of the executed request, we have to fetch the
       real one, pointed by the request field of the issuer process */

    const smx_actor_t issuer = api::get().simcall_get_issuer(saved_req);
    smx_simcall_t req        = &issuer->simcall_;

    /* TODO : handle test and testany simcalls */
    CallType call = MC_get_call_type(req);
    state->get_transition()->replay();
    handle_comm_pattern(call, req, req_num, 1);

    /* Update statistics */
    api::get().mc_inc_visited_states();
  }
}

void CommunicationDeterminismChecker::handle_comm_pattern(simgrid::mc::CallType call_type, smx_simcall_t req, int value,
                                                          bool backtracking)
{
  using simgrid::mc::CallType;
  switch(call_type) {
    case CallType::NONE:
      break;
    case CallType::SEND:
    case CallType::RECV:
      get_comm_pattern(req, call_type, backtracking);
      break;
    case CallType::WAIT:
    case CallType::WAITANY: {
      RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr;
      if (call_type == CallType::WAIT)
        comm_addr = remote(simcall_comm_wait__getraw__comm(req));
      else
        comm_addr = api::get().get_comm_waitany_raw_addr(req, value);
      auto simcall_issuer = api::get().simcall_get_issuer(req);
      complete_comm_pattern(comm_addr, simcall_issuer->get_pid(), backtracking);
    } break;
  default:
    xbt_die("Unexpected call type %i", (int)call_type);
  }
}

void CommunicationDeterminismChecker::real_run()
{
  std::unique_ptr<VisitedState> visited_state = nullptr;

  while (not stack_.empty()) {
    /* Get current state */
    State* cur_state = stack_.back().get();

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth = %zu (state = %ld, interleaved processes = %zu)", stack_.size(), cur_state->num_,
              cur_state->count_todo());

    /* Update statistics */
    api::get().mc_inc_visited_states();

    int next_transition = -1;
    if (stack_.size() <= (std::size_t)_sg_mc_max_depth)
      next_transition = cur_state->next_transition();

    if (next_transition >= 0 && visited_state == nullptr) {
      cur_state->execute_next(next_transition);

      aid_t aid         = cur_state->get_transition()->aid_;
      int req_num       = cur_state->get_transition()->times_considered_;
      smx_simcall_t req = &cur_state->executed_req_;

      XBT_DEBUG("Execute: %s", cur_state->get_transition()->to_cstring());

      std::string req_str;
      if (dot_output != nullptr)
        req_str = api::get().request_get_dot_output(aid, req_num);

      /* TODO : handle test and testany simcalls */
      CallType call = CallType::NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      handle_comm_pattern(call, req, req_num, 0);

      /* Create the new expanded state */
      auto next_state = std::make_unique<State>();

      /* If comm determinism verification, we cannot stop the exploration if some communications are not finished (at
       * least, data are transferred). These communications  are incomplete and they cannot be analyzed and compared
       * with the initial pattern. */
      bool compare_snapshots = this->initial_communications_pattern_done && all_communications_are_finished();

      if (_sg_mc_max_visited_states != 0)
        visited_state = visited_states_.addVisitedState(next_state->num_, next_state.get(), compare_snapshots);
      else
        visited_state = nullptr;

      if (visited_state == nullptr) {
        /* Add all enabled actors to the interleave set of the next state */
        for (auto& act : api::get().get_actors()) {
          auto actor = act.copy.get_buffer();
          if (get_session().actor_is_enabled(actor->get_pid()))
            next_state->mark_todo(actor->get_pid());
        }

        if (dot_output != nullptr)
          fprintf(dot_output, "\"%ld\" -> \"%ld\" [%s];\n", cur_state->num_, next_state->num_, req_str.c_str());

      } else if (dot_output != nullptr)
        fprintf(dot_output, "\"%ld\" -> \"%ld\" [%s];\n", cur_state->num_,
                visited_state->original_num == -1 ? visited_state->num : visited_state->original_num, req_str.c_str());

      stack_.push_back(std::move(next_state));
    } else {
      if (stack_.size() > (std::size_t)_sg_mc_max_depth)
        XBT_WARN("/!\\ Max depth reached! /!\\ ");
      else if (visited_state != nullptr)
        XBT_DEBUG("State already visited (equal to state %ld), exploration stopped on this path.",
                  visited_state->original_num == -1 ? visited_state->num : visited_state->original_num);
      else
        XBT_DEBUG("There are no more processes to interleave. (depth %zu)", stack_.size());

      this->initial_communications_pattern_done = true;

      /* Trash the current state, no longer needed */
      XBT_DEBUG("Delete state %ld at depth %zu", cur_state->num_, stack_.size());
      stack_.pop_back();

      visited_state = nullptr;

      api::get().mc_check_deadlock();

      while (not stack_.empty()) {
        std::unique_ptr<State> state(std::move(stack_.back()));
        stack_.pop_back();
        if (state->count_todo() && stack_.size() < (std::size_t)_sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %ld at depth %zu", state->num_, stack_.size() + 1);
          stack_.push_back(std::move(state));

          this->restoreState();

          XBT_DEBUG("Back-tracking to state %ld at depth %zu done", stack_.back()->num_, stack_.size());

          break;
        } else {
          XBT_DEBUG("Delete state %ld at depth %zu", state->num_, stack_.size() + 1);
        }
      }
    }
  }

  api::get().log_state();
}

void CommunicationDeterminismChecker::run()
{
  XBT_INFO("Check communication determinism");
  get_session().take_initial_snapshot();

  this->prepare();
  this->real_run();
}

Checker* create_communication_determinism_checker(Session* session)
{
  return new CommunicationDeterminismChecker(session);
}

} // namespace mc
} // namespace simgrid
