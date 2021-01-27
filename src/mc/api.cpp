#include "api.hpp"

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/checker/SimcallInspector.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_pattern.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"
#include <xbt/asserts.h>
#include <xbt/log.h>
// #include <xbt/dynar.h>

#if HAVE_SMPI
#include "src/smpi/include/smpi_request.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Api, mc, "Logging specific to MC Fasade APIs ");

using Simcall = simgrid::simix::Simcall;

namespace simgrid {
namespace mc {

static inline const char* get_color(int id)
{
  static constexpr std::array<const char*, 13> colors{{"blue", "red", "green3", "goldenrod", "brown", "purple",
                                                       "magenta", "turquoise4", "gray25", "forestgreen", "hotpink",
                                                       "lightblue", "tan"}};
  return colors[id % colors.size()];
}

static char* pointer_to_string(void* pointer)
{
  if (XBT_LOG_ISENABLED(Api, xbt_log_priority_verbose))
    return bprintf("%p", pointer);

  return xbt_strdup("(verbose only)");
}

static char* buff_size_to_string(size_t buff_size)
{
  if (XBT_LOG_ISENABLED(Api, xbt_log_priority_verbose))
    return bprintf("%zu", buff_size);

  return xbt_strdup("(verbose only)");
}

inline
smx_mailbox_t get_mbox(smx_simcall_t const r)
{
  switch (r->call_) {
    case Simcall::COMM_ISEND:
      return simcall_comm_isend__get__mbox(r);
    case Simcall::COMM_IRECV:
      return simcall_comm_irecv__get__mbox(r);
    default:
      return nullptr;
  }
}

inline simgrid::kernel::activity::CommImpl* get_comm(smx_simcall_t const r)
{
  switch (r->call_) {
    case Simcall::COMM_WAIT:
      return simcall_comm_wait__getraw__comm(r);
    case Simcall::COMM_TEST:
      return simcall_comm_test__getraw__comm(r);
    default:
      return nullptr;
  }
}

// Does half the job
inline bool request_depend_asymmetric(smx_simcall_t r1, smx_simcall_t r2)
{
  if (r1->call_ == Simcall::COMM_ISEND && r2->call_ == Simcall::COMM_IRECV)
    return false;

  if (r1->call_ == Simcall::COMM_IRECV && r2->call_ == Simcall::COMM_ISEND)
    return false;

  // Those are internal requests, we do not need indirection because those objects are copies:
  const kernel::activity::CommImpl* synchro1 = get_comm(r1);
  const kernel::activity::CommImpl* synchro2 = get_comm(r2);

  if ((r1->call_ == Simcall::COMM_ISEND || r1->call_ == Simcall::COMM_IRECV) && r2->call_ == Simcall::COMM_WAIT) {
    const kernel::activity::MailboxImpl* mbox = get_mbox(r1); // r1->get_mboxx')

    if (mbox != synchro2->mbox_cpy
        && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->issuer_ != synchro2->src_actor_.get()) && (r1->issuer_ != synchro2->dst_actor_.get()) &&
        simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == Simcall::COMM_ISEND) && (synchro2->type_ == kernel::activity::CommImpl::Type::SEND) &&
        (synchro2->src_buff_ != simcall_comm_isend__get__src_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == Simcall::COMM_IRECV) && (synchro2->type_ == kernel::activity::CommImpl::Type::RECEIVE) &&
        (synchro2->dst_buff_ != simcall_comm_irecv__get__dst_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the isend/irecv call is not stored in a buffer used in the
   * test call. */
#if 0
  if((r1->call == Simcall::COMM_ISEND || r1->call == Simcall::COMM_IRECV)
      &&  r2->call == Simcall::COMM_TEST)
    return false;
#endif

  if (r1->call_ == Simcall::COMM_WAIT && (r2->call_ == Simcall::COMM_WAIT || r2->call_ == Simcall::COMM_TEST) &&
      (synchro1->src_actor_.get() == nullptr || synchro1->dst_actor_.get() == nullptr))
    return false;

  if (r1->call_ == Simcall::COMM_TEST &&
      (simcall_comm_test__get__comm(r1) == nullptr || synchro1->src_buff_ == nullptr || synchro1->dst_buff_ == nullptr))
    return false;

  if (r1->call_ == Simcall::COMM_TEST && r2->call_ == Simcall::COMM_WAIT &&
      synchro1->src_buff_ == synchro2->src_buff_ && synchro1->dst_buff_ == synchro2->dst_buff_)
    return false;

  if (r1->call_ == Simcall::COMM_WAIT && r2->call_ == Simcall::COMM_TEST && synchro1->src_buff_ != nullptr &&
      synchro1->dst_buff_ != nullptr && synchro2->src_buff_ != nullptr && synchro2->dst_buff_ != nullptr &&
      synchro1->dst_buff_ != synchro2->src_buff_ && synchro1->dst_buff_ != synchro2->dst_buff_ &&
      synchro2->dst_buff_ != synchro1->src_buff_)
    return false;

  return true;
}

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
  state->executed_req_.call_         = Simcall::NONE;

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

void Api::initialize(char** argv) const
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

std::vector<simgrid::mc::ActorInformation>& Api::get_actors() const
{
  return mc_model_checker->get_remote_simulation().actors();
}

bool Api::actor_is_enabled(aid_t pid) const
{
  return session->actor_is_enabled(pid);
}

unsigned long Api::get_maxpid() const
{
  return MC_smx_get_maxpid();
}

int Api::get_actors_size() const
{
  return mc_model_checker->get_remote_simulation().actors().size();
}

bool Api::comm_addr_equal(const kernel::activity::CommImpl* comm_addr1,
                             const kernel::activity::CommImpl* comm_addr2) const
{
  return remote(comm_addr1) == remote(comm_addr2);
}

RemotePtr<kernel::activity::CommImpl> Api::get_comm_isend_raw_addr(smx_simcall_t request) const
{
  auto comm_addr = simcall_comm_isend__getraw__result(request);
  return RemotePtr<kernel::activity::CommImpl>(static_cast<kernel::activity::CommImpl*>(comm_addr));
}

kernel::activity::CommImpl* Api::get_comm_wait_raw_addr(smx_simcall_t request) const
{
  return simcall_comm_wait__getraw__comm(request);
}

kernel::activity::CommImpl* Api::get_comm_waitany_raw_addr(smx_simcall_t request, int value) const
{
  auto addr =
      mc_model_checker->get_remote_simulation().read(remote(simcall_comm_waitany__getraw__comms(request) + value));
  return static_cast<simgrid::kernel::activity::CommImpl*>(addr);
}

std::string Api::get_pattern_comm_rdv(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();

  char* remote_name = mc_model_checker->get_remote_simulation().read<char*>(RemotePtr<char*>(
      (uint64_t)(synchro->get_mailbox() ? &synchro->get_mailbox()->get_name() : &synchro->mbox_cpy->get_name())));
  auto rdv          = mc_model_checker->get_remote_simulation().read_string(RemotePtr<char>(remote_name));
  return rdv;
}

unsigned long Api::get_pattern_comm_src_proc(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();
  auto src_proc =
      mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(synchro->src_actor_.get()))->get_pid();
  return src_proc;
}

unsigned long Api::get_pattern_comm_dst_proc(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();
  auto src_proc =
      mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(synchro->dst_actor_.get()))->get_pid();
  return src_proc;
}

std::vector<char> Api::get_pattern_comm_data(void* addr) const
{
  Remote<kernel::activity::CommImpl> temp_synchro;
  mc_model_checker->get_remote_simulation().read(temp_synchro, remote((simgrid::kernel::activity::CommImpl*)addr));
  const kernel::activity::CommImpl* synchro = temp_synchro.get_buffer();

  std::vector<char> buffer{};
  if (synchro->src_buff_ != nullptr) {
    buffer.resize(synchro->src_buff_size_);
    mc_model_checker->get_remote_simulation().read_bytes(buffer.data(), buffer.size(), remote(synchro->src_buff_));
  }
  return buffer;
}

std::vector<char> Api::get_pattern_comm_data(const kernel::activity::CommImpl* comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, remote((kernel::activity::CommImpl*)comm_addr));
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  std::vector<char> buffer{};
  if (comm->src_buff_ != nullptr) {
    buffer.resize(comm->src_buff_size_);
    mc_model_checker->get_remote_simulation().read_bytes(buffer.data(), buffer.size(), remote(comm->src_buff_));
  }
  return buffer;
}

