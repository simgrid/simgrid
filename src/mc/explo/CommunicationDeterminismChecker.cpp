/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/explo/SafetyChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"

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
  int num                       = 0;
  uintptr_t comm_addr           = 0;
  PatternCommunicationType type = PatternCommunicationType::send;
  unsigned long src_proc        = 0;
  unsigned long dst_proc        = 0;
  unsigned mbox                 = 0;
  unsigned size                 = 0;
  int tag                       = 0;
  int index                     = 0;

  PatternCommunication dup() const
  {
    simgrid::mc::PatternCommunication res;
    // num?
    res.comm_addr = this->comm_addr;
    res.type      = this->type;
    res.src_proc  = this->src_proc;
    res.dst_proc  = this->dst_proc;
    res.mbox      = this->mbox;
    res.tag       = this->tag;
    res.index     = this->index;
    return res;
  }
};

struct PatternCommunicationList {
  unsigned int index_comm = 0;
  std::vector<std::unique_ptr<simgrid::mc::PatternCommunication>> list;
};

/********** Checker extension **********/

struct CommDetExtension {
  static simgrid::xbt::Extension<simgrid::mc::Exploration, CommDetExtension> EXTENSION_ID;

  std::vector<simgrid::mc::PatternCommunicationList> initial_communications_pattern;
  std::vector<std::vector<simgrid::mc::PatternCommunication*>> incomplete_communications_pattern;

  bool initial_communications_pattern_done = false;
  bool recv_deterministic                  = true;
  bool send_deterministic                  = true;
  std::string send_diff;
  std::string recv_diff;

  void exploration_start()
  {
    const unsigned long maxpid = api::get().get_maxpid();

    initial_communications_pattern.resize(maxpid);
    incomplete_communications_pattern.resize(maxpid);
  }
  void restore_communications_pattern(const simgrid::mc::State* state);
  void enforce_deterministic_pattern(aid_t process, const PatternCommunication* comm);
  void get_comm_pattern(const Transition* transition);
  void complete_comm_pattern(const CommWaitTransition* transition);
  void handle_comm_pattern(const Transition* transition);
};
simgrid::xbt::Extension<simgrid::mc::Exploration, CommDetExtension> CommDetExtension::EXTENSION_ID;
/********** State Extension ***********/

class StateCommDet {
  CommDetExtension* checker_;

public:
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern_;
  std::vector<unsigned> communication_indices_;

