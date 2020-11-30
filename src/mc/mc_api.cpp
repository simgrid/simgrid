#include "mc_api.hpp"

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"
#include "src/mc/mc_pattern.hpp"

#include <xbt/asserts.h>
#include <xbt/log.h>

#if HAVE_SMPI
#include "src/smpi/include/smpi_request.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_api, mc, "Logging specific to MC Fasade APIs ");

using Simcall = simgrid::simix::Simcall;

namespace simgrid {
namespace mc {

/* Search an enabled transition for the given process.
 *
 * This can be seen as an iterator returning the next transition of the process.
 *
 * We only consider the processes that are both
 *  - marked "to be interleaved" in their ActorState (controlled by the checker algorithm).
 *  - which simcall can currently be executed (like a comm where the other partner is already known)
 * Once we returned the last enabled transition of a process, it is marked done.
 *
 * Things can get muddled with the WAITANY and TESTANY simcalls, that are rewritten on the fly to a bunch of WAIT
 * (resp TEST) transitions using the transition.argument field to remember what was the last returned sub-transition.
 */
static inline smx_simcall_t MC_state_choose_request_for_process(simgrid::mc::State* state, smx_actor_t actor)
{
  /* reset the outgoing transition */
  simgrid::mc::ActorState* procstate = &state->actor_states_[actor->get_pid()];
  state->transition_.pid_            = -1;
  state->transition_.argument_       = -1;
  state->executed_req_.call_         =  Simcall::NONE;

  if (not simgrid::mc::actor_is_enabled(actor))
    return nullptr; // Not executable in the application

  smx_simcall_t req = nullptr;
  switch (actor->simcall_.call_) {
    case Simcall::COMM_WAITANY:
      state->transition_.argument_ = -1;
      while (procstate->times_considered < simcall_comm_waitany__get__count(&actor->simcall_)) {
        if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall_, procstate->times_considered)) {
          state->transition_.argument_ = procstate->times_considered;
          ++procstate->times_considered;
          break;
        }
        ++procstate->times_considered;
      }

      if (procstate->times_considered >= simcall_comm_waitany__get__count(&actor->simcall_))
        procstate->set_done();
      if (state->transition_.argument_ != -1)
        req = &actor->simcall_;
      break;

    case Simcall::COMM_TESTANY: {
      unsigned start_count         = procstate->times_considered;
      state->transition_.argument_ = -1;
      while (procstate->times_considered < simcall_comm_testany__get__count(&actor->simcall_)) {
        if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall_, procstate->times_considered)) {
          state->transition_.argument_ = procstate->times_considered;
          ++procstate->times_considered;
          break;
        }
        ++procstate->times_considered;
      }

      if (procstate->times_considered >= simcall_comm_testany__get__count(&actor->simcall_))
        procstate->set_done();

      if (state->transition_.argument_ != -1 || start_count == 0)
        req = &actor->simcall_;

