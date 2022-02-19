/* Copyright (c) 2020-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "api.hpp"

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_pattern.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/remote/RemoteProcess.hpp"
#include "src/surf/HostImpl.hpp"

#include <xbt/asserts.h>
#include <xbt/log.h>
#include "simgrid/s4u/Host.hpp"
#include "xbt/string.hpp"
#if HAVE_SMPI
#include "src/smpi/include/smpi_request.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Api, mc, "Logging specific to MC Facade APIs ");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

using Simcall = simgrid::simix::Simcall;

namespace simgrid {
namespace mc {

/** Statically "upcast" a s_smx_actor_t into an ActorInformation
 *
 *  This gets 'actorInfo' from '&actorInfo->copy'. It upcasts in the
 *  sense that we could achieve the same thing by having ActorInformation
 *  inherit from s_smx_actor_t but we don't really want to do that.
 */
static simgrid::mc::ActorInformation* actor_info_cast(smx_actor_t actor)
{
  simgrid::mc::ActorInformation temp;
  std::size_t offset = (char*)temp.copy.get_buffer() - (char*)&temp;

  auto* process_info = reinterpret_cast<simgrid::mc::ActorInformation*>((char*)actor - offset);
  return process_info;
}

xbt::string const& Api::get_actor_host_name(smx_actor_t actor) const
{
  if (mc_model_checker == nullptr)
    return actor->get_host()->get_name();

  const simgrid::mc::RemoteProcess* process = &mc_model_checker->get_remote_process();

  // Read the simgrid::xbt::string in the MCed process:
  simgrid::mc::ActorInformation* info = actor_info_cast(actor);

  if (not info->hostname) {
    Remote<s4u::Host> temp_host = process->read(remote(actor->get_host()));
    auto remote_string_address  = remote(&xbt::string::to_string_data(temp_host.get_buffer()->get_impl()->get_name()));
    simgrid::xbt::string_data remote_string = process->read(remote_string_address);
    std::vector<char> hostname(remote_string.len + 1);
    // no need to read the terminating null byte, and thus hostname[remote_string.len] is guaranteed to be '\0'
    process->read_bytes(hostname.data(), remote_string.len, remote(remote_string.data));
    info->hostname = &mc_model_checker->get_host_name(hostname.data());
  }
  return *info->hostname;
}

xbt::string const& Api::get_actor_name(smx_actor_t actor) const
{
  if (mc_model_checker == nullptr)
    return actor->get_name();

  simgrid::mc::ActorInformation* info = actor_info_cast(actor);
  if (info->name.empty()) {
    const simgrid::mc::RemoteProcess* process = &mc_model_checker->get_remote_process();

    simgrid::xbt::string_data string_data = simgrid::xbt::string::to_string_data(actor->name_);
    info->name = process->read_string(remote(string_data.data), string_data.len);
  }
  return info->name;
}

simgrid::mc::Checker* Api::initialize(char** argv, simgrid::mc::CheckerAlgorithm algo) const
{
  auto session = new simgrid::mc::Session([argv] {
    int i = 1;
    while (argv[i] != nullptr && argv[i][0] == '-')
      i++;
    xbt_assert(argv[i] != nullptr,
               "Unable to find a binary to exec on the command line. Did you only pass config flags?");
    execvp(argv[i], argv + i);
    xbt_die("The model-checked process failed to exec(%s): %s", argv[i], strerror(errno));
  });

  simgrid::mc::Checker* checker;
  switch (algo) {
    case CheckerAlgorithm::CommDeterminism:
      checker = simgrid::mc::create_communication_determinism_checker(session);
      break;

    case CheckerAlgorithm::UDPOR:
      checker = simgrid::mc::create_udpor_checker(session);
      break;

    case CheckerAlgorithm::Safety:
      checker = simgrid::mc::create_safety_checker(session);
      break;

    case CheckerAlgorithm::Liveness:
      checker = simgrid::mc::create_liveness_checker(session);
      break;

    default:
      THROW_IMPOSSIBLE;
  }

  // FIXME: session and checker are never deleted
  simgrid::mc::session_singleton = session;
  mc_model_checker->setChecker(checker);
  return checker;
}

