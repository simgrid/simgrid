/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/AppSide.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/internal_config.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_environ.h"
#include "src/mc/remote/mc_protocol.h"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/random.hpp"
#include "xbt/sysdep.h"
#include <algorithm>
#include <cstddef>
#include <sstream>
#if HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif
#include "src/sthread/sthread.h"
#include "src/xbt/coverage.h"
#include "xbt/str.h"
#include <simgrid/modelchecker.h>

#include <cerrno>
#include <cinttypes>
#include <cstdio> // setvbuf
#include <cstdlib>
#include <memory>
#include <numeric>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client, mc, "MC client logic");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

namespace simgrid::mc {

std::unique_ptr<AppSide> AppSide::instance_;

AppSide* AppSide::get()
{
  // Only initialize the MC world once
  if (instance_ != nullptr)
    return instance_.get();

  if (std::getenv(MC_ENV_SOCKET_FD) == nullptr) // We are not in MC mode: don't initialize the MC world
    return nullptr;

  XBT_DEBUG("Initialize the MC world.");

  simgrid::mc::set_model_checking_mode(ModelCheckingMode::APP_SIDE);

  setvbuf(stdout, nullptr, _IOLBF, 0);

  // Fetch socket from MC_ENV_SOCKET_FD:
  const char* fd_env = std::getenv(MC_ENV_SOCKET_FD);
  int fd             = xbt_str_parse_int(fd_env, "Not a number in variable '" MC_ENV_SOCKET_FD "'");
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  instance_ = std::make_unique<simgrid::mc::AppSide>(fd);

  // If we plan to fork, remove the SIGINT handler that would get messed up by all the forked childs
  if (not _sg_mc_nofork)
    std::signal(SIGINT, SIG_DFL);

  instance_->handle_messages();
  return instance_.get();
}

void AppSide::handle_deadlock_check(const s_mc_message_int_t* request)
{
  const auto* engine     = kernel::EngineImpl::get_instance();
  const auto& actor_list = engine->get_actor_list();
  bool deadlock = not actor_list.empty() && std::none_of(begin(actor_list), end(actor_list), [](const auto& kv) {
    return mc::actor_is_enabled(kv.second);
  });

  if (deadlock && request->value) {
    XBT_CINFO(mc_global, "**************************");
    XBT_CINFO(mc_global, "*** DEADLOCK DETECTED ***");
    XBT_CINFO(mc_global, "**************************");
    engine->display_all_actor_status();
  }
  // Send result:
  s_mc_message_int_t answer = {};
  answer.type               = MessageType::DEADLOCK_CHECK_REPLY;
  answer.value              = deadlock;
  xbt_assert(channel_.send(answer) == 0, "Could not send response: %s", strerror(errno));
}

void AppSide::send_executed_transition(kernel::actor::ActorImpl* actor, bool want_transition)
{
  s_mc_message_simcall_execute_answer_t answer{};
  answer.type = MessageType::SIMCALL_EXECUTE_REPLY;
  answer.aid  = actor->get_pid();
  channel_.pack(answer);

  if (want_transition) {
    if (actor->simcall_.observer_ != nullptr) {
      actor->simcall_.observer_->serialize(channel_);
      actor->recorded_memory_accesses_->serialize(channel_);
    } else {
      channel_.pack(mc::Transition::Type::UNKNOWN);
    }
    XBT_VERB("send SIMCALL_EXECUTE_REPLY(%s:%ld) with a transition", actor->get_cname(), actor->get_pid());
  } else
    XBT_VERB("send SIMCALL_EXECUTE_REPLY(%s:%ld) with no transition inside", actor->get_cname(), actor->get_pid());

  xbt_assert(channel_.send() == 0, "Could not send response: %s", strerror(errno));
}

void AppSide::handle_simcall_execute(const s_mc_message_simcall_execute_t* message)
{
  kernel::actor::ActorImpl* actor = kernel::EngineImpl::get_instance()->get_actor_by_pid(message->aid_);
  xbt_assert(actor != nullptr, "Invalid pid %ld", message->aid_);
  xbt_assert(
      (actor->simcall_.observer_ == nullptr && actor->simcall_.call_ != simgrid::kernel::actor::Simcall::Type::NONE) ||
          (actor->simcall_.observer_ != nullptr && actor->simcall_.observer_->is_enabled()),
      "Please, model-checker, don't execute disabled transitions. You tried to execute %s which is disabled",
      actor->simcall_.observer_->to_string().c_str());

  // The client may send some messages to the server while processing the transition
  actor->simcall_handle(message->times_considered_);
  // Say the server that the transition is over and that it should proceed
  xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send MESSAGE_WAITING to model-checker: %s",
             strerror(errno));

