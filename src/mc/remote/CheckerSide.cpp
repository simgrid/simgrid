/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include "simgrid/Exception.hpp"
#include "src/mc/api/Strategy.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_environ.h"
#include "src/mc/remote/Channel.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "xbt/asserts.h"
#include "xbt/config.hpp"
#include "xbt/log.h"
#include "xbt/system_error.hpp"
#include <cerrno>
#include <unistd.h>
#include <utility>

#ifdef __linux__
#include <sys/prctl.h>
#include <sys/sysinfo.h>
#endif

#include <boost/tokenizer.hpp>
#include <csignal>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#ifdef __linux__
#define WAITPID_CHECKED_FLAGS __WALL
#else
#define WAITPID_CHECKED_FLAGS 0
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkerside, mc, "MC communication with the application");

static simgrid::config::Flag<std::string> _sg_mc_setenv{
    "model-check/setenv", "Extra environment variables to pass to the child process (ex: 'AZE=aze;QWE=qwe').", "",
    [](std::string_view value) {
      xbt_assert(value.empty() || value.find('=', 0) != std::string_view::npos,
                 "The 'model-check/setenv' parameter must be like 'AZE=aze', but it does not contain an equal sign.");
    }};

namespace simgrid::mc {

unsigned CheckerSide::count_ = 0;

XBT_ATTRIB_NORETURN static void run_child_process(int socket, const std::vector<char*>& args)
{
  /* On startup, EngineImpl::initialize() calls simgrid::mc::AppSide::get(), which checks whether the MC_ENV_SOCKET_FD
   * env variable is set. If so, MC mode is assumed, and the client is setup from its side
   */

#ifdef __linux__
  // Make sure we do not outlive our parent
  sigset_t mask;
  sigemptyset(&mask);
  xbt_assert(sigprocmask(SIG_SETMASK, &mask, nullptr) >= 0, "Could not unblock signals");
  xbt_assert(prctl(PR_SET_PDEATHSIG, SIGHUP) == 0, "Could not PR_SET_PDEATHSIG");
#endif

  setenv(MC_ENV_SOCKET_FD, std::to_string(socket).c_str(), 1);

  /* Setup the tokenizer that parses the cfg:model-check/setenv parameter */
  using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> semicol_sep(";");
  boost::char_separator<char> equal_sep("=");
  Tokenizer token_vars(_sg_mc_setenv.get(), semicol_sep); /* Iterate over all FOO=foo parts */
  for (const auto& token : token_vars) {
    std::vector<std::string> kv;
    Tokenizer token_kv(token, equal_sep);
    for (const auto& t : token_kv) /* Iterate over 'FOO' and then 'foo' in that 'FOO=foo' */
      kv.push_back(t);
    xbt_assert(kv.size() == 2, "Parse error on 'model-check/setenv' value %s. Does it contain an equal sign?",
               token.c_str());
    XBT_INFO("setenv '%s'='%s'", kv[0].c_str(), kv[1].c_str());
    setenv(kv[0].c_str(), kv[1].c_str(), 1);
  }

  /* And now, exec the child process */
  int i = 1;
  while (args[i] != nullptr && args[i][0] == '-')
    i++;

  xbt_assert(args[i] != nullptr,
             "Unable to find a binary to exec on the command line. Did you only pass config flags?");

  execvp(args[i], args.data() + i);
  XBT_CRITICAL("The model-checked process failed to exec(%s): %s.\n"
               "        Make sure that your binary exists on disk and is executable.",
               args[i], strerror(errno));
  if (strchr(args[i], '=') != nullptr)
    XBT_CRITICAL("If you want to pass environment variables to the application, please use --cfg=model-check/setenv:%s",
                 args[i]);

  xbt_die("Aborting now.");
}

static void wait_application_process(pid_t pid)
{
  XBT_DEBUG("Waiting for the model-checked process");
  int status;

  // The model-checked process SIGSTOP itself to signal it's ready:
  xbt_assert(waitpid(pid, &status, WAITPID_CHECKED_FLAGS) == pid && WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
             "Could not wait model-checked process");

  errno = 0;
#ifdef __linux__
  ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEEXIT);
  ptrace(PTRACE_CONT, pid, 0, 0);
#elif defined BSD
  ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);
#else
  xbt_die("no ptrace equivalent coded for this platform, stateful model-checking is impossible.");
#endif
  xbt_assert(errno == 0,
             "Ptrace does not seem to be usable in your setup (errno: %d). "
             "If you run from within a docker, adding `--cap-add SYS_PTRACE` to the docker line may help. "
             "If it does not help, please report this bug.",
             errno);
  XBT_DEBUG("%d ptrace correctly setup.", getpid());
}