      break;
    }

    case Simcall::COMM_WAIT: {
      simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> remote_act =
          remote(simcall_comm_wait__getraw__comm(&actor->simcall_));
      simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_act;
      mc_model_checker->get_remote_simulation().read(temp_act, remote_act);
      const simgrid::kernel::activity::CommImpl* act = temp_act.get_buffer();
      if (act->src_actor_.get() && act->dst_actor_.get())
        state->transition_.argument_ = 0; // OK
      else if (act->src_actor_.get() == nullptr && act->type_ == simgrid::kernel::activity::CommImpl::Type::READY &&
               act->detached())
        state->transition_.argument_ = 0; // OK
      else
        state->transition_.argument_ = -1; // timeout
      procstate->set_done();
      req = &actor->simcall_;
      break;
    }

    case Simcall::MC_RANDOM: {
      int min_value                = simcall_mc_random__get__min(&actor->simcall_);
      state->transition_.argument_ = procstate->times_considered + min_value;
      procstate->times_considered++;
      if (state->transition_.argument_ == simcall_mc_random__get__max(&actor->simcall_))
        procstate->set_done();
      req = &actor->simcall_;
      break;
    }

    default:
      procstate->set_done();
      state->transition_.argument_ = 0;
      req                          = &actor->simcall_;
      break;
  }
  if (not req)
    return nullptr;

  state->transition_.pid_ = actor->get_pid();
  state->executed_req_    = *req;
  // Fetch the data of the request and translate it:
  state->internal_req_ = *req;

  /* The waitany and testany request are transformed into a wait or test request over the corresponding communication
   * action so it can be treated later by the dependence function. */
  switch (req->call_) {
    case Simcall::COMM_WAITANY: {
      state->internal_req_.call_ = Simcall::COMM_WAIT;
      simgrid::kernel::activity::CommImpl* remote_comm;
      remote_comm = mc_model_checker->get_remote_simulation().read(
          remote(simcall_comm_waitany__get__comms(req) + state->transition_.argument_));
      mc_model_checker->get_remote_simulation().read(state->internal_comm_, remote(remote_comm));
      simcall_comm_wait__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      simcall_comm_wait__set__timeout(&state->internal_req_, 0);
      break;
    }

    case Simcall::COMM_TESTANY:
      state->internal_req_.call_ = Simcall::COMM_TEST;

      if (state->transition_.argument_ > 0) {
        simgrid::kernel::activity::CommImpl* remote_comm = mc_model_checker->get_remote_simulation().read(
            remote(simcall_comm_testany__get__comms(req) + state->transition_.argument_));
        mc_model_checker->get_remote_simulation().read(state->internal_comm_, remote(remote_comm));
      }

      simcall_comm_test__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      simcall_comm_test__set__result(&state->internal_req_, state->transition_.argument_);
      break;

    case Simcall::COMM_WAIT:
      mc_model_checker->get_remote_simulation().read_bytes(&state->internal_comm_, sizeof(state->internal_comm_),
                                                           remote(simcall_comm_wait__getraw__comm(req)));
      simcall_comm_wait__set__comm(&state->executed_req_, state->internal_comm_.get_buffer());
      simcall_comm_wait__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      break;

    case Simcall::COMM_TEST:
      mc_model_checker->get_remote_simulation().read_bytes(&state->internal_comm_, sizeof(state->internal_comm_),
                                                           remote(simcall_comm_test__getraw__comm(req)));
      simcall_comm_test__set__comm(&state->executed_req_, state->internal_comm_.get_buffer());
      simcall_comm_test__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      break;

    default:
      /* No translation needed */
      break;
  }

  return req;
}

void mc_api::initialize(char** argv)
{
  simgrid::mc::session = new simgrid::mc::Session([argv] {
    int i = 1;
    while (argv[i] != nullptr && argv[i][0] == '-')
      i++;
    xbt_assert(argv[i] != nullptr,
               "Unable to find a binary to exec on the command line. Did you only pass config flags?");
    execvp(argv[i], argv + i);
    xbt_die("The model-checked process failed to exec(): %s", strerror(errno));
  });
}

std::vector<simgrid::mc::ActorInformation>& mc_api::get_actors() const
{
  return mc_model_checker->get_remote_simulation().actors();
}

bool mc_api::actor_is_enabled(aid_t pid) const
{
  return session->actor_is_enabled(pid);
}

unsigned long mc_api::get_maxpid() const
{
  return MC_smx_get_maxpid();
}

int mc_api::get_actors_size() const
{
  return mc_model_checker->get_remote_simulation().actors().size();
}

void mc_api::copy_incomplete_comm_pattern(simgrid::mc::State* state) const
{
  state->incomplete_comm_pattern_.clear();
  for (unsigned i=0; i < MC_smx_get_maxpid(); i++) {
    std::vector<simgrid::mc::PatternCommunication> res;
    for (auto const& comm : incomplete_communications_pattern[i])
      res.push_back(comm->dup());
    state->incomplete_comm_pattern_.push_back(std::move(res));
  }
}

