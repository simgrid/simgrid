#include "api.hpp"

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/checker/SimcallObserver.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_pattern.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"

#include <xbt/asserts.h>
#include <xbt/log.h>
#include "simgrid/s4u/Host.hpp"
#include "xbt/string.hpp"
#if HAVE_SMPI
#include "src/smpi/include/smpi_request.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Api, mc, "Logging specific to MC Facade APIs ");

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

static std::string pointer_to_string(void* pointer)
{
  return XBT_LOG_ISENABLED(Api, xbt_log_priority_verbose) ? xbt::string_printf("%p", pointer) : "(verbose only)";
}

static std::string buff_size_to_string(size_t buff_size)
{
  return XBT_LOG_ISENABLED(Api, xbt_log_priority_verbose) ? std::to_string(buff_size) : "(verbose only)";
}

static void simcall_translate(smx_simcall_t req,
                              simgrid::mc::Remote<simgrid::kernel::activity::CommImpl>& buffered_comm);

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
  state->transition_.times_considered_ = -1;
  state->transition_.textual[0]        = '\0';
  state->executed_req_.call_         = Simcall::NONE;

  if (not simgrid::mc::actor_is_enabled(actor))
    return nullptr; // Not executable in the application

  smx_simcall_t req = nullptr;
  if (actor->simcall_.observer_ != nullptr) {
    state->transition_.times_considered_ = procstate->times_considered;
    procstate->times_considered++;
    if (actor->simcall_.mc_max_consider_ <= procstate->times_considered)
      procstate->set_done();
    req = &actor->simcall_;
  } else
    switch (actor->simcall_.call_) {
      case Simcall::COMM_WAITANY:
        state->transition_.times_considered_ = -1;
        while (procstate->times_considered < simcall_comm_waitany__get__count(&actor->simcall_)) {
          if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall_, procstate->times_considered)) {
            state->transition_.times_considered_ = procstate->times_considered;
            ++procstate->times_considered;
            break;
          }
          ++procstate->times_considered;
        }

        if (procstate->times_considered >= simcall_comm_waitany__get__count(&actor->simcall_))
          procstate->set_done();
        if (state->transition_.times_considered_ != -1)
          req = &actor->simcall_;
        break;

      case Simcall::COMM_TESTANY:
        state->transition_.times_considered_ = -1;
        while (procstate->times_considered < simcall_comm_testany__get__count(&actor->simcall_)) {
          if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall_, procstate->times_considered)) {
            state->transition_.times_considered_ = procstate->times_considered;
            ++procstate->times_considered;
            break;
          }
          ++procstate->times_considered;
        }

        if (procstate->times_considered >= simcall_comm_testany__get__count(&actor->simcall_))
          procstate->set_done();
        if (state->transition_.times_considered_ != -1)
          req = &actor->simcall_;
        break;

      case Simcall::COMM_WAIT: {
        simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> remote_act =
            remote(simcall_comm_wait__getraw__comm(&actor->simcall_));
        simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_act;
        mc_model_checker->get_remote_simulation().read(temp_act, remote_act);
        const simgrid::kernel::activity::CommImpl* act = temp_act.get_buffer();
        if (act->src_actor_.get() && act->dst_actor_.get())
          state->transition_.times_considered_ = 0; // OK
        else if (act->src_actor_.get() == nullptr && act->state_ == simgrid::kernel::activity::State::READY &&
                 act->detached())
          state->transition_.times_considered_ = 0; // OK
        else
          state->transition_.times_considered_ = -1; // timeout
        procstate->set_done();
        req = &actor->simcall_;
        break;
      }

      default:
        procstate->set_done();
        state->transition_.times_considered_ = 0;
        req                                  = &actor->simcall_;
        break;
    }
  if (not req)
    return nullptr;

  state->transition_.pid_ = actor->get_pid();
  state->executed_req_    = *req;

  // Fetch the data of the request and translate it:
  state->internal_req_ = *req;
  state->internal_req_.mc_value_ = state->transition_.times_considered_;
  simcall_translate(&state->internal_req_, state->internal_comm_);

  return req;
}