const char* Api::get_actor_host_name(smx_actor_t actor) const
{
  const char* host_name = MC_smx_actor_get_host_name(actor);
  return host_name;
}

#if HAVE_SMPI
bool Api::check_send_request_detached(smx_simcall_t const& simcall) const
{
  simgrid::smpi::Request mpi_request;
  mc_model_checker->get_remote_simulation().read(
      &mpi_request, remote(static_cast<smpi::Request*>(simcall_comm_isend__get__data(simcall))));
  return mpi_request.detached();
}
#endif

smx_actor_t Api::get_src_actor(const kernel::activity::CommImpl* comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, remote((kernel::activity::CommImpl*)comm_addr));
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto src_proc = mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->src_actor_.get()));
  return src_proc;
}

smx_actor_t Api::get_dst_actor(const kernel::activity::CommImpl* comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, remote((kernel::activity::CommImpl*)comm_addr));
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto dst_proc = mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->dst_actor_.get()));
  return dst_proc;
}

std::size_t Api::get_remote_heap_bytes() const
{
  RemoteSimulation& process = mc_model_checker->get_remote_simulation();
  auto heap_bytes_used      = mmalloc_get_bytes_used_remote(process.get_heap()->heaplimit, process.get_malloc_info());
  return heap_bytes_used;
}

