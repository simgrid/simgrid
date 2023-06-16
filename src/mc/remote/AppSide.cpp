/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/AppSide.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/internal_config.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_environ.h"
#if SIMGRID_HAVE_STATEFUL_MC
#include "src/mc/sosp/RemoteProcessMemory.hpp"
#endif
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

  XBT_DEBUG("Initialize the MC world. %s=%s", MC_ENV_NEED_PTRACE, std::getenv(MC_ENV_NEED_PTRACE));

  simgrid::mc::set_model_checking_mode(ModelCheckingMode::APP_SIDE);

  setvbuf(stdout, nullptr, _IOLBF, 0);

  // Fetch socket from MC_ENV_SOCKET_FD:
  const char* fd_env = std::getenv(MC_ENV_SOCKET_FD);
  int fd             = xbt_str_parse_int(fd_env, "Not a number in variable '" MC_ENV_SOCKET_FD "'");
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  instance_ = std::make_unique<simgrid::mc::AppSide>(fd);

  // Wait for the model-checker:
  if (getenv(MC_ENV_NEED_PTRACE) != nullptr) {
    errno = 0;
#if defined __linux__
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
#elif defined BSD
    ptrace(PT_TRACE_ME, 0, nullptr, 0);
#else
    xbt_die("no ptrace equivalent coded for this platform, please don't use the liveness checker here.");
#endif

    xbt_assert(errno == 0 && raise(SIGSTOP) == 0, "Could not wait for the model-checker (errno = %d: %s)", errno,
               strerror(errno));
  }

  instance_->handle_messages();
  return instance_.get();
}

void AppSide::handle_deadlock_check(const s_mc_message_t*) const
{
  const auto* engine     = kernel::EngineImpl::get_instance();
  const auto& actor_list = engine->get_actor_list();
  bool deadlock = not actor_list.empty() && std::none_of(begin(actor_list), end(actor_list), [](const auto& kv) {
    return mc::actor_is_enabled(kv.second);
  });

  if (deadlock) {
    XBT_CINFO(mc_global, "**************************");
    XBT_CINFO(mc_global, "*** DEADLOCK DETECTED ***");
    XBT_CINFO(mc_global, "**************************");
    engine->display_all_actor_status();
  }
  // Send result:
  s_mc_message_int_t answer = {};
  answer.type  = MessageType::DEADLOCK_CHECK_REPLY;
  answer.value = deadlock;
  xbt_assert(channel_.send(answer) == 0, "Could not send response: %s", strerror(errno));
}
void AppSide::handle_simcall_execute(const s_mc_message_simcall_execute_t* message) const
{
  kernel::actor::ActorImpl* actor = kernel::EngineImpl::get_instance()->get_actor_by_pid(message->aid_);
  xbt_assert(actor != nullptr, "Invalid pid %ld", message->aid_);

  // The client may send some messages to the server while processing the transition
  actor->simcall_handle(message->times_considered_);
  // Say the server that the transition is over and that it should proceed
  xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send MESSAGE_WAITING to model-checker: %s",
             strerror(errno));

  // Finish the RPC from the server: return a serialized observer, to build a Transition on Checker side
  s_mc_message_simcall_execute_answer_t answer = {};
  answer.type                                  = MessageType::SIMCALL_EXECUTE_REPLY;
  std::stringstream stream;
  if (actor->simcall_.observer_ != nullptr) {
    actor->simcall_.observer_->serialize(stream);
  } else {
    stream << (short)mc::Transition::Type::UNKNOWN;
  }
  std::string str = stream.str();
  xbt_assert(str.size() + 1 <= answer.buffer.size(),
             "The serialized simcall is too large for the buffer. Please fix the code.");
  strncpy(answer.buffer.data(), str.c_str(), answer.buffer.size() - 1);
  answer.buffer.back() = '\0';

  XBT_DEBUG("send SIMCALL_EXECUTE_ANSWER(%s) ~> '%s'", actor->get_cname(), str.c_str());
  xbt_assert(channel_.send(answer) == 0, "Could not send response: %s", strerror(errno));
}