  static simgrid::xbt::Extension<simgrid::mc::State, StateCommDet> EXTENSION_ID;
  explicit StateCommDet(CommDetExtension* checker) : checker_(checker)
  {
    const unsigned long maxpid = api::get().get_maxpid();
    for (unsigned long i = 0; i < maxpid; i++) {
      std::vector<simgrid::mc::PatternCommunication> res;
      for (auto const& comm : checker_->incomplete_communications_pattern[i])
        res.push_back(comm->dup());
      incomplete_comm_pattern_.push_back(std::move(res));
    }

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

static std::string print_determinism_result(simgrid::mc::CommPatternDifference diff, aid_t actor,
                                            const simgrid::mc::PatternCommunication* comm, unsigned int cursor)
{
  std::string type;
  std::string res;

  if (comm->type == simgrid::mc::PatternCommunicationType::send)
    type = xbt::string_printf("The send communications pattern of the actor %ld is different!", actor - 1);
  else
    type = xbt::string_printf("The recv communications pattern of the actor %ld is different!", actor - 1);

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

void CommDetExtension::enforce_deterministic_pattern(aid_t actor, const PatternCommunication* comm)
{
  PatternCommunicationList& list = initial_communications_pattern[actor];
  CommPatternDifference diff     = compare_comm_pattern(list.list[list.index_comm].get(), comm);

  if (diff != CommPatternDifference::NONE) {
    if (comm->type == PatternCommunicationType::send) {
      send_deterministic = false;
      send_diff          = print_determinism_result(diff, actor, comm, list.index_comm + 1);
    } else {
      recv_deterministic = false;
      if (recv_diff.empty())
        recv_diff = print_determinism_result(diff, actor, comm, list.index_comm + 1);
    }
    if (_sg_mc_send_determinism && not send_deterministic) {
      XBT_INFO("*********************************************************");
      XBT_INFO("***** Non-send-deterministic communications pattern *****");
      XBT_INFO("*********************************************************");
      XBT_INFO("%s", send_diff.c_str());
      session_singleton->log_state();
      api::get().mc_exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
    } else if (_sg_mc_comms_determinism && (not send_deterministic && not recv_deterministic)) {
      XBT_INFO("****************************************************");
      XBT_INFO("***** Non-deterministic communications pattern *****");
      XBT_INFO("****************************************************");
      if (not send_diff.empty())
        XBT_INFO("%s", send_diff.c_str());
      if (not recv_diff.empty())
        XBT_INFO("%s", recv_diff.c_str());
      session_singleton->log_state();
      api::get().mc_exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
    }
  }
}

/********** Non Static functions ***********/

void CommDetExtension::get_comm_pattern(const Transition* transition)
{
  const mc::PatternCommunicationList& initial_pattern          = initial_communications_pattern[transition->aid_];
  const std::vector<PatternCommunication*>& incomplete_pattern = incomplete_communications_pattern[transition->aid_];

  auto pattern   = std::make_unique<PatternCommunication>();
  pattern->index = initial_pattern.index_comm + incomplete_pattern.size();

  if (transition->type_ == Transition::Type::COMM_SEND) {
    auto* send = static_cast<const CommSendTransition*>(transition);

    pattern->type      = PatternCommunicationType::send;
    pattern->comm_addr = send->get_comm();
    pattern->tag       = send->get_tag();

    // FIXME: Detached sends should be enforced when the receive is waited

  } else if (transition->type_ == Transition::Type::COMM_RECV) {
    auto* recv = static_cast<const CommRecvTransition*>(transition);

    pattern->type      = PatternCommunicationType::receive;
    pattern->comm_addr = recv->get_comm();
    pattern->tag       = recv->get_tag();
  }

  XBT_DEBUG("Insert incomplete comm pattern %p type:%d for process %ld (comm: %" PRIxPTR ")", pattern.get(),
            (int)pattern->type, transition->aid_, pattern->comm_addr);
  incomplete_communications_pattern[transition->aid_].push_back(pattern.release());
}

void CommDetExtension::complete_comm_pattern(const CommWaitTransition* transition)
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
  comm_pattern->mbox     = transition->get_mailbox();

  XBT_DEBUG("Remove incomplete comm pattern for actor %ld at cursor %zd", transition->aid_,
            std::distance(begin(incomplete_pattern), current_comm_pattern));
  incomplete_pattern.erase(current_comm_pattern);

  if (initial_communications_pattern_done) {
    /* Evaluate comm determinism */
    enforce_deterministic_pattern(transition->aid_, comm_pattern.get());
    initial_communications_pattern[transition->aid_].index_comm++;
  } else {
    /* Store comm pattern */
    initial_communications_pattern[transition->aid_].list.push_back(std::move(comm_pattern));
  }
}

void CommDetExtension::handle_comm_pattern(const Transition* transition)
{
  if (not _sg_mc_comms_determinism && not _sg_mc_send_determinism)
    return;

  switch (transition->type_) {
    case Transition::Type::COMM_SEND:
      get_comm_pattern(transition);
      break;
    case Transition::Type::COMM_RECV:
      get_comm_pattern(transition);
      break;
    case Transition::Type::COMM_WAIT:
      complete_comm_pattern(static_cast<const CommWaitTransition*>(transition));
      break;
    case Transition::Type::WAITANY:
      // Ignore wait on non-comm
      if (auto const* wait = dynamic_cast<const CommWaitTransition*>(
              static_cast<const WaitAnyTransition*>(transition)->get_current_transition()))
        complete_comm_pattern(wait);
      break;
    default: /* Ignore unhandled transition types */
      break;
  }
}

/* FIXME: CommDet probably don't play nicely with stateful exploration

      bool all_communications_are_finished = true;
      for (size_t current_actor = 1; all_communications_are_finished && current_actor < maxpid; current_actor++) {
        if (not extension->incomplete_communications_pattern[current_actor].empty()) {
          XBT_DEBUG("Some communications are not finished, cannot stop the exploration! State not visited.");
          all_communications_are_finished = false;
        }
      }
 */

Exploration* create_communication_determinism_checker(Session* session)
{
  CommDetExtension::EXTENSION_ID = simgrid::mc::Exploration::extension_create<CommDetExtension>();
  StateCommDet::EXTENSION_ID     = simgrid::mc::State::extension_create<StateCommDet>();

  XBT_DEBUG("********* Start communication determinism verification *********");

  auto extension = new CommDetExtension();

  SafetyChecker::on_exploration_start([extension]() {
    XBT_INFO("Check communication determinism");
    extension->exploration_start();
  });
  SafetyChecker::on_backtracking([extension]() { extension->initial_communications_pattern_done = true; });
  SafetyChecker::on_state_creation([extension](State* state) { state->extension_set(new StateCommDet(extension)); });

  SafetyChecker::on_restore_system_state(
      [extension](State* state) { extension->restore_communications_pattern(state); });

  SafetyChecker::on_restore_initial_state([extension]() {
    const unsigned long maxpid = api::get().get_maxpid();
    assert(maxpid == extension->incomplete_communications_pattern.size());
    assert(maxpid == extension->initial_communications_pattern.size());
    for (unsigned long j = 0; j < maxpid; j++) {
      extension->incomplete_communications_pattern[j].clear();
      extension->initial_communications_pattern[j].index_comm = 0;
    }
  });

  SafetyChecker::on_transition_replay([extension](Transition* t) { extension->handle_comm_pattern(t); });
  SafetyChecker::on_transition_execute([extension](Transition* t) { extension->handle_comm_pattern(t); });

  SafetyChecker::on_log_state([extension]() {
    if (_sg_mc_comms_determinism) {
      if (extension->send_deterministic && not extension->recv_deterministic) {
        XBT_INFO("*******************************************************");
        XBT_INFO("**** Only-send-deterministic communication pattern ****");
        XBT_INFO("*******************************************************");
        XBT_INFO("%s", extension->recv_diff.c_str());
      }
      if (not extension->send_deterministic && extension->recv_deterministic) {
        XBT_INFO("*******************************************************");
        XBT_INFO("**** Only-recv-deterministic communication pattern ****");
        XBT_INFO("*******************************************************");
        XBT_INFO("%s", extension->send_diff.c_str());
      }
    }
    XBT_INFO("Send-deterministic : %s", extension->send_deterministic ? "Yes" : "No");
    if (_sg_mc_comms_determinism)
      XBT_INFO("Recv-deterministic : %s", extension->recv_deterministic ? "Yes" : "No");
    delete extension;
  });

  return new SafetyChecker(session);
}

} // namespace mc
} // namespace simgrid