void Api::session_initialize() const
{
  session->initialize();
}

void Api::mc_inc_visited_states() const
{
  mc_model_checker->visited_states++;
}

void Api::mc_inc_executed_trans() const
{
  mc_model_checker->executed_transitions++;
}

unsigned long Api::mc_get_visited_states() const
{
  return mc_model_checker->visited_states;
}

unsigned long Api::mc_get_executed_trans() const
{
  return mc_model_checker->executed_transitions;
}

bool Api::mc_check_deadlock() const
{
  return mc_model_checker->checkDeadlock();
}

void Api::mc_show_deadlock() const
{
  MC_show_deadlock();
}

smx_actor_t Api::simcall_get_issuer(s_smx_simcall const* req) const
{
  return MC_smx_simcall_get_issuer(req);
}

long Api::simcall_get_actor_id(s_smx_simcall const* req) const
{
  return MC_smx_simcall_get_issuer(req)->get_pid();
}

smx_mailbox_t Api::simcall_get_mbox(smx_simcall_t const req) const
{
  return get_mbox(req);
}

bool Api::mc_is_null() const
{
  auto is_null = (mc_model_checker == nullptr) ? true : false;
  return is_null;
}

Checker* Api::mc_get_checker() const
{
  return mc_model_checker->getChecker();
}

void Api::set_checker(Checker* const checker) const
{
  xbt_assert(mc_model_checker);
  xbt_assert(mc_model_checker->getChecker() == nullptr);
  mc_model_checker->setChecker(checker);
}

void Api::handle_simcall(Transition const& transition) const
{
  mc_model_checker->handle_simcall(transition);
}

void Api::mc_wait_for_requests() const
{
  mc_model_checker->wait_for_requests();
}

void Api::mc_exit(int status) const
{
  mc_model_checker->exit(status);
}

std::string const& Api::mc_get_host_name(std::string const& hostname) const
{
  return mc_model_checker->get_host_name(hostname);
}