  // Finish the RPC from the server: return a serialized observer, to build a Transition on Checker side
  send_executed_transition(actor, message->want_transition);
}

void AppSide::handle_replay(const s_mc_message_int_t* msg)
{
  XBT_DEBUG("Starting to handle the replay");
  unsigned replay_size = msg->value;

  auto [more_aid, aids] = channel_.receive(sizeof(unsigned char) * replay_size);
  if (not more_aid)
    ::_Exit(0); // Nobody's listening to that process anymore => exit as quickly as possible.

  auto [more_times, times] = channel_.receive(sizeof(unsigned char) * replay_size);
  if (not more_times)
    ::_Exit(0); // Nobody's listening to that process anymore => exit as quickly as possible.

  XBT_DEBUG("Going to replay %u transitions", replay_size - 1);
  unsigned i = 0;
  for (; i < replay_size; i++) {
    aid_t aid            = ((unsigned char*)aids)[i];
    int times_considered = ((unsigned char*)times)[i];

    // The -1 value is used to indicate the part requiring transition and status
    if (aid == 255 and times_considered == 255) // -1 in unsigned char
      break;

    XBT_VERB("MC asked to replay %ld(nb_times=%d)", aid, times_considered);
    kernel::actor::ActorImpl* actor = kernel::EngineImpl::get_instance()->get_actor_by_pid(aid);
    xbt_assert(actor != nullptr, "Invalid pid %ld", aid);
    xbt_assert((actor->simcall_.observer_ == nullptr &&
                actor->simcall_.call_ != simgrid::kernel::actor::Simcall::Type::NONE) ||
                   (actor->simcall_.observer_ != nullptr && actor->simcall_.observer_->is_enabled()),
               "Please, model-checker, don't execute disabled transitions. You tried to execute %s which is disabled",
               actor->simcall_.observer_->to_string().c_str());

    actor->simcall_handle(times_considered);
    simgrid::mc::execute_actors();
  }
  i++;
  XBT_DEBUG("Now going to ask for actor status for the next %u transitions", replay_size - i - 1);
  if (i >= replay_size)
    // Say the server that the replay is over and that it should proceed
    xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send MESSAGE_WAITING to model-checker: %s",
               strerror(errno));

  // else, the actor status will tell when the replay is over
  for (; i < replay_size; i++) {
    aid_t aid            = ((unsigned char*)aids)[i];
    int times_considered = ((unsigned char*)times)[i];

    XBT_VERB("MC asked to replay %ld(nb_times=%d)", aid, times_considered);
    kernel::actor::ActorImpl* actor = kernel::EngineImpl::get_instance()->get_actor_by_pid(aid);
    xbt_assert(actor != nullptr, "Invalid pid %ld", aid);
    xbt_assert((actor->simcall_.observer_ == nullptr &&
                actor->simcall_.call_ != simgrid::kernel::actor::Simcall::Type::NONE) ||
                   (actor->simcall_.observer_ != nullptr && actor->simcall_.observer_->is_enabled()),
               "Please, model-checker, don't execute disabled transitions. You tried to execute %s which is disabled",
               actor->simcall_.observer_->to_string().c_str());

    actor->simcall_handle(times_considered);
    // The checker is asking for this message specifically to update the incoming
    // transition, so let's sent it with 'true'
    send_executed_transition(actor, true);

    simgrid::mc::execute_actors();
    // The false doesn't look safe, and it would be better to modify the msg content to rlly know if the checker want
    // the transition or not
    send_actor_status(false);
  }
}

