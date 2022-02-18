/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/api/TransitionComm.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"

#include <cstdint>

using api = simgrid::mc::Api;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc, "Logging specific to MC communication determinism detection");

namespace simgrid {
namespace mc {

enum class CallType { NONE, SEND, RECV, WAIT, WAITANY };
enum class CommPatternDifference { NONE, TYPE, MBOX, TAG, SRC_PROC, DST_PROC, DATA_SIZE };
enum class PatternCommunicationType {
  none    = 0,
  send    = 1,
  receive = 2,
};

class PatternCommunication {
public:
  int num = 0;
  uintptr_t comm_addr           = 0;
  PatternCommunicationType type = PatternCommunicationType::send;
  unsigned long src_proc        = 0;
  unsigned long dst_proc        = 0;
  unsigned mbox                 = 0;
  unsigned size                 = 0;
  int tag   = 0;
  int index = 0;

  PatternCommunication dup() const
  {
    simgrid::mc::PatternCommunication res;
    // num?
    res.comm_addr = this->comm_addr;
    res.type      = this->type;
    res.src_proc  = this->src_proc;
    res.dst_proc = this->dst_proc;
    res.mbox     = this->mbox;
    res.tag       = this->tag;
    res.index = this->index;
    return res;
  }
};

struct PatternCommunicationList {
  unsigned int index_comm = 0;
  std::vector<std::unique_ptr<simgrid::mc::PatternCommunication>> list;
};

/********** Checker extension **********/

struct CommDetExtension {
  static simgrid::xbt::Extension<simgrid::mc::Checker, CommDetExtension> EXTENSION_ID;

  CommDetExtension()
  {
    const unsigned long maxpid = api::get().get_maxpid();

    initial_communications_pattern.resize(maxpid);
    incomplete_communications_pattern.resize(maxpid);
  }

  std::vector<simgrid::mc::PatternCommunicationList> initial_communications_pattern;
  std::vector<std::vector<simgrid::mc::PatternCommunication*>> incomplete_communications_pattern;

  bool initial_communications_pattern_done = false;
  bool recv_deterministic                  = true;
  bool send_deterministic                  = true;
  std::string send_diff;
  std::string recv_diff;

  void restore_communications_pattern(const simgrid::mc::State* state);
  void deterministic_comm_pattern(aid_t process, const PatternCommunication* comm, bool backtracking);
  void get_comm_pattern(const Transition* transition, bool backtracking);
  void complete_comm_pattern(const CommWaitTransition* transition, bool backtracking);
  void handle_comm_pattern(const Transition* transition, bool backtracking);
};
simgrid::xbt::Extension<simgrid::mc::Checker, CommDetExtension> CommDetExtension::EXTENSION_ID;
/********** State Extension ***********/

class StateCommDet {
  CommDetExtension* checker_;

public:
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern_;
  std::vector<unsigned> communication_indices_;

  static simgrid::xbt::Extension<simgrid::mc::State, StateCommDet> EXTENSION_ID;
  explicit StateCommDet(CommDetExtension* checker) : checker_(checker)
  {
    copy_incomplete_comm_pattern();
    copy_index_comm_pattern();
  }

  void copy_incomplete_comm_pattern()
  {
    incomplete_comm_pattern_.clear();
    const unsigned long maxpid = api::get().get_maxpid();
    for (unsigned long i = 0; i < maxpid; i++) {
      std::vector<simgrid::mc::PatternCommunication> res;
      for (auto const& comm : checker_->incomplete_communications_pattern[i])
        res.push_back(comm->dup());
      incomplete_comm_pattern_.push_back(std::move(res));
    }
  }