void AppSide::handle_finalize(const s_mc_message_int_t* msg) const
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
  int status;
  int pid;
  /* Reap any zombie child, saving its status for later use in AppSide::handle_wait_child() */
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    child_statuses_[pid] = status;

  pid = fork();
  xbt_assert(pid >= 0, "Could not fork application sub-process: %s.", strerror(errno));

  if (pid == 0) { // Child
    int sock = socket(AF_UNIX,
#ifdef __APPLE__
                      SOCK_STREAM, /* Mac OSX does not have AF_UNIX + SOCK_SEQPACKET, even if that's faster*/
#else
                      SOCK_SEQPACKET,
#endif
                      0);

    struct sockaddr_un addr = {};
    addr.sun_family         = AF_UNIX;
    std::copy_n(begin(msg->socket_name), MC_SOCKET_NAME_LEN, addr.sun_path);

    xbt_assert(connect(sock, (struct sockaddr*)&addr, sizeof addr) >= 0, "Cannot connect to Checker on %c%s: %s.",
               (addr.sun_path[0] ? addr.sun_path[0] : '@'), addr.sun_path + 1, strerror(errno));

    channel_.reset_socket(sock);

    s_mc_message_int_t answer = {};
    answer.type               = MessageType::FORK_REPLY;
    answer.value              = getpid();
    xbt_assert(channel_.send(answer) == 0, "Could not send response to WAIT_CHILD_REPLY: %s", strerror(errno));
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
void AppSide::handle_need_meminfo()
{
#if SIMGRID_HAVE_STATEFUL_MC
  this->need_memory_info_                  = true;
  s_mc_message_need_meminfo_reply_t answer = {};
  answer.type                              = MessageType::NEED_MEMINFO_REPLY;
  answer.mmalloc_default_mdp               = mmalloc_get_current_heap();
  xbt_assert(channel_.send(answer) == 0, "Could not send response to the request for meminfo: %s", strerror(errno));
#else
  xbt_die("SimGrid was compiled without MC suppport, so liveness and similar features are not available.");
#endif
}
void AppSide::handle_actors_status() const
{
  auto const& actor_list = kernel::EngineImpl::get_instance()->get_actor_list();
  XBT_DEBUG("Serialize the actors to answer ACTORS_STATUS from the checker. %zu actors to go.", actor_list.size());

  std::vector<s_mc_message_actors_status_one_t> status;
  for (auto const& [aid, actor] : actor_list) {
    s_mc_message_actors_status_one_t one = {};
    one.type                             = MessageType::ACTORS_STATUS_REPLY_TRANSITION;
    one.aid                              = aid;
    one.enabled                          = mc::actor_is_enabled(actor);
    one.max_considered                   = actor->simcall_.observer_->get_max_consider();
    status.push_back(one);
  }

  struct s_mc_message_actors_status_answer_t answer = {};
  answer.type                                       = MessageType::ACTORS_STATUS_REPLY_COUNT;
  answer.count                                      = static_cast<int>(status.size());

  xbt_assert(channel_.send(answer) == 0, "Could not send ACTORS_STATUS_REPLY msg: %s", strerror(errno));
  if (answer.count > 0) {
    size_t size = status.size() * sizeof(s_mc_message_actors_status_one_t);
    xbt_assert(channel_.send(status.data(), size) == 0, "Could not send ACTORS_STATUS_REPLY data: %s", strerror(errno));
  }

  // Serialize each transition to describe what each actor is doing
  XBT_DEBUG("Deliver ACTOR_TRANSITION_PROBE payload");
  for (const auto& actor_status : status) {
    const auto& actor        = actor_list.at(actor_status.aid);
    const int max_considered = actor_status.max_considered;

    for (int times_considered = 0; times_considered < max_considered; times_considered++) {
      std::stringstream stream;
      s_mc_message_simcall_probe_one_t probe;
      probe.type = MessageType::ACTORS_STATUS_REPLY_SIMCALL;

      if (actor->simcall_.observer_ != nullptr) {
        actor->simcall_.observer_->prepare(times_considered);
        actor->simcall_.observer_->serialize(stream);
      } else {
        stream << (short)mc::Transition::Type::UNKNOWN;
      }

      std::string str = stream.str();
      xbt_assert(str.size() + 1 <= probe.buffer.size(),
                 "The serialized transition is too large for the buffer. Please fix the code.");
      strncpy(probe.buffer.data(), str.c_str(), probe.buffer.size() - 1);
      probe.buffer.back() = '\0';

      xbt_assert(channel_.send(probe) == 0, "Could not send ACTOR_TRANSITION_PROBE payload: %s", strerror(errno));
    }
    // NOTE: We do NOT need to reset `times_considered` for each actor's
    // simcall observer here to the "original" value (i.e. the value BEFORE
    // multiple prepare() calls were made for serialization purposes) since
    // each SIMCALL_EXECUTE provides a `times_considered` to be used to prepare
    // the transition before execution.
  }
}
void AppSide::handle_actors_maxpid() const
{
  s_mc_message_int_t answer = {};
  answer.type               = MessageType::ACTORS_MAXPID_REPLY;
  answer.value              = kernel::actor::ActorImpl::get_maxpid();
  xbt_assert(channel_.send(answer) == 0, "Could not send response: %s", strerror(errno));
}

#define assert_msg_size(_name_, _type_)                                                                                \
  xbt_assert(received_size == sizeof(_type_), "Unexpected size for " _name_ " (%zd != %zu)", received_size,            \
             sizeof(_type_))

void AppSide::handle_messages()
{
  while (true) { // Until we get a CONTINUE message
    XBT_DEBUG("Waiting messages from the model-checker");

    std::array<char, MC_MESSAGE_LENGTH> message_buffer;
    ssize_t received_size = channel_.receive(message_buffer.data(), message_buffer.size());

    if (received_size == 0) {
      XBT_DEBUG("Socket closed on the Checker side, bailing out.");
      ::_Exit(0); // Nobody's listening to that process anymore => exit as quickly as possible.
    }
    xbt_assert(received_size >= 0, "Could not receive commands from the model-checker: %s", strerror(errno));
    xbt_assert(static_cast<size_t>(received_size) >= sizeof(s_mc_message_t), "Cannot handle short message (size=%zd)",
               received_size);

    const s_mc_message_t* message = (s_mc_message_t*)message_buffer.data();
    switch (message->type) {
      case MessageType::CONTINUE:
        assert_msg_size("MESSAGE_CONTINUE", s_mc_message_t);
        return;

      case MessageType::DEADLOCK_CHECK:
        assert_msg_size("DEADLOCK_CHECK", s_mc_message_t);
        handle_deadlock_check(message);
        break;

      case MessageType::SIMCALL_EXECUTE:
        assert_msg_size("SIMCALL_EXECUTE", s_mc_message_simcall_execute_t);
        handle_simcall_execute((s_mc_message_simcall_execute_t*)message_buffer.data());
        break;

      case MessageType::FINALIZE:
        assert_msg_size("FINALIZE", s_mc_message_int_t);
        handle_finalize((s_mc_message_int_t*)message_buffer.data());
        break;

      case MessageType::FORK:
        assert_msg_size("FORK", s_mc_message_fork_t);
        handle_fork((s_mc_message_fork_t*)message_buffer.data());
        break;

      case MessageType::WAIT_CHILD:
        assert_msg_size("WAIT_CHILD", s_mc_message_int_t);
        handle_wait_child((s_mc_message_int_t*)message_buffer.data());
        break;

      case MessageType::NEED_MEMINFO:
        assert_msg_size("NEED_MEMINFO", s_mc_message_t);
        handle_need_meminfo();
        break;

      case MessageType::ACTORS_STATUS:
        assert_msg_size("ACTORS_STATUS", s_mc_message_t);
        handle_actors_status();
        break;

      case MessageType::ACTORS_MAXPID:
        assert_msg_size("ACTORS_MAXPID", s_mc_message_t);
        handle_actors_maxpid();
        break;

      default:
        xbt_die("Received unexpected message %s (%i)", to_c_str(message->type), static_cast<int>(message->type));
        break;
    }
  }
}

void AppSide::main_loop()
{
  simgrid::mc::processes_time.resize(simgrid::kernel::actor::ActorImpl::get_maxpid());
  MC_ignore_heap(simgrid::mc::processes_time.data(),
                 simgrid::mc::processes_time.size() * sizeof(simgrid::mc::processes_time[0]));
  kernel::activity::CommImpl::setup_mc();

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

void AppSide::ignore_memory(void* addr, std::size_t size) const
{
  if (not MC_is_active() || not need_memory_info_)
    return;

#if SIMGRID_HAVE_STATEFUL_MC
  s_mc_message_ignore_memory_t message = {};
  message.type = MessageType::IGNORE_MEMORY;
  message.addr = (std::uintptr_t)addr;
  message.size = size;
  xbt_assert(channel_.send(message) == 0, "Could not send IGNORE_MEMORY message to model-checker: %s", strerror(errno));
#else
  xbt_die("Cannot really call ignore_memory() in non-SIMGRID_MC mode.");
#endif
}

void AppSide::unignore_memory(void* addr, std::size_t size) const
{
  if (not MC_is_active() || not need_memory_info_)
    return;

#if SIMGRID_HAVE_STATEFUL_MC
  s_mc_message_ignore_memory_t message = {};
  message.type                         = MessageType::UNIGNORE_MEMORY;
  message.addr                         = (std::uintptr_t)addr;
  message.size                         = size;
  xbt_assert(channel_.send(message) == 0, "Could not send UNIGNORE_MEMORY message to model-checker: %s",
             strerror(errno));
#else
  xbt_die("Cannot really call unignore_memory() in non-SIMGRID_MC mode.");
#endif
}

void AppSide::ignore_heap(void* address, std::size_t size) const
{
  if (not MC_is_active() || not need_memory_info_)
    return;

#if SIMGRID_HAVE_STATEFUL_MC
  const s_xbt_mheap_t* heap = mmalloc_get_current_heap();

  s_mc_message_ignore_heap_t message = {};
  message.type    = MessageType::IGNORE_HEAP;
  message.address = address;
  message.size    = size;
  message.block   = ((char*)address - (char*)heap->heapbase) / BLOCKSIZE + 1;
  if (heap->heapinfo[message.block].type == 0) {
    message.fragment = -1;
    heap->heapinfo[message.block].busy_block.ignore++;
  } else {
    message.fragment = (ADDR2UINT(address) % BLOCKSIZE) >> heap->heapinfo[message.block].type;
    heap->heapinfo[message.block].busy_frag.ignore[message.fragment]++;
  }

  xbt_assert(channel_.send(message) == 0, "Could not send ignored region to MCer: %s", strerror(errno));
#else
  xbt_die("Cannot really call ignore_heap() in non-SIMGRID_MC mode.");
#endif
}

void AppSide::unignore_heap(void* address, std::size_t size) const
{
  if (not MC_is_active() || not need_memory_info_)
    return;

#if SIMGRID_HAVE_STATEFUL_MC
  s_mc_message_ignore_memory_t message = {};
  message.type = MessageType::UNIGNORE_HEAP;
  message.addr = (std::uintptr_t)address;
  message.size = size;
  xbt_assert(channel_.send(message) == 0, "Could not send IGNORE_HEAP message to model-checker: %s", strerror(errno));
#else
  xbt_die("Cannot really call unignore_heap() in non-SIMGRID_MC mode.");
#endif
}

void AppSide::declare_symbol(const char* name, int* value) const
{
  if (not MC_is_active() || not need_memory_info_) {
    XBT_CRITICAL("Ignore AppSide::declare_symbol(%s)", name);
    return;
  }

#if SIMGRID_HAVE_STATEFUL_MC
  s_mc_message_register_symbol_t message = {};
  message.type = MessageType::REGISTER_SYMBOL;
  xbt_assert(strlen(name) + 1 <= message.name.size(), "Symbol is too long");
  strncpy(message.name.data(), name, message.name.size() - 1);
  message.callback = nullptr;
  message.data     = value;
  xbt_assert(channel_.send(message) == 0, "Could send REGISTER_SYMBOL message to model-checker: %s", strerror(errno));
#else
  xbt_die("Cannot really call declare_symbol() in non-SIMGRID_MC mode.");
#endif
}

/** Register a stack in the model checker
 *
 *  The stacks are allocated in the heap. The MC handle them specifically
 *  when we analyze/compare the content of the heap so it must be told where
 *  they are with this function.
 */
#if HAVE_UCONTEXT_H /* Apple don't want us to use ucontexts */
void AppSide::declare_stack(void* stack, size_t size, ucontext_t* context) const
{
  if (not MC_is_active() || not need_memory_info_)
    return;

#if SIMGRID_HAVE_STATEFUL_MC
  const s_xbt_mheap_t* heap = mmalloc_get_current_heap();

  s_stack_region_t region = {};
  region.address = stack;
  region.context = context;
  region.size    = size;
  region.block   = ((char*)stack - (char*)heap->heapbase) / BLOCKSIZE + 1;

  s_mc_message_stack_region_t message = {};
  message.type         = MessageType::STACK_REGION;
  message.stack_region = region;
  xbt_assert(channel_.send(message) == 0, "Could not send STACK_REGION to model-checker: %s", strerror(errno));
#else
  xbt_die("Cannot really call declare_stack() in non-SIMGRID_MC mode.");
#endif
}
#endif

} // namespace simgrid::mc