void Api::dump_record_path() const
{
  simgrid::mc::dumpRecordPath();
}

smx_simcall_t Api::mc_state_choose_request(simgrid::mc::State* state) const
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

bool Api::simcall_check_dependency(smx_simcall_t const req1, smx_simcall_t const req2) const
{
  if (req1->issuer_ == req2->issuer_)
    return false;

  /* Wait with timeout transitions are not considered by the independence theorem, thus we consider them as dependent with all other transitions */
  if ((req1->call_ == Simcall::COMM_WAIT && simcall_comm_wait__get__timeout(req1) > 0) ||
      (req2->call_ == Simcall::COMM_WAIT && simcall_comm_wait__get__timeout(req2) > 0))
    return true;

  if (req1->call_ != req2->call_)
    return request_depend_asymmetric(req1, req2) && request_depend_asymmetric(req2, req1);

  // Those are internal requests, we do not need indirection because those objects are copies:
  const kernel::activity::CommImpl* synchro1 = get_comm(req1);
  const kernel::activity::CommImpl* synchro2 = get_comm(req2);

  switch (req1->call_) {
    case Simcall::COMM_ISEND:
      return simcall_comm_isend__get__mbox(req1) == simcall_comm_isend__get__mbox(req2);
    case Simcall::COMM_IRECV:
      return simcall_comm_irecv__get__mbox(req1) == simcall_comm_irecv__get__mbox(req2);
    case Simcall::COMM_WAIT:
      if (synchro1->src_buff_ == synchro2->src_buff_ && synchro1->dst_buff_ == synchro2->dst_buff_)
        return false;
      if (synchro1->src_buff_ != nullptr && synchro1->dst_buff_ != nullptr && synchro2->src_buff_ != nullptr &&
          synchro2->dst_buff_ != nullptr && synchro1->dst_buff_ != synchro2->src_buff_ &&
          synchro1->dst_buff_ != synchro2->dst_buff_ && synchro2->dst_buff_ != synchro1->src_buff_)
        return false;
      return true;
    default:
      return true;
  }
}