static void simcall_translate(smx_simcall_t req,
                              simgrid::mc::Remote<simgrid::kernel::activity::CommImpl>& buffered_comm)
{
  simgrid::kernel::activity::CommImpl* chosen_comm;

  /* The waitany and testany request are transformed into a wait or test request over the corresponding communication
   * action so it can be treated later by the dependence function. */
  switch (req->call_) {
    case Simcall::COMM_WAITANY:
      req->call_  = Simcall::COMM_WAIT;
      chosen_comm = mc_model_checker->get_remote_simulation().read(
          remote(simcall_comm_waitany__get__comms(req) + req->mc_value_));

      mc_model_checker->get_remote_simulation().read(buffered_comm, remote(chosen_comm));
      simcall_comm_wait__set__comm(req, buffered_comm.get_buffer());
      simcall_comm_wait__set__timeout(req, 0);
      break;

    case Simcall::COMM_TESTANY:
      req->call_  = Simcall::COMM_TEST;
      chosen_comm = mc_model_checker->get_remote_simulation().read(
          remote(simcall_comm_testany__get__comms(req) + req->mc_value_));

      mc_model_checker->get_remote_simulation().read(buffered_comm, remote(chosen_comm));
      simcall_comm_test__set__comm(req, buffered_comm.get_buffer());
      simcall_comm_test__set__result(req, req->mc_value_);
      break;

    case Simcall::COMM_WAIT:
      chosen_comm = simcall_comm_wait__getraw__comm(req);
      mc_model_checker->get_remote_simulation().read(buffered_comm, remote(chosen_comm));
      simcall_comm_wait__set__comm(req, buffered_comm.get_buffer());
      break;

    case Simcall::COMM_TEST:
      chosen_comm = simcall_comm_test__getraw__comm(req);
      mc_model_checker->get_remote_simulation().read(buffered_comm, remote(chosen_comm));
      simcall_comm_test__set__comm(req, buffered_comm.get_buffer());
      break;

    default:
      /* No translation needed */
      break;
  }
}

simgrid::kernel::activity::CommImpl* Api::get_comm_or_nullptr(smx_simcall_t const r) const
{
  if (r->call_ == Simcall::COMM_WAIT)
    return simcall_comm_wait__getraw__comm(r);
  if (r->call_ == Simcall::COMM_TEST)
    return simcall_comm_test__getraw__comm(r);
  return nullptr;
}

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

// Does half the job
bool Api::request_depend_asymmetric(smx_simcall_t r1, smx_simcall_t r2) const
{
  if (r1->call_ == Simcall::COMM_ISEND && r2->call_ == Simcall::COMM_IRECV)
    return false;

  if (r1->call_ == Simcall::COMM_IRECV && r2->call_ == Simcall::COMM_ISEND)
    return false;

  // Those are internal requests, we do not need indirection because those objects are copies:
  auto comm1 = get_comm_or_nullptr(r1);
  auto comm2 = get_comm_or_nullptr(r2);

  if ((r1->call_ == Simcall::COMM_ISEND || r1->call_ == Simcall::COMM_IRECV) && r2->call_ == Simcall::COMM_WAIT) {
    auto mbox1 = get_mbox_remote_addr(r1);
    auto mbox2 = remote(comm2->mbox_cpy);

    if (mbox1 != mbox2 && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->issuer_ != comm2->src_actor_.get()) && (r1->issuer_ != comm2->dst_actor_.get()) &&
        simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == Simcall::COMM_ISEND) && (comm2->type_ == kernel::activity::CommImpl::Type::SEND) &&
        (comm2->src_buff_ != simcall_comm_isend__get__src_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;

    if ((r1->call_ == Simcall::COMM_IRECV) && (comm2->type_ == kernel::activity::CommImpl::Type::RECEIVE) &&
        (comm2->dst_buff_ != simcall_comm_irecv__get__dst_buff(r1)) && simcall_comm_wait__get__timeout(r2) <= 0)
      return false;
  }

  /* FIXME: the following rule assumes that the result of the isend/irecv call is not stored in a buffer used in the
   * test call. */
#if 0
  if((r1->call == Simcall::COMM_ISEND || r1->call == Simcall::COMM_IRECV)
      &&  r2->call == Simcall::COMM_TEST)
    return false;
#endif

  if (r1->call_ == Simcall::COMM_WAIT && r2->call_ == Simcall::COMM_TEST &&
      (comm1->src_actor_.get() == nullptr || comm1->dst_actor_.get() == nullptr))
    return false;

  if (r1->call_ == Simcall::COMM_TEST &&
      (simcall_comm_test__get__comm(r1) == nullptr || comm1->src_buff_ == nullptr || comm1->dst_buff_ == nullptr))
    return false;

  if (r1->call_ == Simcall::COMM_TEST && r2->call_ == Simcall::COMM_WAIT && comm1->src_buff_ == comm2->src_buff_ &&
      comm1->dst_buff_ == comm2->dst_buff_)
    return false;

  if (r1->call_ == Simcall::COMM_WAIT && r2->call_ == Simcall::COMM_TEST && comm1->src_buff_ != nullptr &&
      comm1->dst_buff_ != nullptr && comm2->src_buff_ != nullptr && comm2->dst_buff_ != nullptr &&
      comm1->dst_buff_ != comm2->src_buff_ && comm1->dst_buff_ != comm2->dst_buff_ &&
      comm2->dst_buff_ != comm1->src_buff_)
    return false;

  return true;
}