void AppSide::handle_one_way(const s_mc_message_one_way_t* msg)
{
  auto* engine                   = kernel::EngineImpl::get_instance();
  bool is_random                 = msg->is_random;
  static bool initialized_random = false;
  if (not initialized_random and is_random) {
    xbt::random::set_mersenne_seed(msg->random_seed);
    initialized_random = true;
  }

  std::vector<aid_t> fireables;
  for (auto const& [aid, actor] : engine->get_actor_list())
    if (mc::actor_is_enabled(actor))
      fireables.emplace_back(aid);
  if (not is_random)
    std::sort(fireables.begin(), fireables.end());
  while (fireables.size() > 0) {
    XBT_DEBUG("App<%d> is now going one way! There are %lu actors to run here", getpid(), fireables.size());

    unsigned long chosen =
        is_random ? xbt::random::uniform_int(0, fireables.size() - 1) : 0; // The first aid since fireables is sorted

    XBT_DEBUG("Picked actor %ld to run", fireables[chosen]);
    aid_t chosen_aid = engine->get_actor_by_pid(fireables[chosen])->get_pid();

    kernel::actor::ActorImpl* actor = kernel::EngineImpl::get_instance()->get_actor_by_pid(chosen_aid);
    xbt_assert(actor != nullptr, "Invalid pid %ld", chosen_aid);
    xbt_assert((actor->simcall_.observer_ == nullptr &&
                actor->simcall_.call_ != simgrid::kernel::actor::Simcall::Type::NONE) ||
                   (actor->simcall_.observer_ != nullptr && actor->simcall_.observer_->is_enabled()),
               "Please, model-checker, don't execute disabled transitions. You tried to execute %s which is disabled",
               actor->simcall_.observer_->to_string().c_str());

    // Since the AppSide is allowed to decide what to pick, this means this is the first time we come here
    // hence, let's play the transition with considered_times = 0
    actor->simcall_handle(0);

    // Sending the transition message with the actual transition
    send_executed_transition(actor, true);
    // Make a step in the app
    simgrid::mc::execute_actors(); // May raise assertion errors, every packed data must be send before this.

    // Sending the actor status
    auto const& actor_list = kernel::EngineImpl::get_instance()->get_actor_list();
    XBT_DEBUG("Serialize the actors to answer ACTORS_STATUS from the checker. %zu actors to go.", actor_list.size());

    struct s_mc_message_actors_status_answer_t answer = {};
    answer.type                                       = MessageType::ACTORS_STATUS_REPLY_COUNT;
    answer.count                                      = static_cast<int>(actor_list.size());
    channel_.pack(answer);

    if (answer.count > 0) {
      size_t status_size = actor_list.size() * sizeof(s_mc_message_actors_status_one_t);
      auto* status       = (s_mc_message_actors_status_one_t*)xbt_malloc0(status_size);

      int i = 0;
      for (auto const& [aid, actor] : actor_list) {
        xbt_assert(actor);
        xbt_assert(actor->simcall_.observer_, "simcall %s in actor %s has no observer.", actor->simcall_.get_cname(),
                   actor->get_cname());
        status[i].type           = MessageType::ACTORS_STATUS_REPLY_TRANSITION;
        status[i].aid            = aid;
        status[i].enabled        = mc::actor_is_enabled(actor);
        status[i].max_considered = actor->simcall_.observer_->get_max_consider();
        i++;
      }

      channel_.pack(status, status_size);

      if (msg->want_transitions) {
        // Serialize each transition to describe what each actor is doing
        XBT_DEBUG("Deliver ACTOR_TRANSITION_PROBE payload");
        for (int i = 0; i < answer.count; i++) {
          const auto& actor        = actor_list.at(status[i].aid);
          const int max_considered = status[i].max_considered;

          for (int times_considered = 0; times_considered < max_considered; times_considered++) {
            if (actor->simcall_.observer_ != nullptr) {
              actor->simcall_.observer_->prepare(times_considered);
              actor->simcall_.observer_->serialize(channel_);
            } else {
              channel_.pack(mc::Transition::Type::UNKNOWN);
            }
          }
        }
      }
    }
    xbt_assert(channel_.send() == 0, "Could not send ACTOR_EXECUTE answer: %s", strerror(errno));
    // re-compute the fireables actor so we can move on
    fireables.clear();
    for (auto const& [aid, actor] : engine->get_actor_list())
      if (mc::actor_is_enabled(actor))
        fireables.emplace_back(aid);
    if (not is_random)
      std::sort(fireables.begin(), fireables.end());
  }
  XBT_DEBUG("I've finished guys! What do I do now ?");

  xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send WAITING message to model-checker: %s",
             strerror(errno));
}

