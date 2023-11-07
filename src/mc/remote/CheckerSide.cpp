/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_environ.h"
#include "xbt/config.hpp"
#include "xbt/system_error.hpp"

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <boost/tokenizer.hpp>
#include <csignal>
#include <fcntl.h>
#include <sys/ptrace.h>
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

XBT_ATTRIB_NORETURN static void run_child_process(int socket, const std::vector<char*>& args)
{
  /* On startup, simix_global_init() calls simgrid::mc::Client::initialize(), which checks whether the MC_ENV_SOCKET_FD
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

void CheckerSide::setup_events(bool socket_only)
{
  auto* base = event_base_new();
  base_.reset(base);

  socket_event_ = event_new(
      base, get_channel().get_socket(), EV_READ | EV_PERSIST,
      [](evutil_socket_t, short events, void* arg) {
        auto* checker = static_cast<simgrid::mc::CheckerSide*>(arg);
        if (events == EV_READ) {
          do {
            std::array<char, MC_MESSAGE_LENGTH> buffer;
            ssize_t size = checker->get_channel().receive(buffer.data(), buffer.size(), MSG_DONTWAIT);
            if (size == -1) {
              XBT_ERROR("Channel::receive failure: %s", strerror(errno));
              if (errno != EAGAIN)
                throw simgrid::xbt::errno_error();
            }

            if (size == 0) // The app closed the socket. It must be dead by now.
              checker->handle_waitpid();
            else if (not checker->handle_message(buffer.data(), size)) {
              checker->break_loop();
              break;
            }
          } while (checker->get_channel().has_pending_data());
        } else {
          xbt_die("Unexpected event");
        }
      },
      this);
  event_add(socket_event_, nullptr);

  if (socket_only) {
    signal_event_ = nullptr;
  } else {
    signal_event_ = event_new(
        base, SIGCHLD, EV_SIGNAL | EV_PERSIST,
        [](evutil_socket_t sig, short events, void* arg) {
          auto* checker = static_cast<simgrid::mc::CheckerSide*>(arg);
          if (events == EV_SIGNAL) {
            if (sig == SIGCHLD)
              checker->handle_waitpid();
            else
              xbt_die("Unexpected signal: %d", sig);
          } else {
            xbt_die("Unexpected event");
          }
        },
        this);
    event_add(signal_event_, nullptr);
  }
}

/* When this constructor is called, no other checkerside exists */
CheckerSide::CheckerSide(const std::vector<char*>& args) : running_(true)
{
  XBT_DEBUG("Create a CheckerSide.");

  // Create an AF_UNIX socketpair used for exchanging messages between the model-checker process (ancestor)
  // and the application process (child)
  int sockets[2];
  xbt_assert(socketpair(AF_UNIX,
#ifdef __APPLE__
                        SOCK_STREAM, /* Mac OSX does not have AF_UNIX + SOCK_SEQPACKET, even if that's faster */
#else
                        SOCK_SEQPACKET,
#endif
                        0, sockets) != -1,
             "Could not create socketpair: %s", strerror(errno));

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

  setup_events(false); /* we need a signal handler too */
  wait_for_requests();
}

CheckerSide::~CheckerSide()
{
  event_del(socket_event_);
  event_free(socket_event_);
  if (signal_event_ != nullptr) {
    event_del(signal_event_);
    event_free(signal_event_);
  }
}

/* This constructor is called when cloning a checkerside to get its application to fork away */
CheckerSide::CheckerSide(int socket, CheckerSide* child_checker)
    : channel_(socket, child_checker->channel_), running_(true), child_checker_(child_checker)
{
  setup_events(true); // We already have a signal handled in that case

  s_mc_message_int_t answer;
  ssize_t s = get_channel().receive(answer);
  xbt_assert(s != -1, "Could not receive answer to FORK_REPLY");
  xbt_assert(s == sizeof answer, "Broken message (size=%zd; expected %zu)", s, sizeof answer);
  xbt_assert(answer.type == MessageType::FORK_REPLY,
             "Received unexpected message %s (%i); expected MessageType::FORK_REPLY (%i)", to_c_str(answer.type),
             (int)answer.type, (int)MessageType::FORK_REPLY);
  pid_ = answer.value;

  wait_for_requests();
}

std::unique_ptr<CheckerSide> CheckerSide::clone(int master_socket, const std::string& master_socket_name)
{
  s_mc_message_fork_t m = {};
  m.type                = MessageType::FORK;
  xbt_assert(master_socket_name.size() == MC_SOCKET_NAME_LEN);
  std::copy_n(begin(master_socket_name), MC_SOCKET_NAME_LEN, begin(m.socket_name));
  xbt_assert(get_channel().send(m) == 0, "Could not ask the app to fork on need.");

  int sock = accept(master_socket, nullptr /* I know who's connecting*/, nullptr);
  xbt_assert(sock > 0, "Cannot accept the incomming connection of the forked app: %s.", strerror(errno));

  return std::make_unique<CheckerSide>(sock, this);
}

void CheckerSide::finalize(bool terminate_asap)
{
  s_mc_message_int_t m = {};
  m.type               = MessageType::FINALIZE;
  m.value              = terminate_asap;
  xbt_assert(get_channel().send(m) == 0, "Could not ask the app to finalize on need");

  s_mc_message_t answer;
  ssize_t s = get_channel().receive(answer);
  xbt_assert(s != -1, "Could not receive answer to FINALIZE");
  xbt_assert(s == sizeof answer, "Broken message (size=%zd; expected %zu)", s, sizeof answer);
  xbt_assert(answer.type == MessageType::FINALIZE_REPLY,
             "Received unexpected message %s (%i); expected MessageType::FINALIZE_REPLY (%i)", to_c_str(answer.type),
             (int)answer.type, (int)MessageType::FINALIZE_REPLY);
}

void CheckerSide::dispatch_events() const
{
  event_base_dispatch(base_.get());
}

void CheckerSide::break_loop() const
{
  event_base_loopbreak(base_.get());
}

bool CheckerSide::handle_message(const char* buffer, ssize_t size)
{
  s_mc_message_t base_message;
  ssize_t consumed;
  xbt_assert(size >= (ssize_t)sizeof(base_message), "Broken message. Got only %ld bytes.", size);
  memcpy(&base_message, buffer, sizeof(base_message));

  switch (base_message.type) {

    case MessageType::WAITING:
      consumed = sizeof(s_mc_message_t);
      if (size > consumed) {
        XBT_DEBUG("%d reinject %d bytes after a %s message", getpid(), (int)(size - consumed),
                  to_c_str(base_message.type));
        channel_.reinject(&buffer[consumed], size - consumed);
      }

      return false;

    case MessageType::ASSERTION_FAILED:
      // report_assertion_failure() is NORETURN, but it may change when we report more than one error per run,
      // so please keep the consumed computation even if clang-static detects it as a dead affectation.
      consumed = sizeof(s_mc_message_t);
      Exploration::get_instance()->report_assertion_failure();
      break;

    default:
      xbt_die("Unexpected message from the application");
  }
  if (size > consumed) {
    XBT_DEBUG("%d reinject %d bytes after a %s message", getpid(), (int)(size - consumed), to_c_str(base_message.type));
    channel_.reinject(&buffer[consumed], size - consumed);
  }
  return true;
}

void CheckerSide::wait_for_requests()
{
  XBT_DEBUG("Resume the application");
  if (get_channel().send(MessageType::CONTINUE) != 0)
    throw xbt::errno_error();

  if (running())
    dispatch_events();
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
      this->terminate();
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
    this->terminate();
    Exploration::get_instance()->report_crash(status);
  } else if (WIFEXITED(status)) {
    XBT_DEBUG("Child process is over");
    this->terminate();
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
        if (errno == ECHILD) { // No more children:
          xbt_assert(not this->running(), "Inconsistent state");
          break;
        } else {
          xbt_die("Could not wait for pid: %s", strerror(errno));
        }
      }

      if (pid == get_pid())
        handle_dead_child(status);
    }

  } else { // Ask our proxy to wait for us
    s_mc_message_int_t request = {};
    request.type               = MessageType::WAIT_CHILD;
    request.value              = pid_;
    xbt_assert(child_checker_->get_channel().send(request) == 0,
               "Could not ask my child to waitpid its child for me: %s", strerror(errno));

    s_mc_message_int_t answer;
    ssize_t answer_size = child_checker_->get_channel().receive(answer);
    xbt_assert(answer_size != -1, "Could not receive message");
    xbt_assert(answer.type == MessageType::WAIT_CHILD_REPLY,
               "The received message is not the WAIT_CHILD_REPLY I was expecting but of type %s",
               to_c_str(answer.type));
    xbt_assert(answer_size == sizeof answer, "Broken message (size=%zd; expected %zu)", answer_size, sizeof answer);
    handle_dead_child(answer.value);
  }
}
} // namespace simgrid::mc