bool Api::simcall_check_dependency(smx_simcall_t const req1, smx_simcall_t const req2) const
{
  const auto ISEND = Simcall::COMM_ISEND;
  const auto IRECV = Simcall::COMM_IRECV;
  const auto TEST  = Simcall::COMM_TEST;
  const auto WAIT  = Simcall::COMM_WAIT;

  if (req1->issuer_ == req2->issuer_)
    return false;

  /* The independence theorem only consider 4 simcalls. All others are dependent with anything. */
  if (req1->call_ != ISEND && req1->call_ != IRECV && req1->call_ != TEST && req1->call_ != WAIT)
    return true;
  if (req2->call_ != ISEND && req2->call_ != IRECV && req2->call_ != TEST && req2->call_ != WAIT)
    return true;

  /* Timeouts in wait transitions are not considered by the independence theorem, thus assumed dependent */
  if ((req1->call_ == WAIT && simcall_comm_wait__get__timeout(req1) > 0) ||
      (req2->call_ == WAIT && simcall_comm_wait__get__timeout(req2) > 0))
    return true;

  if (req1->call_ != req2->call_)
    return request_depend_asymmetric(req1, req2) && request_depend_asymmetric(req2, req1);

  // Those are internal requests, we do not need indirection because those objects are copies:
  const auto comm1 = get_comm_or_nullptr(req1);
  const auto comm2 = get_comm_or_nullptr(req2);

  switch (req1->call_) {
    case Simcall::COMM_ISEND:
      return simcall_comm_isend__get__mbox(req1) == simcall_comm_isend__get__mbox(req2);
    case Simcall::COMM_IRECV:
      return simcall_comm_irecv__get__mbox(req1) == simcall_comm_irecv__get__mbox(req2);
    case Simcall::COMM_WAIT:
      if (comm1->src_buff_ == comm2->src_buff_ && comm1->dst_buff_ == comm2->dst_buff_)
        return false;
      if (comm1->src_buff_ != nullptr && comm1->dst_buff_ != nullptr && comm2->src_buff_ != nullptr &&
          comm2->dst_buff_ != nullptr && comm1->dst_buff_ != comm2->src_buff_ && comm1->dst_buff_ != comm2->dst_buff_ &&
          comm2->dst_buff_ != comm1->src_buff_)
        return false;
      return true;
    default:
      return true;
  }
}

xbt::string const& Api::get_actor_host_name(smx_actor_t actor) const
{
  if (mc_model_checker == nullptr)
    return actor->get_host()->get_name();

  const simgrid::mc::RemoteSimulation* process = &mc_model_checker->get_remote_simulation();

  // Read the simgrid::xbt::string in the MCed process:
  simgrid::mc::ActorInformation* info = actor_info_cast(actor);
  auto remote_string_address =
      remote(reinterpret_cast<const simgrid::xbt::string_data*>(&actor->get_host()->get_name()));
  simgrid::xbt::string_data remote_string = process->read(remote_string_address);
  std::vector<char> hostname(remote_string.len + 1);
  // no need to read the terminating null byte, and thus hostname[remote_string.len] is guaranteed to be '\0'
  process->read_bytes(hostname.data(), remote_string.len, remote(remote_string.data));
  info->hostname = &mc_model_checker->get_host_name(hostname.data());
  return *info->hostname;
}