std::string Api::request_to_string(smx_simcall_t req, int value, RequestType request_type) const
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  if (req->inspector_ != nullptr)
    return req->inspector_->to_string();

  bool use_remote_comm = true;
  switch (request_type) {
    case simgrid::mc::RequestType::simix:
      use_remote_comm = true;
      break;
    case simgrid::mc::RequestType::executed:
    case simgrid::mc::RequestType::internal:
      use_remote_comm = false;
      break;
    default:
      THROW_IMPOSSIBLE;
  }

  const char* type = nullptr;
  char* args       = nullptr;

  smx_actor_t issuer = MC_smx_simcall_get_issuer(req);

  switch (req->call_) {
    case Simcall::COMM_ISEND: {
      type     = "iSend";
      char* p  = pointer_to_string(simcall_comm_isend__get__src_buff(req));
      char* bs = buff_size_to_string(simcall_comm_isend__get__src_buff_size(req));
      if (issuer->get_host())
        args = bprintf("src=(%ld)%s (%s), buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                       MC_smx_actor_get_name(issuer), p, bs);
      else
        args = bprintf("src=(%ld)%s, buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_name(issuer), p, bs);
      xbt_free(bs);
      xbt_free(p);
      break;
    }

    case Simcall::COMM_IRECV: {
      size_t* remote_size = simcall_comm_irecv__get__dst_buff_size(req);
      size_t size         = 0;
      if (remote_size)
        mc_model_checker->get_remote_simulation().read_bytes(&size, sizeof(size), remote(remote_size));

      type     = "iRecv";
      char* p  = pointer_to_string(simcall_comm_irecv__get__dst_buff(req));
      char* bs = buff_size_to_string(size);
      if (issuer->get_host())
        args = bprintf("dst=(%ld)%s (%s), buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                       MC_smx_actor_get_name(issuer), p, bs);
      else
        args = bprintf("dst=(%ld)%s, buff=%s, size=%s", issuer->get_pid(), MC_smx_actor_get_name(issuer), p, bs);
      xbt_free(bs);
      xbt_free(p);
      break;
    }

    case Simcall::COMM_WAIT: {
      simgrid::kernel::activity::CommImpl* remote_act = simcall_comm_wait__getraw__comm(req);
      char* p;
      if (value == -1) {
        type = "WaitTimeout";
        p    = pointer_to_string(remote_act);
        args = bprintf("comm=%s", p);
      } else {
        type = "Wait";
        p    = pointer_to_string(remote_act);

        simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_synchro;
        const simgrid::kernel::activity::CommImpl* act;
        if (use_remote_comm) {
          mc_model_checker->get_remote_simulation().read(temp_synchro, remote(remote_act));
          act = temp_synchro.get_buffer();
        } else
          act = remote_act;

        smx_actor_t src_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->src_actor_.get()));
        smx_actor_t dst_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->dst_actor_.get()));
        args = bprintf("comm=%s [(%ld)%s (%s)-> (%ld)%s (%s)]", p, src_proc ? src_proc->get_pid() : 0,
                       src_proc ? MC_smx_actor_get_host_name(src_proc) : "",
                       src_proc ? MC_smx_actor_get_name(src_proc) : "", dst_proc ? dst_proc->get_pid() : 0,
                       dst_proc ? MC_smx_actor_get_host_name(dst_proc) : "",
                       dst_proc ? MC_smx_actor_get_name(dst_proc) : "");
      }
      xbt_free(p);
      break;
    }

    case Simcall::COMM_TEST: {
      simgrid::kernel::activity::CommImpl* remote_act = simcall_comm_test__getraw__comm(req);
      simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_synchro;
      const simgrid::kernel::activity::CommImpl* act;
      if (use_remote_comm) {
        mc_model_checker->get_remote_simulation().read(temp_synchro, remote(remote_act));
        act = temp_synchro.get_buffer();
      } else
        act = remote_act;

      char* p;
      if (act->src_actor_.get() == nullptr || act->dst_actor_.get() == nullptr) {
        type = "Test FALSE";
        p    = pointer_to_string(remote_act);
        args = bprintf("comm=%s", p);
      } else {
        type = "Test TRUE";
        p    = pointer_to_string(remote_act);

        smx_actor_t src_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->src_actor_.get()));
        smx_actor_t dst_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->dst_actor_.get()));
        args = bprintf("comm=%s [(%ld)%s (%s) -> (%ld)%s (%s)]", p, src_proc->get_pid(),
                       MC_smx_actor_get_name(src_proc), MC_smx_actor_get_host_name(src_proc), dst_proc->get_pid(),
                       MC_smx_actor_get_name(dst_proc), MC_smx_actor_get_host_name(dst_proc));
      }
      xbt_free(p);
      break;
    }

    case Simcall::COMM_WAITANY: {
      type         = "WaitAny";
      size_t count = simcall_comm_waitany__get__count(req);
      if (count > 0) {
        simgrid::kernel::activity::CommImpl* remote_sync;
        remote_sync =
            mc_model_checker->get_remote_simulation().read(remote(simcall_comm_waitany__get__comms(req) + value));
        char* p = pointer_to_string(remote_sync);
        args    = bprintf("comm=%s (%d of %zu)", p, value + 1, count);
        xbt_free(p);
      } else
        args = bprintf("comm at idx %d", value);
      break;
    }

    case Simcall::COMM_TESTANY:
      if (value == -1) {
        type = "TestAny FALSE";
        args = xbt_strdup("-");
      } else {
        type = "TestAny";
        args = bprintf("(%d of %zu)", value + 1, simcall_comm_testany__get__count(req));
      }
      break;

    case Simcall::MUTEX_TRYLOCK:
    case Simcall::MUTEX_LOCK: {
      if (req->call_ == Simcall::MUTEX_LOCK)
        type = "Mutex LOCK";
      else
        type = "Mutex TRYLOCK";

      simgrid::mc::Remote<simgrid::kernel::activity::MutexImpl> mutex;
      mc_model_checker->get_remote_simulation().read_bytes(mutex.get_buffer(), sizeof(mutex),
                                                           remote(req->call_ == Simcall::MUTEX_LOCK
                                                                      ? simcall_mutex_lock__get__mutex(req)
                                                                      : simcall_mutex_trylock__get__mutex(req)));
      args = bprintf("locked = %d, owner = %d, sleeping = n/a", mutex.get_buffer()->is_locked(),
                     mutex.get_buffer()->get_owner() != nullptr
                         ? (int)mc_model_checker->get_remote_simulation()
                               .resolve_actor(simgrid::mc::remote(mutex.get_buffer()->get_owner()))
                               ->get_pid()
                         : -1);
      break;
    }

    case Simcall::MC_RANDOM:
      type = "MC_RANDOM";
      args = bprintf("%d", value);
      break;

    default:
      type = SIMIX_simcall_name(req->call_);
      args = bprintf("??");
      break;
  }

  std::string str;
  if (args != nullptr)
    str = simgrid::xbt::string_printf("[(%ld)%s (%s)] %s(%s)", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                      MC_smx_actor_get_name(issuer), type, args);
  else
    str = simgrid::xbt::string_printf("[(%ld)%s (%s)] %s ", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                      MC_smx_actor_get_name(issuer), type);
  xbt_free(args);
  return str;
}

