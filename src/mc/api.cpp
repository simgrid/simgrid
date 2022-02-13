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
#include "src/mc/mc_comm_pattern.hpp"
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
simgrid::mc::ActorInformation* Api::actor_info_cast(smx_actor_t actor) const
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

std::string Api::get_actor_string(smx_actor_t actor) const
{
  std::string res;
  if (actor) {
    res = "(" + std::to_string(actor->get_pid()) + ")";
    if (actor->get_host())
      res += std::string(get_actor_host_name(actor)) + " (" + std::string(get_actor_name(actor)) + ")";
    else
      res += get_actor_name(actor);
  } else
    res = "(0) ()";
  return res;
}

std::string Api::get_actor_dot_label(smx_actor_t actor) const
{
  std::string res = "(" + std::to_string(actor->get_pid()) + ")";
  if (actor->get_host())
    res += get_actor_host_name(actor);
  return res;
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

int Api::get_actors_size() const
{
  return mc_model_checker->get_remote_process().actors().size();
}

RemotePtr<kernel::activity::CommImpl> Api::get_comm_isend_raw_addr(smx_simcall_t request) const
{
  return remote(static_cast<kernel::activity::CommImpl*>(simcall_comm_isend__getraw__result(request)));
}

RemotePtr<kernel::activity::CommImpl> Api::get_comm_waitany_raw_addr(smx_simcall_t request, int value) const
{
  auto addr      = simcall_comm_waitany__getraw__comms(request) + value;
  auto comm_addr = mc_model_checker->get_remote_process().read(remote(addr));
  return RemotePtr<kernel::activity::CommImpl>(static_cast<kernel::activity::CommImpl*>(comm_addr));
}

std::string Api::get_pattern_comm_rdv(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  Remote<kernel::activity::CommImpl> temp_activity;
  mc_model_checker->get_remote_process().read(temp_activity, addr);
  const kernel::activity::CommImpl* activity = temp_activity.get_buffer();

  char* remote_name = mc_model_checker->get_remote_process().read<char*>(RemotePtr<char*>(
      (uint64_t)(activity->get_mailbox() ? &activity->get_mailbox()->get_name() : &activity->mbox_cpy->get_name())));
  auto rdv          = mc_model_checker->get_remote_process().read_string(RemotePtr<char>(remote_name));
  return rdv;
}

unsigned long Api::get_pattern_comm_src_proc(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  Remote<kernel::activity::CommImpl> temp_activity;
  mc_model_checker->get_remote_process().read(temp_activity, addr);
  const kernel::activity::CommImpl* activity = temp_activity.get_buffer();
  auto src_proc =
      mc_model_checker->get_remote_process().resolve_actor(mc::remote(activity->src_actor_.get()))->get_pid();
  return src_proc;
}

unsigned long Api::get_pattern_comm_dst_proc(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  Remote<kernel::activity::CommImpl> temp_activity;
  mc_model_checker->get_remote_process().read(temp_activity, addr);
  const kernel::activity::CommImpl* activity = temp_activity.get_buffer();
  auto src_proc =
      mc_model_checker->get_remote_process().resolve_actor(mc::remote(activity->dst_actor_.get()))->get_pid();
  return src_proc;
}

std::vector<char> Api::get_pattern_comm_data(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_process().read(temp_comm, addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  std::vector<char> buffer{};
  if (comm->src_buff_ != nullptr) {
    buffer.resize(comm->src_buff_size_);
    mc_model_checker->get_remote_process().read_bytes(buffer.data(), buffer.size(), remote(comm->src_buff_));
  }
  return buffer;
}

#if HAVE_SMPI
bool Api::check_send_request_detached(smx_simcall_t const& simcall) const
{
  Remote<simgrid::smpi::Request> mpi_request;
  mc_model_checker->get_remote_process().read(
      mpi_request, remote(static_cast<smpi::Request*>(simcall_comm_isend__get__data(simcall))));
  return mpi_request.get_buffer()->detached();
}
#endif

smx_actor_t Api::get_src_actor(RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_process().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto src_proc = mc_model_checker->get_remote_process().resolve_actor(simgrid::mc::remote(comm->src_actor_.get()));
  return src_proc;
}

smx_actor_t Api::get_dst_actor(RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_process().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto dst_proc = mc_model_checker->get_remote_process().resolve_actor(simgrid::mc::remote(comm->dst_actor_.get()));
  return dst_proc;
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

void Api::mc_check_deadlock() const
{
  if (mc_model_checker->checkDeadlock()) {
    XBT_CINFO(mc_global, "**************************");
    XBT_CINFO(mc_global, "*** DEADLOCK DETECTED ***");
    XBT_CINFO(mc_global, "**************************");
    XBT_CINFO(mc_global, "Counter-example execution trace:");
    for (auto const& s : mc_model_checker->getChecker()->get_textual_trace())
      XBT_CINFO(mc_global, "  %s", s.c_str());
    XBT_INFO("Path = %s", mc_model_checker->getChecker()->get_record_trace().to_string().c_str());
    simgrid::mc::session_singleton->log_state();
    throw DeadlockError();
  }
}

/** Get the issuer of a simcall (`req->issuer`)
 *
 *  In split-process mode, it does the black magic necessary to get an address
 *  of a (shallow) copy of the data structure the issuer SIMIX actor in the local
 *  address space.
 *
 *  @param process the MCed process
 *  @param req     the simcall (copied in the local process)
 */
smx_actor_t Api::simcall_get_issuer(s_smx_simcall const* req) const
{
  xbt_assert(mc_model_checker != nullptr);

  // This is the address of the smx_actor in the MCed process:
  auto address = simgrid::mc::remote(req->issuer_);

  // Lookup by address:
  for (auto& actor : mc_model_checker->get_remote_process().actors())
    if (actor.address == address)
      return actor.copy.get_buffer();
  for (auto& actor : mc_model_checker->get_remote_process().dead_actors())
    if (actor.address == address)
      return actor.copy.get_buffer();

  xbt_die("Issuer not found");
}

RemotePtr<kernel::activity::MailboxImpl> Api::get_mbox_remote_addr(smx_simcall_t const req) const
{
  if (req->call_ == Simcall::COMM_ISEND)
    return remote(simcall_comm_isend__get__mbox(req));
  if (req->call_ == Simcall::COMM_IRECV)
    return remote(simcall_comm_irecv__get__mbox(req));
  THROW_IMPOSSIBLE;
}

RemotePtr<kernel::activity::ActivityImpl> Api::get_comm_remote_addr(smx_simcall_t const req) const
{
  if (req->call_ == Simcall::COMM_ISEND)
    return remote(simcall_comm_isend__getraw__result(req));
  if (req->call_ == Simcall::COMM_IRECV)
    return remote(simcall_comm_irecv__getraw__result(req));
  THROW_IMPOSSIBLE;
}

void Api::mc_exit(int status) const
{
  mc_model_checker->exit(status);
}

std::string Api::request_get_dot_output(const Transition* t) const
{
  static constexpr std::array<const char*, 13> colors{{"blue", "red", "green3", "goldenrod", "brown", "purple",
                                                       "magenta", "turquoise4", "gray25", "forestgreen", "hotpink",
                                                       "lightblue", "tan"}};
  const char* color = colors[(t->aid_ - 1) % colors.size()];

  return "label = \"" + t->dot_label() + "\", color = " + color + ", fontcolor = " + color;
}

#if HAVE_SMPI
int Api::get_smpi_request_tag(smx_simcall_t const& simcall, simgrid::simix::Simcall type) const
{
  void* simcall_data = nullptr;
  if (type == Simcall::COMM_ISEND)
    simcall_data = simcall_comm_isend__get__data(simcall);
  else if (type == Simcall::COMM_IRECV)
    simcall_data = simcall_comm_irecv__get__data(simcall);
  Remote<simgrid::smpi::Request> mpi_request;
  mc_model_checker->get_remote_process().read(mpi_request, remote(static_cast<smpi::Request*>(simcall_data)));
  return mpi_request.get_buffer()->tag();
}
#endif

void Api::restore_state(std::shared_ptr<simgrid::mc::Snapshot> system_state) const
{
  system_state->restore(&mc_model_checker->get_remote_process());
}

void Api::log_state() const
{
  session_singleton->log_state();
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