std::string Api::get_actor_name(smx_actor_t actor) const
{
  if (mc_model_checker == nullptr)
    return actor->get_cname();

  simgrid::mc::ActorInformation* info = actor_info_cast(actor);
  if (info->name.empty()) {
    const simgrid::mc::RemoteSimulation* process = &mc_model_checker->get_remote_simulation();

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
      res += std::string(get_actor_host_name(actor)) + " (" + get_actor_name(actor) + ")";
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
  static const char* name = nullptr;
  if (not name) {
    name = "simgrid::kernel::actor::maxpid";
    if (mc_model_checker->get_remote_simulation().find_variable(name) == nullptr)
      name = "maxpid"; // We seem to miss the namespaces when compiling with GCC
  }
  unsigned long maxpid;
  mc_model_checker->get_remote_simulation().read_variable(name, &maxpid, sizeof(maxpid));
  return maxpid;
}

int Api::get_actors_size() const
{
  return mc_model_checker->get_remote_simulation().actors().size();
}

RemotePtr<kernel::activity::CommImpl> Api::get_comm_isend_raw_addr(smx_simcall_t request) const
{
  return remote(static_cast<kernel::activity::CommImpl*>(simcall_comm_isend__getraw__result(request)));
}

RemotePtr<kernel::activity::CommImpl> Api::get_comm_waitany_raw_addr(smx_simcall_t request, int value) const
{
  auto addr      = simcall_comm_waitany__getraw__comms(request) + value;
  auto comm_addr = mc_model_checker->get_remote_simulation().read(remote(addr));
  return RemotePtr<kernel::activity::CommImpl>(static_cast<kernel::activity::CommImpl*>(comm_addr));
}

std::string Api::get_pattern_comm_rdv(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  Remote<kernel::activity::CommImpl> temp_activity;
  mc_model_checker->get_remote_simulation().read(temp_activity, addr);
  const kernel::activity::CommImpl* activity = temp_activity.get_buffer();

  char* remote_name = mc_model_checker->get_remote_simulation().read<char*>(RemotePtr<char*>(
      (uint64_t)(activity->get_mailbox() ? &activity->get_mailbox()->get_name() : &activity->mbox_cpy->get_name())));
  auto rdv          = mc_model_checker->get_remote_simulation().read_string(RemotePtr<char>(remote_name));
  return rdv;
}

unsigned long Api::get_pattern_comm_src_proc(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  Remote<kernel::activity::CommImpl> temp_activity;
  mc_model_checker->get_remote_simulation().read(temp_activity, addr);
  const kernel::activity::CommImpl* activity = temp_activity.get_buffer();
  auto src_proc =
      mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(activity->src_actor_.get()))->get_pid();
  return src_proc;
}

unsigned long Api::get_pattern_comm_dst_proc(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  Remote<kernel::activity::CommImpl> temp_activity;
  mc_model_checker->get_remote_simulation().read(temp_activity, addr);
  const kernel::activity::CommImpl* activity = temp_activity.get_buffer();
  auto src_proc =
      mc_model_checker->get_remote_simulation().resolve_actor(mc::remote(activity->dst_actor_.get()))->get_pid();
  return src_proc;
}

std::vector<char> Api::get_pattern_comm_data(RemotePtr<kernel::activity::CommImpl> const& addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  std::vector<char> buffer{};
  if (comm->src_buff_ != nullptr) {
    buffer.resize(comm->src_buff_size_);
    mc_model_checker->get_remote_simulation().read_bytes(buffer.data(), buffer.size(), remote(comm->src_buff_));
  }
  return buffer;
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

smx_actor_t Api::get_src_actor(RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, comm_addr);
  const simgrid::kernel::activity::CommImpl* comm = temp_comm.get_buffer();

  auto src_proc = mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(comm->src_actor_.get()));
  return src_proc;
}

smx_actor_t Api::get_dst_actor(RemotePtr<kernel::activity::CommImpl> const& comm_addr) const
{
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, comm_addr);
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