void mc_api::copy_index_comm_pattern(simgrid::mc::State* state) const
{
  state->communication_indices_.clear();
  for (auto const& list_process_comm : initial_communications_pattern)
    state->communication_indices_.push_back(list_process_comm.index_comm);
}

kernel::activity::CommImpl* mc_api::get_pattern_comm_addr(smx_simcall_t request) const
{
  auto comm_addr = simcall_comm_isend__getraw__result(request);
  return static_cast<kernel::activity::CommImpl*>(comm_addr);
}

std::string mc_api::get_pattern_comm_rdv(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();

  char* remote_name = mc_model_checker->get_remote_simulation().read<char*>(RemotePtr<char*>(
      (uint64_t)(synchro->get_mailbox() ? &synchro->get_mailbox()->get_name() : &synchro->mbox_cpy->get_name())));
  auto rdv = mc_model_checker->get_remote_simulation().read_string(RemotePtr<char>(remote_name));
  return rdv;
}

unsigned long mc_api::get_pattern_comm_src_proc(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();
  auto src_proc = mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(synchro->src_actor_.get()))->get_pid();
  return src_proc;
}

unsigned long mc_api::get_pattern_comm_dst_proc(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();
  auto src_proc = mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(synchro->dst_actor_.get()))->get_pid();
  return src_proc;
}

std::vector<char> mc_api::get_pattern_comm_data(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();

  std::vector<char> buffer {};
  if (synchro->src_buff_ != nullptr) {
    buffer.resize(synchro->src_buff_size_);
    mc_model_checker->get_remote_simulation().read_bytes(buffer.data(), buffer.size(),
                                                         remote(synchro->src_buff_));
  }
  return buffer;
}

std::vector<char> mc_api::get_pattern_comm_data(mc::RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();
  
  std::vector<char> buffer {};
  if (comm->src_buff_ != nullptr) {
    buffer.resize(comm->src_buff_size_);
    mc_model_checker->get_remote_simulation().read_bytes(buffer.data(), buffer.size(),
                                                         remote(comm->src_buff_));
  }
  return buffer;
}

const char* mc_api::get_actor_host_name(smx_actor_t actor) const
{
  const char* host_name = MC_smx_actor_get_host_name(actor);
  return host_name;
}

bool mc_api::check_send_request_detached(smx_simcall_t const& simcall) const
{
  simgrid::smpi::Request mpi_request;
  mc_model_checker->get_remote_simulation().read(
      &mpi_request, remote(static_cast<smpi::Request*>(simcall_comm_isend__get__data(simcall))));
  return mpi_request.detached();
}

smx_actor_t mc_api::get_src_actor(mc::RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto src_proc = mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->src_actor_.get()));
  return src_proc;
}

smx_actor_t mc_api::get_dst_actor(mc::RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto dst_proc = mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->dst_actor_.get()));
  return dst_proc;
}

std::size_t mc_api::get_remote_heap_bytes() const
{
  RemoteSimulation& process = mc_model_checker->get_remote_simulation();
  auto heap_bytes_used      = mmalloc_get_bytes_used_remote(process.get_heap()->heaplimit, process.get_malloc_info());
  return heap_bytes_used;
}

void mc_api::s_initialize() const
{
  session->initialize();
}

ModelChecker* mc_api::get_model_checker() const
{
  return mc_model_checker;
}

void mc_api::mc_inc_visited_states() const
{
  mc_model_checker->visited_states++;
}

void mc_api::mc_inc_executed_trans() const
{
  mc_model_checker->executed_transitions++;
}

unsigned long mc_api::mc_get_visited_states() const
{
  return mc_model_checker->visited_states;
}

unsigned long mc_api::mc_get_executed_trans() const
{
  return mc_model_checker->executed_transitions;
}