/* When this constructor is called, no other checkerside exists */
CheckerSide::CheckerSide(const std::vector<char*>& args)
{
  count_++;
  XBT_DEBUG("Create a CheckerSide.");

  // Create an AF_UNIX socketpair used for exchanging messages between the model-checker process (ancestor)
  // and the application process (child)
  int sockets[2];
  xbt_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != -1,
             "Could not create socketpair: %s.\nPlease increase the file limit with `ulimit -n 10000`.",
             strerror(errno));

  pid_ = fork();
  xbt_assert(pid_ >= 0, "Could not fork application process");

  if (pid_ == 0) { // Child
    ::close(sockets[1]);

    run_child_process(sockets[0], args);
    DIE_IMPOSSIBLE;
  }

  // Parent (model-checker):
  ::close(sockets[0]);
  channel_.reset_socket(sockets[1]);

  try {
    wait_for_requests();
  } catch (const AssertionError& ae) {
    XBT_CRITICAL("Failed to get an answer from the child. The error was '%s'\n%s", ae.what(),
                 ae.resolve_backtrace().c_str());
    XBT_CRITICAL("Use valgrind to check whether your child process is segfaulting on start (instead of verifying your "
                 "code with simgrid-mc, verify valgrind that runs your code).");
    xbt_abort();
  }
}

CheckerSide::~CheckerSide()
{
  count_--;
}

/* This constructor is called when cloning a checkerside to get its application to fork away */
CheckerSide::CheckerSide(int socket, CheckerSide* child_checker)
    : channel_(socket, child_checker->channel_), child_checker_(child_checker)
{
  count_++;

  auto* answer = (s_mc_message_int_t*)(get_channel().expect_message(sizeof(s_mc_message_int_t), MessageType::FORK_REPLY,
                                                                    "Could not receive answer to FORK_REPLY"));

  xbt_assert(answer->value != 0, "Error while forking the application.");
  pid_ = answer->value;

  wait_for_requests();
}

static void handle_sigalarm(int)
{
  xbt_die("The child process failed to connect within the 5 seconds time limit. The model-checker is bailing out now.");
}

std::unique_ptr<CheckerSide> CheckerSide::clone(int master_socket, const std::string& master_socket_name)
{
  if (is_one_way)
    return nullptr;

  s_mc_message_fork_t m = {};
  m.type                = MessageType::FORK;
  xbt_assert(master_socket_name.size() == MC_SOCKET_NAME_LEN);
  std::copy_n(begin(master_socket_name), MC_SOCKET_NAME_LEN, begin(m.socket_name));
  xbt_assert(get_channel().send(m) == 0, "Could not ask the app to fork on need.");

  /* Accept an incomming socket under a 5 seconds time limit*/
  struct sigaction action;
  action.sa_handler = handle_sigalarm;
  sigaction(SIGALRM, &action, nullptr); /* Override the default behaviour which would be to end the process */
  alarm(5);
  int sock = accept(master_socket, nullptr /* I know who's connecting*/, nullptr);
  int real_errno = errno;
  alarm(0);
  errno = real_errno;

  if (sock <= 0) {
    switch (errno) {
      case EMFILE:
        xbt_die("Cannot accept the incomming connection of the forked app: the per-process limit on the number of open "
                "file has been reached (errno: EMFILE).\n"
                "You may want to increase the limit using for example `ulimit -n 10000` to improve the overall "
                "performance.");
      case ENFILE:
        xbt_die("Cannot accept the incomming connection of the forked app: the system-wide limit on the number of open "
                "file has been reached (errno: ENFILE).\n"
                "If you want to push the limit, try increasing the value in /proc/sys/fs/file-max (at your own risk).");
      default:
        perror("Cannot accept the incomming connection of the forked app.");
        xbt_die("Bailing out now.");
    }
  }

  return std::make_unique<CheckerSide>(sock, this);
}

void CheckerSide::peek_assertion_failure()
{
  /* Handle an ASSERTION message if any */
  auto [more_data, type] = get_channel().peek_message_type();
  if (not more_data) // The app closed the socket. It must be dead by now.
    handle_waitpid();
  if (type == MessageType::ASSERTION_FAILED)
    Exploration::get_instance()->report_assertion_failure(); // This is a noreturn function
}