void AppSide::handle_finalize(const s_mc_message_int_t* msg)
{
  bool terminate_asap = msg->value;
  XBT_DEBUG("Finalize (terminate = %d)", (int)terminate_asap);
  if (not terminate_asap) {
    if (XBT_LOG_ISENABLED(mc_client, xbt_log_priority_debug))
      kernel::EngineImpl::get_instance()->display_all_actor_status();
#if HAVE_SMPI
    XBT_DEBUG("Smpi_enabled: %d", SMPI_is_inited());
    if (SMPI_is_inited())
      SMPI_finalize();
#endif
  }
  coverage_checkpoint();
  xbt_assert(channel_.send(MessageType::FINALIZE_REPLY) == 0, "Could not answer to FINALIZE: %s", strerror(errno));
  std::fflush(stdout);
  if (terminate_asap)
    ::_Exit(0);
}
void AppSide::handle_fork(const s_mc_message_fork_t* msg)
{
  static bool first_time = true;
  if (first_time && std::strcmp(simgrid::s4u::Engine::get_instance()->get_context_factory_name(), "thread") == 0) {
    s_mc_message_int_t answer = {.type = MessageType::FORK_REPLY, .value = 0};
    xbt_assert(channel_.send(answer) == 0, "Could not send failure as a response to FORK: %s", strerror(errno));

    xbt_die("The SimGrid model-checker cannot handle the threaded context factory because it relies on forking the "
            "application, which is not possible with multi-threaded applications. Please remove the "
            "--cfg=contexts/factory:thread parameter to the application, or add --cfg=model-check/no-fork:on to "
            "simgrid-mc. The second option results in slower explorations, but it is the only option when verifying "
            "Python programs, as the SimGrid Python bindings mandate threads.");
    // See also commit c2c077dc7edb4a4217610dbaf675f414cbf6f09f for the reason why Python needs threads
  }
  first_time = false;

  int status;
  int pid;
  /* Reap any zombie child, saving its status for later use in AppSide::handle_wait_child() */
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    child_statuses_[pid] = status;

  pid = fork();
  xbt_assert(pid >= 0, "Could not fork application sub-process: %s.", strerror(errno));

  if (pid == 0) { // Child
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr = {};
    addr.sun_family         = AF_UNIX;
    std::copy_n(begin(msg->socket_name), MC_SOCKET_NAME_LEN, addr.sun_path);

    xbt_assert(connect(sock, (struct sockaddr*)&addr, sizeof addr) >= 0, "Cannot connect to Checker on %c%s: %s.",
               (addr.sun_path[0] ? addr.sun_path[0] : '@'), addr.sun_path + 1, strerror(errno));

    channel_.reset_socket(sock);

    s_mc_message_int_t answer = {};
    answer.type               = MessageType::FORK_REPLY;
    answer.value              = getpid();
    xbt_assert(channel_.send(answer) == 0, "Could not send response to FORK: %s", strerror(errno));
  } else {
    XBT_VERB("App %d forks subprocess %d.", getpid(), pid);
  }
}
void AppSide::handle_wait_child(const s_mc_message_int_t* msg)
{
  int status;
  errno = 0;
  if (auto search = child_statuses_.find(msg->value); search != child_statuses_.end()) {
    status = search->second;
    child_statuses_.erase(search); // We only need this info once
  } else {
    waitpid(msg->value, &status, 0);
  }
  xbt_assert(errno == 0, "Cannot wait on behalf of the checker: %s.", strerror(errno));

  s_mc_message_int_t answer = {};
  answer.type               = MessageType::WAIT_CHILD_REPLY;
  answer.value              = status;
  xbt_assert(channel_.send(answer) == 0, "Could not send response to WAIT_CHILD: %s", strerror(errno));
}

void AppSide::send_actor_status(bool want_transitions)
{
  auto const& actor_list = kernel::EngineImpl::get_instance()->get_actor_list();
  XBT_DEBUG("Serialize the actors to answer ACTORS_STATUS from the checker. %zu actors to go.", actor_list.size());

  struct s_mc_message_actors_status_answer_t answer = {};
  answer.type                                       = MessageType::ACTORS_STATUS_REPLY_COUNT;
  answer.count                                      = static_cast<int>(actor_list.size());
  channel_.pack(answer);
  XBT_DEBUG("Pack ACTORS_STATUS_REPLY with count %d", answer.count);

  if (answer.count > 0) {
    size_t status_size = actor_list.size() * sizeof(s_mc_message_actors_status_one_t);
    auto* status       = (s_mc_message_actors_status_one_t*)alloca(status_size);
    memset(status, 0, status_size);

    int i = 0;
    for (auto const& [aid, actor] : actor_list) {
      xbt_assert(actor);
      xbt_assert(actor->simcall_.observer_, "simcall %s in actor %s has no observer.", actor->simcall_.get_cname(),
                 actor->get_cname());
      status[i].type           = MessageType::ACTORS_STATUS_REPLY_TRANSITION;
      status[i].aid            = aid;
      status[i].enabled        = mc::actor_is_enabled(actor);
      status[i].max_considered = actor->simcall_.observer_->get_max_consider();
      i++;
    }

    channel_.pack(status, status_size);

    if (want_transitions) {
      // Serialize each transition to describe what each actor is doing
      XBT_DEBUG("pack the transitions");
      for (int i = 0; i < answer.count; i++) {
        const auto& actor        = actor_list.at(status[i].aid);
        const int max_considered = status[i].max_considered;

        for (int times_considered = 0; times_considered < max_considered; times_considered++) {
          if (actor->simcall_.observer_ != nullptr) {
            actor->simcall_.observer_->prepare(times_considered);
            actor->simcall_.observer_->serialize(channel_);
          } else {
            channel_.pack(mc::Transition::Type::UNKNOWN);
          }
        }
        // NOTE: We do NOT need to reset `times_considered` for each actor's
        // simcall observer here to the "original" value (i.e. the value BEFORE
        // multiple prepare() calls were made for serialization purposes) since
        // each SIMCALL_EXECUTE provides a `times_considered` to be used to prepare
        // the transition before execution.
      }
    } else {
      XBT_DEBUG("Do not pack the transitions: the checker don't want them");
    }
  }
  xbt_assert(channel_.send() == 0, "Could not send ACTORS_STATUS_REPLY: %s", strerror(errno));
  XBT_DEBUG("Done sending ACTORS_STATUS_REPLY");
}