void Api::mc_check_deadlock() const
{
  if (mc_model_checker->checkDeadlock()) {
    MC_show_deadlock();
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
  for (auto& actor : mc_model_checker->get_remote_simulation().actors())
    if (actor.address == address)
      return actor.copy.get_buffer();
  for (auto& actor : mc_model_checker->get_remote_simulation().dead_actors())
    if (actor.address == address)
      return actor.copy.get_buffer();

  xbt_die("Issuer not found");
}

long Api::simcall_get_actor_id(s_smx_simcall const* req) const
{
  return simcall_get_issuer(req)->get_pid();
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

std::list<transition_detail_t> Api::get_enabled_transitions(simgrid::mc::State* state) const
{
  std::list<transition_detail_t> tr_list{};

  for (auto& actor : mc_model_checker->get_remote_simulation().actors()) {
    auto actor_pid  = actor.copy.get_buffer()->get_pid();
    auto actor_impl = actor.copy.get_buffer();

    // Only consider the actors that were marked as interleaving by the checker algorithm
    if (not state->actor_states_[actor_pid].is_todo())
      continue;
    // Not executable in the application
    if (not simgrid::mc::actor_is_enabled(actor_impl))
      continue;

    auto transition       = std::make_unique<s_transition_detail>();
    Simcall simcall_call  = actor_impl->simcall_.call_;
    smx_simcall_t simcall = &actor_impl->simcall_;
    transition->call_     = simcall_call;
    switch (simcall_call) {
      case Simcall::COMM_ISEND:
      case Simcall::COMM_IRECV:
        transition->mbox_remote_addr = get_mbox_remote_addr(simcall);
        transition->comm_remote_addr = get_comm_remote_addr(simcall);
        break;

      default:
        break;
    }
    tr_list.emplace_back(std::move(transition));
  }
  
  return tr_list;
}

std::string Api::request_to_string(smx_simcall_t req, int value) const
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  std::string type;
  std::string args;

  smx_actor_t issuer = simcall_get_issuer(req);

  if (issuer->simcall_.observer_ != nullptr)
    return mc_model_checker->simcall_to_string(issuer->get_pid(), value);

  switch (req->call_) {
    case Simcall::COMM_ISEND:
      type = "iSend";
      args = "src=" + get_actor_string(issuer);
      args += ", buff=" + pointer_to_string(simcall_comm_isend__get__src_buff(req));
      args += ", size=" + buff_size_to_string(simcall_comm_isend__get__src_buff_size(req));
      break;

    case Simcall::COMM_IRECV: {
      size_t* remote_size = simcall_comm_irecv__get__dst_buff_size(req);
      size_t size         = 0;
      if (remote_size)
        mc_model_checker->get_remote_simulation().read_bytes(&size, sizeof(size), remote(remote_size));

      type = "iRecv";
      args = "dst=" + get_actor_string(issuer);
      args += ", buff=" + pointer_to_string(simcall_comm_irecv__get__dst_buff(req));
      args += ", size=" + buff_size_to_string(size);
      break;
    }

    case Simcall::COMM_WAIT: {
      simgrid::kernel::activity::CommImpl* remote_act = simcall_comm_wait__getraw__comm(req);
      if (value == -1) {
        type = "WaitTimeout";
        args = "comm=" + pointer_to_string(remote_act);
      } else {
        type = "Wait";

        simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_activity;
        const simgrid::kernel::activity::CommImpl* act;
        mc_model_checker->get_remote_simulation().read(temp_activity, remote(remote_act));
        act = temp_activity.get_buffer();

        smx_actor_t src_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->src_actor_.get()));
        smx_actor_t dst_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->dst_actor_.get()));
        args = "comm=" + pointer_to_string(remote_act);
        args += " [" + get_actor_string(src_proc) + "-> " + get_actor_string(dst_proc) + "]";
      }
      break;
    }

    case Simcall::COMM_TEST: {
      simgrid::kernel::activity::CommImpl* remote_act = simcall_comm_test__getraw__comm(req);
      simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_activity;
      const simgrid::kernel::activity::CommImpl* act;
      mc_model_checker->get_remote_simulation().read(temp_activity, remote(remote_act));
      act = temp_activity.get_buffer();

      if (act->src_actor_.get() == nullptr || act->dst_actor_.get() == nullptr) {
        type = "Test FALSE";
        args = "comm=" + pointer_to_string(remote_act);
      } else {
        type = "Test TRUE";

        smx_actor_t src_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->src_actor_.get()));
        smx_actor_t dst_proc =
            mc_model_checker->get_remote_simulation().resolve_actor(simgrid::mc::remote(act->dst_actor_.get()));
        args = "comm=" + pointer_to_string(remote_act);
        args += " [" + get_actor_string(src_proc) + " -> " + get_actor_string(dst_proc) + "]";
      }
      break;
    }

    case Simcall::COMM_WAITANY: {
      type         = "WaitAny";
      size_t count = simcall_comm_waitany__get__count(req);
      if (count > 0) {
        simgrid::kernel::activity::CommImpl* remote_sync;
        remote_sync =
            mc_model_checker->get_remote_simulation().read(remote(simcall_comm_waitany__get__comms(req) + value));
        args = "comm=" + pointer_to_string(remote_sync) + xbt::string_printf("(%d of %zu)", value + 1, count);
      } else
        args = "comm at idx " + std::to_string(value);
      break;
    }

    case Simcall::COMM_TESTANY:
      if (value == -1) {
        type = "TestAny FALSE";
        args = "-";
      } else {
        type = "TestAny";
        args = xbt::string_printf("(%d of %zu)", value + 1, simcall_comm_testany__get__count(req));
      }
      break;

    default:
      type = SIMIX_simcall_name(req->call_);
      args = "??";
      break;
  }

  return "[" + get_actor_string(issuer) + "] " + type + "(" + args + ")";
}