Transition* CheckerSide::handle_simcall(aid_t aid, int times_considered, bool new_transition)
{
  if (not is_one_way) {
    s_mc_message_simcall_execute_t execute = {};
    execute.type                           = MessageType::SIMCALL_EXECUTE;
    execute.aid_                           = aid;
    execute.times_considered_              = times_considered;
    execute.want_transition                = new_transition;
    get_channel().send(execute);

    sync_with_app(); // The app may send messages while processing the transition
  } else {
    XBT_DEBUG("Not sending EXECUTE as we're one way");
  }

  peek_assertion_failure();

  auto* answer = (s_mc_message_simcall_execute_answer_t*)(get_channel().expect_message(
      sizeof(s_mc_message_simcall_execute_answer_t), MessageType::SIMCALL_EXECUTE_REPLY,
      "Could not receive answer to SIMCALL_EXECUTE"));
  xbt_assert(answer->aid == aid, "The application did not execute the expected actor (expected %ld, ran %ld)", aid,
             answer->aid);

  if (new_transition) {
    auto* t = deserialize_transition(aid, times_considered, channel_);
    XBT_DEBUG("Got a transtion: %s", t->to_string(true).c_str());
    t->deserialize_memory_operations(channel_);
    return t;
  }
  XBT_DEBUG("No need for transitions today");
  return nullptr;
}

void CheckerSide::handle_replay(std::deque<std::pair<aid_t, int>> to_replay,
                                std::deque<std::pair<aid_t, int>> to_replay_and_actor_status)
{
  long unsigned msg_length = to_replay.size() + to_replay_and_actor_status.size() + 1;

  s_mc_message_int_t replay_msg = {};
  replay_msg.type  = MessageType::REPLAY;
  replay_msg.value              = msg_length;

  unsigned char* aids  = (unsigned char*)alloca(sizeof(unsigned char) * msg_length);
  unsigned char* times = (unsigned char*)alloca(sizeof(unsigned char) * msg_length);

  XBT_DEBUG("send a replay of size %lu", msg_length);
  int i = 0;
  for (auto const& [aid, time] : to_replay) {
    xbt_assert(aid < 254, "Overflow on the aid value. %ld is too big to fit in an unsigned char", aid);
    xbt_assert(time < 254, "Overflow on the time_considered value. %d is too big to fit in an unsigned char", time);
    aids[i]  = aid;
    times[i] = time;
    i++;
  }
  // Signal the end of the first part
  aids[i]  = -1;
  times[i] = -1;
  i++;

  for (auto const& [aid, time] : to_replay_and_actor_status) {
    xbt_assert(aid < 254, "Overflow on the aid value. %ld is too big to fit in an unsigned char", aid);
    xbt_assert(time < 254, "Overflow on the time_considered value. %d is too big to fit in an unsigned char", time);
    aids[i]  = aid;
    times[i] = time;
    i++;
  }

  get_channel().pack(&replay_msg, sizeof(replay_msg));
  get_channel().pack(aids, sizeof(unsigned char) * msg_length);
  get_channel().pack(times, sizeof(unsigned char) * msg_length);

  xbt_assert(get_channel().send() == 0, "Could not send message to the app: %s", strerror(errno));

  // Wait for the application to signal that it is waiting
  if (to_replay_and_actor_status.size() == 0)
    sync_with_app();
}

void CheckerSide::finalize(bool terminate_asap)
{
  if (is_one_way) {
    this->killed_by_us_ = true;
    kill(pid_, SIGTERM);
    return;
  }
  s_mc_message_int_t m = {};
  m.type               = MessageType::FINALIZE;
  m.value              = terminate_asap;
  xbt_assert(get_channel().send(m) == 0, "Could not ask the app to finalize on need");

  get_channel().expect_message(sizeof(s_mc_message_t), MessageType::FINALIZE_REPLY,
                               "Could not receive answer to FINALIZE");
}

aid_t CheckerSide::get_aid_of_next_transition()
{
  auto [more_data, type] = get_channel().peek_message_type();

  if (not more_data) { // The app closed the socket. It must be dead by now.
    handle_waitpid();
    return -1;
  }

  if (type == MessageType::WAITING) {
    get_channel().expect_message(sizeof(s_mc_message_t), MessageType::WAITING,
                                 "Could not receive MessageType::WAITING");
    is_one_way = false;
    return -1;
  } else {
    auto [more_data2, got] = get_channel().peek(sizeof(struct s_mc_message_simcall_execute_answer_t));
    xbt_assert(more_data2);
    auto* msg = static_cast<struct s_mc_message_simcall_execute_answer_t*>(got);
    xbt_assert(msg->type == MessageType::SIMCALL_EXECUTE_REPLY,
               "The next message on the wire is not a SIMCALL_EXECUTE as expected but a %s",
               is_valid_MessageType((int)msg->type) ? to_c_str(msg->type) : "invalid message");

    return msg->aid;
  }
}