std::string Api::request_get_dot_output(smx_simcall_t req, int value) const
{
  const smx_actor_t issuer = MC_smx_simcall_get_issuer(req);
  const char* color        = get_color(issuer->get_pid() - 1);

  if (req->inspector_ != nullptr)
    return simgrid::xbt::string_printf("label = \"%s\", color = %s, fontcolor = %s",
                                       req->inspector_->dot_label().c_str(), color, color);

  std::string label;

  switch (req->call_) {
    case Simcall::COMM_ISEND:
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] iSend", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
      else
        label = bprintf("[(%ld)] iSend", issuer->get_pid());
      break;

    case Simcall::COMM_IRECV:
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] iRecv", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
      else
        label = xbt::string_printf("[(%ld)] iRecv", issuer->get_pid());
      break;

    case Simcall::COMM_WAIT:
      if (value == -1) {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] WaitTimeout", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = xbt::string_printf("[(%ld)] WaitTimeout", issuer->get_pid());
      } else {
        kernel::activity::ActivityImpl* remote_act = simcall_comm_wait__getraw__comm(req);
        Remote<kernel::activity::CommImpl> temp_comm;
        mc_model_checker->get_remote_simulation().read(temp_comm,
                                                       remote(static_cast<kernel::activity::CommImpl*>(remote_act)));
        const kernel::activity::CommImpl* comm = temp_comm.get_buffer();

        const kernel::actor::ActorImpl* src_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(comm->src_actor_.get()));
        const kernel::actor::ActorImpl* dst_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(comm->dst_actor_.get()));
        if (issuer->get_host())
          label =
              xbt::string_printf("[(%ld)%s] Wait [(%ld)->(%ld)]", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                 src_proc ? src_proc->get_pid() : 0, dst_proc ? dst_proc->get_pid() : 0);
        else
          label = xbt::string_printf("[(%ld)] Wait [(%ld)->(%ld)]", issuer->get_pid(),
                                     src_proc ? src_proc->get_pid() : 0, dst_proc ? dst_proc->get_pid() : 0);
      }
      break;

    case Simcall::COMM_TEST: {
      kernel::activity::ActivityImpl* remote_act = simcall_comm_test__getraw__comm(req);
      Remote<simgrid::kernel::activity::CommImpl> temp_comm;
      mc_model_checker->get_remote_simulation().read(temp_comm,
                                                     remote(static_cast<kernel::activity::CommImpl*>(remote_act)));
      const kernel::activity::CommImpl* comm = temp_comm.get_buffer();
      if (comm->src_actor_.get() == nullptr || comm->dst_actor_.get() == nullptr) {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] Test FALSE", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = bprintf("[(%ld)] Test FALSE", issuer->get_pid());
      } else {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] Test TRUE", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = xbt::string_printf("[(%ld)] Test TRUE", issuer->get_pid());
      }
      break;
    }

    case Simcall::COMM_WAITANY: {
      size_t comms_size = simcall_comm_waitany__get__count(req);
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] WaitAny [%d of %zu]", issuer->get_pid(),
                                   MC_smx_actor_get_host_name(issuer), value + 1, comms_size);
      else
        label = xbt::string_printf("[(%ld)] WaitAny [%d of %zu]", issuer->get_pid(), value + 1, comms_size);
      break;
    }

    case Simcall::COMM_TESTANY:
      if (value == -1) {
        if (issuer->get_host())
          label = xbt::string_printf("[(%ld)%s] TestAny FALSE", issuer->get_pid(), MC_smx_actor_get_host_name(issuer));
        else
          label = xbt::string_printf("[(%ld)] TestAny FALSE", issuer->get_pid());
      } else {
        if (issuer->get_host())
          label =
              xbt::string_printf("[(%ld)%s] TestAny TRUE [%d of %lu]", issuer->get_pid(),
                                 MC_smx_actor_get_host_name(issuer), value + 1, simcall_comm_testany__get__count(req));
        else
          label = xbt::string_printf("[(%ld)] TestAny TRUE [%d of %lu]", issuer->get_pid(), value + 1,
                                     simcall_comm_testany__get__count(req));
      }
      break;

    case Simcall::MUTEX_TRYLOCK:
      label = xbt::string_printf("[(%ld)] Mutex TRYLOCK", issuer->get_pid());
      break;

    case Simcall::MUTEX_LOCK:
      label = xbt::string_printf("[(%ld)] Mutex LOCK", issuer->get_pid());
      break;

    case Simcall::MC_RANDOM:
      if (issuer->get_host())
        label = xbt::string_printf("[(%ld)%s] MC_RANDOM (%d)", issuer->get_pid(), MC_smx_actor_get_host_name(issuer),
                                   value);
      else
        label = xbt::string_printf("[(%ld)] MC_RANDOM (%d)", issuer->get_pid(), value);
      break;

    default:
      THROW_UNIMPLEMENTED;
  }

  return xbt::string_printf("label = \"%s\", color = %s, fontcolor = %s", label.c_str(), color, color);
}

const char* Api::simcall_get_name(simgrid::simix::Simcall kind) const
{
  return SIMIX_simcall_name(kind);
}

#if HAVE_SMPI
int Api::get_smpi_request_tag(smx_simcall_t const& simcall, simgrid::simix::Simcall type) const
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

void Api::restore_state(std::shared_ptr<simgrid::mc::Snapshot> system_state) const
{
  system_state->restore(&mc_model_checker->get_remote_simulation());
}

void Api::log_state() const
{
  session->log_state();
}

bool Api::snapshot_equal(const Snapshot* s1, const Snapshot* s2) const
{
  return simgrid::mc::snapshot_equal(s1, s2);
}

simgrid::mc::Snapshot* Api::take_snapshot(int num_state) const
{
  auto snapshot = new simgrid::mc::Snapshot(num_state);
  return snapshot;
}

void Api::s_close() const
{
  session->close();
}

void Api::restore_initial_state() const
{
  session->restore_initial_state();
}

void Api::execute(Transition const& transition) const
{
  session->execute(transition);
}

#if SIMGRID_HAVE_MC
void Api::automaton_load(const char* file) const
{
  MC_automaton_load(file);
}
#endif

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