std::vector<simgrid::mc::ActorInformation>& Api::get_actors() const
{
  return mc_model_checker->get_remote_process().actors();
}

unsigned long Api::get_maxpid() const
{
  return mc_model_checker->get_remote_process().get_maxpid();
}

std::size_t Api::get_remote_heap_bytes() const
{
  RemoteProcess& process    = mc_model_checker->get_remote_process();
  auto heap_bytes_used      = mmalloc_get_bytes_used_remote(process.get_heap()->heaplimit, process.get_malloc_info());
  return heap_bytes_used;
}

void Api::mc_inc_visited_states() const
{
  mc_model_checker->visited_states++;
}

unsigned long Api::mc_get_visited_states() const
{
  return mc_model_checker->visited_states;
}

void Api::mc_exit(int status) const
{
  mc_model_checker->exit(status);
}

void Api::restore_state(std::shared_ptr<simgrid::mc::Snapshot> system_state) const
{
  system_state->restore(&mc_model_checker->get_remote_process());
}

bool Api::snapshot_equal(const Snapshot* s1, const Snapshot* s2) const
{
  return simgrid::mc::snapshot_equal(s1, s2);
}

simgrid::mc::Snapshot* Api::take_snapshot(long num_state) const
{
  auto snapshot = new simgrid::mc::Snapshot(num_state);
  return snapshot;
}

void Api::s_close() const
{
  session_singleton->close();
}

void Api::automaton_load(const char* file) const
{
  if (simgrid::mc::property_automaton == nullptr)
    simgrid::mc::property_automaton = xbt_automaton_new();

  xbt_automaton_load(simgrid::mc::property_automaton, file);
}

std::vector<int> Api::automaton_propositional_symbol_evaluate() const
{
  unsigned int cursor = 0;
  std::vector<int> values;
  xbt_automaton_propositional_symbol_t ps = nullptr;
  xbt_dynar_foreach (mc::property_automaton->propositional_symbols, cursor, ps)
    values.push_back(xbt_automaton_propositional_symbol_evaluate(ps));
  return values;
}

std::vector<xbt_automaton_state_t> Api::get_automaton_state() const
{
  std::vector<xbt_automaton_state_t> automaton_stack;
  unsigned int cursor = 0;
  xbt_automaton_state_t automaton_state;
  xbt_dynar_foreach (mc::property_automaton->states, cursor, automaton_state)
    if (automaton_state->type == -1)
      automaton_stack.push_back(automaton_state);
  return automaton_stack;
}

int Api::compare_automaton_exp_label(const xbt_automaton_exp_label* l) const
{
  unsigned int cursor                    = 0;
  xbt_automaton_propositional_symbol_t p = nullptr;
  xbt_dynar_foreach (simgrid::mc::property_automaton->propositional_symbols, cursor, p) {
    if (std::strcmp(xbt_automaton_propositional_symbol_get_name(p), l->u.predicat) == 0)
      return cursor;
  }
  return -1;
}

void Api::set_property_automaton(xbt_automaton_state_t const& automaton_state) const
{
  mc::property_automaton->current_state = automaton_state;
}

xbt_automaton_exp_label_t Api::get_automaton_transition_label(xbt_dynar_t const& dynar, int index) const
{
  const xbt_automaton_transition* transition = xbt_dynar_get_as(dynar, index, xbt_automaton_transition_t);
  return transition->label;
}

xbt_automaton_state_t Api::get_automaton_transition_dst(xbt_dynar_t const& dynar, int index) const
{
  const xbt_automaton_transition* transition = xbt_dynar_get_as(dynar, index, xbt_automaton_transition_t);
  return transition->dst;
}

} // namespace mc
} // namespace simgrid