void AppSide::handle_actors_status(const s_mc_message_actors_status_t* msg)
{
  send_actor_status(msg->want_transitions_);
}

void AppSide::handle_actors_maxpid()
{
  s_mc_message_int_t answer = {};
  answer.type               = MessageType::ACTORS_MAXPID_REPLY;
  answer.value              = kernel::actor::ActorImpl::get_maxpid();
  xbt_assert(channel_.send(answer) == 0, "Could not send response: %s", strerror(errno));
}

void AppSide::handle_messages()
{
  while (true) { // Until we get a CONTINUE message
    XBT_DEBUG("Waiting messages from the model-checker");

    auto [more_data, msg_type] = channel_.peek_message_type();

    if (not more_data) {
      XBT_DEBUG("Socket closed on the Checker side, bailing out.");
      ::_Exit(0); // Nobody's listening to that process anymore => exit as quickly as possible.
    }
    XBT_DEBUG("Got a %s message from the model-checker", to_c_str(msg_type));

    std::pair<bool, void*> received;
    switch (msg_type) {
      case MessageType::CONTINUE:
        received = channel_.receive(sizeof(s_mc_message_t));
        return;

      case MessageType::DEADLOCK_CHECK:
        received = channel_.receive(sizeof(s_mc_message_int_t));
        handle_deadlock_check((s_mc_message_int_t*)received.second);
        break;

      case MessageType::SIMCALL_EXECUTE:
        received = channel_.receive(sizeof(s_mc_message_simcall_execute_t));
        handle_simcall_execute((s_mc_message_simcall_execute_t*)received.second);
        break;

      case MessageType::REPLAY:
        received = channel_.receive(sizeof(s_mc_message_int_t));
        handle_replay((s_mc_message_int_t*)received.second);
        break;

      case MessageType::GO_ONE_WAY:
        received = channel_.receive(sizeof(s_mc_message_one_way_t));
        handle_one_way((s_mc_message_one_way_t*)received.second);
        break;

      case MessageType::FINALIZE:
        received = channel_.receive(sizeof(s_mc_message_int_t));
        handle_finalize((s_mc_message_int_t*)received.second);
        break;

      case MessageType::FORK:
        received = channel_.receive(sizeof(s_mc_message_fork_t));
        handle_fork((s_mc_message_fork_t*)received.second);
        break;

      case MessageType::WAIT_CHILD:
        received = channel_.receive(sizeof(s_mc_message_int_t));
        handle_wait_child((s_mc_message_int_t*)received.second);
        break;

      case MessageType::ACTORS_STATUS:
        received = channel_.receive(sizeof(s_mc_message_actors_status_t));
        handle_actors_status((s_mc_message_actors_status_t*)received.second);
        break;

      case MessageType::ACTORS_MAXPID:
        received = channel_.receive(sizeof(s_mc_message_t));
        handle_actors_maxpid();
        break;

      default:
        xbt_die("Received unexpected message %s (%i)", to_c_str(msg_type), static_cast<int>(msg_type));
        break;
    }
  }
}

void AppSide::main_loop()
{
  simgrid::mc::processes_time.resize(simgrid::kernel::actor::ActorImpl::get_maxpid());

  sthread_disable();
  coverage_checkpoint();
  sthread_enable();

  while (true) {
    simgrid::mc::execute_actors();
    xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send WAITING message to model-checker: %s",
               strerror(errno));
    this->handle_messages();
  }
}

void AppSide::report_assertion_failure()
{
  xbt_assert(channel_.send(MessageType::ASSERTION_FAILED) == 0, "Could not send assertion to model-checker: %s",
             strerror(errno));
  this->handle_messages();
}

} // namespace simgrid::mc