void CheckerSide::sync_with_app()
{
  /* Handle an ASSERTION message if any */
  auto [more_data, type] = get_channel().peek_message_type();
  if (not more_data) // The app closed the socket. It must be dead by now.
    handle_waitpid();

  if (type == MessageType::ASSERTION_FAILED)
    Exploration::get_instance()->report_assertion_failure(); // This is a noreturn function

  if (is_one_way)
    return;

  /* Stop waiting for eventual ASSERTION message when we get something else, that must be WAITING */
  get_channel().expect_message(sizeof(s_mc_message_t), MessageType::WAITING, "Could not receive MessageType::WAITING");
}

void CheckerSide::wait_for_requests()
{
  if (is_one_way)
    return;
  XBT_DEBUG("Resume the application");
  if (get_channel().send(MessageType::CONTINUE) != 0)
    throw xbt::errno_error();

  sync_with_app();
}

void CheckerSide::handle_dead_child(int status)
{
  // From PTRACE_O_TRACEEXIT:
#ifdef __linux__
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))) {
    unsigned long eventmsg;
    xbt_assert(ptrace(PTRACE_GETEVENTMSG, pid_, 0, &eventmsg) != -1, "Could not get exit status");
    status = static_cast<int>(eventmsg);
    if (WIFSIGNALED(status)) {
      Exploration::get_instance()->report_crash(status);
    }
  }
#endif

  // We don't care about non-lethal signals, just reinject them:
  if (WIFSTOPPED(status)) {
    XBT_DEBUG("Stopped with signal %i", (int)WSTOPSIG(status));
    errno = 0;
#ifdef __linux__
    ptrace(PTRACE_CONT, pid_, 0, WSTOPSIG(status));
#elif defined BSD
    ptrace(PT_CONTINUE, pid_, (caddr_t)1, WSTOPSIG(status));
#endif
    xbt_assert(errno == 0, "Could not PTRACE_CONT: %s", strerror(errno));
  }

  else if (WIFSIGNALED(status)) {
    if (not killed_by_us_) // We did not send the kill signal ourselves, so report it
      Exploration::get_instance()->report_crash(status);
  } else if (WIFEXITED(status)) {
    XBT_DEBUG("Child process is over");
  }
}

void CheckerSide::handle_waitpid()
{
  XBT_DEBUG("%d checks for wait event. %s", getpid(),
            child_checker_ == nullptr ? "Wait directly." : "Ask our proxy to wait for its child.");

  if (child_checker_ == nullptr) { // Wait directly
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) != 0) {
      if (pid == -1) {
        xbt_assert(errno == ECHILD, "Could not wait for pid: %s", strerror(errno));
        break; // Having no more children to wait for is OK
      }

      if (pid == get_pid())
        handle_dead_child(status);
      else
        THROW_IMPOSSIBLE;
    }

  } else { // Ask our proxy to wait for us
    s_mc_message_int_t request = {};
    request.type               = MessageType::WAIT_CHILD;
    request.value              = pid_;
    xbt_assert(child_checker_->get_channel().send(request) == 0,
               "Could not ask my child to waitpid its child for me: %s", strerror(errno));

    auto answer = (s_mc_message_int_t*)child_checker_->get_channel().expect_message(
        sizeof(s_mc_message_int_t), MessageType::WAIT_CHILD_REPLY, "Could not receive MessageType::WAIT_CHILD_REPLY");

    handle_dead_child(answer->value);
  }
}

void CheckerSide::go_one_way()
{

  if (!is_one_way) {
    is_one_way = true;
    s_mc_message_one_way_t msg{};
    msg.type = MessageType::GO_ONE_WAY;
    msg.want_transitions = Exploration::need_actor_status_transitions();
    msg.is_random        = (_sg_mc_strategy == "uniform");
    msg.random_seed      = _sg_mc_random_seed;
    xbt_assert(get_channel().send(msg) == 0, "Could not ask the application to go one way: %s", strerror(errno));
  }
}

void CheckerSide::terminate_one_way()
{
  xbt_assert(is_one_way);

  auto [more_data, type] = get_channel().peek_message_type();
  while (more_data && type != MessageType::WAITING) {
    get_channel().expect_message(sizeof(type), type, "Could not receive the Message");
    auto [first, second] = get_channel().peek_message_type();
    more_data            = first;
    type                 = second;
  }

  get_channel().expect_message(sizeof(s_mc_message_t), MessageType::WAITING, "Could not receive MessageType::WAITING");
  is_one_way = false;
}

} // namespace simgrid::mc