bool mc_api::mc_check_deadlock() const
{
  return mc_model_checker->checkDeadlock();
}

void mc_api::mc_show_deadlock() const
{
  MC_show_deadlock();
}

smx_actor_t mc_api::simcall_get_issuer(s_smx_simcall const* req) const
{
  return MC_smx_simcall_get_issuer(req);
}

bool mc_api::mc_is_null() const
{
  auto is_null = (mc_model_checker == nullptr) ? true : false;
  return is_null;
}

Checker* mc_api::mc_get_checker() const
{
  return mc_model_checker->getChecker();
}

RemoteSimulation& mc_api::mc_get_remote_simulation() const
{
  return mc_model_checker->get_remote_simulation();
}

void mc_api::handle_simcall(Transition const& transition) const
{
  mc_model_checker->handle_simcall(transition);
}

void mc_api::mc_wait_for_requests() const
{
  mc_model_checker->wait_for_requests();
}

void mc_api::mc_exit(int status) const
{
  mc_model_checker->exit(status);
}

std::string const& mc_api::mc_get_host_name(std::string const& hostname) const
{
  return mc_model_checker->get_host_name(hostname);
}

void mc_api::mc_dump_record_path() const
{
  simgrid::mc::dumpRecordPath();
}

smx_simcall_t mc_api::mc_state_choose_request(simgrid::mc::State* state) const
{
  for (auto& actor : mc_model_checker->get_remote_simulation().actors()) {
    /* Only consider the actors that were marked as interleaving by the checker algorithm */
    if (not state->actor_states_[actor.copy.get_buffer()->get_pid()].is_todo())
      continue;

    smx_simcall_t res = MC_state_choose_request_for_process(state, actor.copy.get_buffer());
    if (res)
      return res;
  }
  return nullptr;
}

bool mc_api::request_depend(smx_simcall_t req1, smx_simcall_t req2) const
{
  return simgrid::mc::request_depend(req1, req2);
}

std::string mc_api::request_to_string(smx_simcall_t req, int value, RequestType request_type) const
{
  return simgrid::mc::request_to_string(req, value, request_type).c_str();
}

std::string mc_api::request_get_dot_output(smx_simcall_t req, int value) const
{
  return simgrid::mc::request_get_dot_output(req, value);
}

const char* mc_api::simcall_get_name(simgrid::simix::Simcall kind) const
{
  return SIMIX_simcall_name(kind);
}

#if HAVE_SMPI
int mc_api::get_smpi_request_tag(smx_simcall_t const& simcall, simgrid::simix::Simcall type) const
{
  simgrid::smpi::Request mpi_request;
  void* simcall_data = nullptr;
  if (type == Simcall::COMM_ISEND)
    simcall_data = simcall_comm_isend__get__data(simcall);
  else if (type == Simcall::COMM_IRECV)
    simcall_data = simcall_comm_irecv__get__data(simcall);
  mc_model_checker->get_remote_simulation().read(&mpi_request, remote(static_cast<smpi::Request*>(simcall_data)));
  return mpi_request.tag();
}
#endif

void mc_api::restore_state(std::shared_ptr<simgrid::mc::Snapshot> system_state) const
{
  system_state->restore(&mc_model_checker->get_remote_simulation());
}

bool mc_api::snapshot_equal(const Snapshot* s1, const Snapshot* s2) const
{
  return simgrid::mc::snapshot_equal(s1, s2);
}

simgrid::mc::Snapshot* mc_api::take_snapshot(int num_state) const
{
  auto snapshot = new simgrid::mc::Snapshot(num_state);
  return snapshot;
}

void mc_api::s_close() const
{
  session->close();
}

void mc_api::restore_initial_state() const
{
  session->restore_initial_state();
}

void mc_api::execute(Transition const& transition)
{
  session->execute(transition);
}

void mc_api::log_state() const
{
  session->log_state();
}

} // namespace mc
} // namespace simgrid