std::string Api::request_get_dot_output(smx_simcall_t req, int value) const
{
  const smx_actor_t issuer = simcall_get_issuer(req);
  const char* color        = get_color(issuer->get_pid() - 1);

  std::string label;

  if (req->observer_ != nullptr) {
    label = mc_model_checker->simcall_dot_label(issuer->get_pid(), value);
  } else
    switch (req->call_) {
      case Simcall::COMM_ISEND:
        label = "[" + get_actor_dot_label(issuer) + "] iSend";
        break;

      case Simcall::COMM_IRECV:
        label = "[" + get_actor_dot_label(issuer) + "] iRecv";
        break;

      case Simcall::COMM_WAIT:
        if (value == -1) {
          label = "[" + get_actor_dot_label(issuer) + "] WaitTimeout";
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
          label = "[" + get_actor_dot_label(issuer) + "] Wait";
          label += " [(" + std::to_string(src_proc ? src_proc->get_pid() : 0) + ")";
          label += "->(" + std::to_string(dst_proc ? dst_proc->get_pid() : 0) + ")]";
        }
        break;

      case Simcall::COMM_TEST: {
        kernel::activity::ActivityImpl* remote_act = simcall_comm_test__getraw__comm(req);
        Remote<simgrid::kernel::activity::CommImpl> temp_comm;
        mc_model_checker->get_remote_simulation().read(temp_comm,
                                                       remote(static_cast<kernel::activity::CommImpl*>(remote_act)));
        const kernel::activity::CommImpl* comm = temp_comm.get_buffer();
        if (comm->src_actor_.get() == nullptr || comm->dst_actor_.get() == nullptr) {
          label = "[" + get_actor_dot_label(issuer) + "] Test FALSE";
        } else {
          label = "[" + get_actor_dot_label(issuer) + "] Test TRUE";
        }
        break;
      }

      case Simcall::COMM_WAITANY:
        label = "[" + get_actor_dot_label(issuer) + "] WaitAny";
        label += xbt::string_printf(" [%d of %zu]", value + 1, simcall_comm_waitany__get__count(req));
        break;

      case Simcall::COMM_TESTANY:
        if (value == -1) {
          label = "[" + get_actor_dot_label(issuer) + "] TestAny FALSE";
        } else {
          label = "[" + get_actor_dot_label(issuer) + "] TestAny TRUE";
          label += xbt::string_printf(" [%d of %zu]", value + 1, simcall_comm_testany__get__count(req));
        }
        break;

      default:
        THROW_UNIMPLEMENTED;
    }

  return "label = \"" + label + "\", color = " + color + ", fontcolor = " + color;
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

void Api::execute(Transition& transition, smx_simcall_t simcall) const
{
  /* FIXME: once all simcalls have observers, kill the simcall parameter and use mc_model_checker->simcall_to_string() */
  transition.textual = request_to_string(simcall, transition.times_considered_);
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