  void copy_index_comm_pattern()
  {
    communication_indices_.clear();
    for (auto const& list_process_comm : checker_->initial_communications_pattern)
      this->communication_indices_.push_back(list_process_comm.index_comm);
  }
};
simgrid::xbt::Extension<simgrid::mc::State, StateCommDet> StateCommDet::EXTENSION_ID;

static simgrid::mc::CommPatternDifference compare_comm_pattern(const simgrid::mc::PatternCommunication* comm1,
                                                               const simgrid::mc::PatternCommunication* comm2)
{
  using simgrid::mc::CommPatternDifference;
  if (comm1->type != comm2->type)
    return CommPatternDifference::TYPE;
  if (comm1->mbox != comm2->mbox)
    return CommPatternDifference::MBOX;
  if (comm1->src_proc != comm2->src_proc)
    return CommPatternDifference::SRC_PROC;
  if (comm1->dst_proc != comm2->dst_proc)
    return CommPatternDifference::DST_PROC;
  if (comm1->tag != comm2->tag)
    return CommPatternDifference::TAG;
  if (comm1->size != comm2->size)
    return CommPatternDifference::DATA_SIZE;
  return CommPatternDifference::NONE;
}

void CommDetExtension::restore_communications_pattern(const simgrid::mc::State* state)
{
  for (size_t i = 0; i < initial_communications_pattern.size(); i++)
    initial_communications_pattern[i].index_comm =
        state->extension<simgrid::mc::StateCommDet>()->communication_indices_[i];

  const unsigned long maxpid = api::get().get_maxpid();
  for (unsigned long i = 0; i < maxpid; i++) {
    incomplete_communications_pattern[i].clear();
    for (simgrid::mc::PatternCommunication const& comm :
         state->extension<simgrid::mc::StateCommDet>()->incomplete_comm_pattern_[i])
      incomplete_communications_pattern[i].push_back(new simgrid::mc::PatternCommunication(comm.dup()));
  }
}

static std::string print_determinism_result(simgrid::mc::CommPatternDifference diff, aid_t process,
                                            const simgrid::mc::PatternCommunication* comm, unsigned int cursor)
{
  std::string type;
  std::string res;

  if (comm->type == simgrid::mc::PatternCommunicationType::send)
    type = xbt::string_printf("The send communications pattern of the process %ld is different!", process - 1);
  else
    type = xbt::string_printf("The recv communications pattern of the process %ld is different!", process - 1);

  using simgrid::mc::CommPatternDifference;
  switch (diff) {
    case CommPatternDifference::TYPE:
      res = xbt::string_printf("%s Different type for communication #%u", type.c_str(), cursor);
      break;
    case CommPatternDifference::MBOX:
      res = xbt::string_printf("%s Different mailbox for communication #%u", type.c_str(), cursor);
      break;
    case CommPatternDifference::TAG:
      res = xbt::string_printf("%s Different tag for communication #%u", type.c_str(), cursor);
      break;
    case CommPatternDifference::SRC_PROC:
      res = xbt::string_printf("%s Different source for communication #%u", type.c_str(), cursor);
      break;
    case CommPatternDifference::DST_PROC:
      res = xbt::string_printf("%s Different destination for communication #%u", type.c_str(), cursor);
      break;
    case CommPatternDifference::DATA_SIZE:
      res = xbt::string_printf("%s Different data size for communication #%u", type.c_str(), cursor);
      break;
    default:
      res = "";
      break;
  }

  return res;
}

void CommDetExtension::deterministic_comm_pattern(aid_t actor, const PatternCommunication* comm, bool backtracking)
{
  if (not backtracking) {
    PatternCommunicationList& list = initial_communications_pattern[actor];
    CommPatternDifference diff     = compare_comm_pattern(list.list[list.index_comm].get(), comm);

    if (diff != CommPatternDifference::NONE) {
      if (comm->type == PatternCommunicationType::send) {
        send_deterministic = false;
        send_diff          = print_determinism_result(diff, actor, comm, list.index_comm + 1);
      } else {
        recv_deterministic = false;
        recv_diff          = print_determinism_result(diff, actor, comm, list.index_comm + 1);
      }
      if (_sg_mc_send_determinism && not send_deterministic) {
        XBT_INFO("*********************************************************");
        XBT_INFO("***** Non-send-deterministic communications pattern *****");
        XBT_INFO("*********************************************************");
        XBT_INFO("%s", send_diff.c_str());
        api::get().log_state();
        api::get().mc_exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      } else if (_sg_mc_comms_determinism && (not send_deterministic && not recv_deterministic)) {
        XBT_INFO("****************************************************");
        XBT_INFO("***** Non-deterministic communications pattern *****");
        XBT_INFO("****************************************************");
        if (not send_diff.empty())
          XBT_INFO("%s", send_diff.c_str());
        if (not recv_diff.empty())
          XBT_INFO("%s", recv_diff.c_str());
        api::get().log_state();
        api::get().mc_exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      }
    }
  }
}

/********** Non Static functions ***********/

void CommDetExtension::get_comm_pattern(const Transition* transition, bool backtracking)
{
  const mc::PatternCommunicationList& initial_pattern          = initial_communications_pattern[transition->aid_];
  const std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[transition->aid_];

  auto pattern   = std::make_unique<PatternCommunication>();
  pattern->index = initial_pattern.index_comm + incomplete_pattern.size();

  if (transition->type_ == Transition::Type::COMM_SEND) {
    auto* send = dynamic_cast<const CommSendTransition*>(transition);
    /* Create comm pattern */
    pattern->type      = PatternCommunicationType::send;
    pattern->comm_addr = send->get_comm();
    pattern->mbox      = send->get_mailbox();
    pattern->src_proc  = send->aid_;
    pattern->tag       = send->get_tag();

#if HAVE_SMPI
    // auto send_detached = api::get().check_send_request_detached(request);
    if (false) { // send_detached) {
      if (initial_communications_pattern_done) {
        /* Evaluate comm determinism */
        deterministic_comm_pattern(pattern->src_proc, pattern.get(), backtracking);
        initial_communications_pattern[pattern->src_proc].index_comm++;
      } else {
        /* Store comm pattern */
        initial_communications_pattern[pattern->src_proc].list.push_back(std::move(pattern));
      }
      return;
    }
#endif
  } else if (transition->type_ == Transition::Type::COMM_RECV) {
    auto* recv = static_cast<const CommRecvTransition*>(transition);

    pattern->type = PatternCommunicationType::receive;
    pattern->comm_addr = recv->get_comm();
    pattern->tag       = recv->get_tag();
    pattern->mbox     = recv->get_mailbox();
    pattern->dst_proc = recv->aid_;
  }

  XBT_DEBUG("Insert incomplete comm pattern %p type:%d for process %ld (comm: %" PRIxPTR ")", pattern.get(),
            (int)pattern->type, transition->aid_, pattern->comm_addr);
  incomplete_communications_pattern[transition->aid_].push_back(pattern.release());
}

void CommDetExtension::complete_comm_pattern(const CommWaitTransition* transition, bool backtracking)
{
  /* Complete comm pattern */
  std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[transition->aid_];
  uintptr_t comm_addr                                    = transition->get_comm();
  auto current_comm_pattern =
      std::find_if(begin(incomplete_pattern), end(incomplete_pattern),
                   [&comm_addr](const PatternCommunication* comm) { return (comm->comm_addr == comm_addr); });
  xbt_assert(current_comm_pattern != std::end(incomplete_pattern), "Corresponding communication not found!");
  std::unique_ptr<PatternCommunication> comm_pattern(*current_comm_pattern);

  comm_pattern->src_proc = transition->get_sender();
  comm_pattern->dst_proc = transition->get_receiver();

  XBT_DEBUG("Remove incomplete comm pattern for actor %ld at cursor %zd", transition->aid_,
            std::distance(begin(incomplete_pattern), current_comm_pattern));
  incomplete_pattern.erase(current_comm_pattern);

  if (initial_communications_pattern_done) {
    /* Evaluate comm determinism */
    deterministic_comm_pattern(transition->aid_, comm_pattern.get(), backtracking);
    initial_communications_pattern[transition->aid_].index_comm++;
  } else {
    /* Store comm pattern */
    initial_communications_pattern[transition->aid_].list.push_back(std::move(comm_pattern));
  }
}

CommunicationDeterminismChecker::CommunicationDeterminismChecker(Session* session) : Checker(session)
{
  CommDetExtension::EXTENSION_ID = simgrid::mc::Checker::extension_create<CommDetExtension>();
  StateCommDet::EXTENSION_ID = simgrid::mc::State::extension_create<StateCommDet>();
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
  for (auto const& state : stack_)
    trace.push_back(state->get_transition()->to_string());
  return trace;
}

void CommunicationDeterminismChecker::log_state() // override
{
  if (_sg_mc_comms_determinism) {
    if (extension<CommDetExtension>()->send_deterministic && not extension<CommDetExtension>()->recv_deterministic) {
      XBT_INFO("*******************************************************");
      XBT_INFO("**** Only-send-deterministic communication pattern ****");
      XBT_INFO("*******************************************************");
      XBT_INFO("%s", extension<CommDetExtension>()->recv_diff.c_str());
    }
    if (not extension<CommDetExtension>()->send_deterministic && extension<CommDetExtension>()->recv_deterministic) {
      XBT_INFO("*******************************************************");
      XBT_INFO("**** Only-recv-deterministic communication pattern ****");
      XBT_INFO("*******************************************************");
      XBT_INFO("%s", extension<CommDetExtension>()->send_diff.c_str());
    }
  }
  XBT_INFO("Expanded states = %ld", State::get_expanded_states());
  XBT_INFO("Visited states = %lu", api::get().mc_get_visited_states());
  XBT_INFO("Executed transitions = %lu", Transition::get_executed_transitions());
  XBT_INFO("Send-deterministic : %s", extension<CommDetExtension>()->send_deterministic ? "Yes" : "No");
  if (_sg_mc_comms_determinism)
    XBT_INFO("Recv-deterministic : %s", extension<CommDetExtension>()->recv_deterministic ? "Yes" : "No");
}

void CommunicationDeterminismChecker::prepare()
{
  auto initial_state = std::make_unique<State>();
  extension_set(new CommDetExtension());
  initial_state->extension_set(new StateCommDet(extension<CommDetExtension>()));

  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Add all enabled actors to the interleave set of the initial state */
  for (auto& act : api::get().get_actors()) {
    auto actor = act.copy.get_buffer();
    if (get_session().actor_is_enabled(actor->get_pid()))
      initial_state->mark_todo(actor->get_pid());
  }

  stack_.push_back(std::move(initial_state));
}

void CommunicationDeterminismChecker::restoreState()
{
  auto extension = this->extension<CommDetExtension>();

  /* If asked to rollback on a state that has a snapshot, restore it */
  State* last_state = stack_.back().get();
  if (last_state->system_state_) {
    api::get().restore_state(last_state->system_state_);
    extension->restore_communications_pattern(last_state);
    return;
  }

  /* if no snapshot, we need to restore the initial state and replay the transitions */
  get_session().restore_initial_state();

  const unsigned long maxpid = api::get().get_maxpid();
  assert(maxpid == extension->incomplete_communications_pattern.size());
  assert(maxpid == extension->initial_communications_pattern.size());
  for (unsigned long j = 0; j < maxpid; j++) {
    extension->incomplete_communications_pattern[j].clear();
    extension->initial_communications_pattern[j].index_comm = 0;
  }

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (std::unique_ptr<simgrid::mc::State> const& state : stack_) {
    if (state == stack_.back())
      break;

    auto* transition = state->get_transition();
    transition->replay();
    extension->handle_comm_pattern(transition, true);

    /* Update statistics */
    api::get().mc_inc_visited_states();
  }
}

void CommDetExtension::handle_comm_pattern(const Transition* transition, bool backtracking)
{
  if (not _sg_mc_comms_determinism && not _sg_mc_send_determinism)
    return;

  switch (transition->type_) {
    case Transition::Type::COMM_SEND:
      get_comm_pattern(transition, backtracking);
      break;
    case Transition::Type::COMM_RECV:
      get_comm_pattern(transition, backtracking);
      break;
    case Transition::Type::COMM_WAIT: {
      complete_comm_pattern(static_cast<const CommWaitTransition*>(transition), backtracking);
      break;
    }
    case Transition::Type::WAITANY: {
      auto const* t    = static_cast<const WaitAnyTransition*>(transition)->get_current_transition();
      auto const* wait = dynamic_cast<const CommWaitTransition*>(t);
      if (wait != nullptr) // Ignore wait on non-comm
        complete_comm_pattern(wait, backtracking);
    } break;
    default: /* Ignore unhandled transition types */
      break;
  }
}

void CommunicationDeterminismChecker::real_run()
{
  auto extension = this->extension<CommDetExtension>();

  std::unique_ptr<VisitedState> visited_state = nullptr;
  const unsigned long maxpid                  = api::get().get_maxpid();

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

      auto* transition  = cur_state->get_transition();
      XBT_DEBUG("Execute: %s", transition->to_string().c_str());

      std::string req_str;
      if (dot_output != nullptr)
        req_str = api::get().request_get_dot_output(transition);

      extension->handle_comm_pattern(transition, false);

      /* Create the new expanded state */
      auto next_state = std::make_unique<State>();
      next_state->extension_set(new StateCommDet(extension));

      bool all_communications_are_finished = true;
      for (size_t current_actor = 1; all_communications_are_finished && current_actor < maxpid; current_actor++) {
        if (not extension->incomplete_communications_pattern[current_actor].empty()) {
          XBT_DEBUG("Some communications are not finished, cannot stop the exploration! State not visited.");
          all_communications_are_finished = false;
        }
      }

      /* If comm determinism verification, we cannot stop the exploration if some communications are not finished (at
       * least, data are transferred). These communications  are incomplete and they cannot be analyzed and compared
       * with the initial pattern. */
      bool compare_snapshots = extension->initial_communications_pattern_done && all_communications_are_finished;

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

      extension->initial_communications_pattern_done = true;

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
