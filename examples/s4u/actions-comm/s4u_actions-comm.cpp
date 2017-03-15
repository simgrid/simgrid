/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/replay.hpp"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(actions, "Messages specific for this msg example");

#define ACT_DEBUG(...) \
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {  \
    char *NAME = xbt_str_join_array(action, " ");              \
    XBT_DEBUG(__VA_ARGS__);                                    \
    xbt_free(NAME);                                            \
  } else ((void)0)

static void log_action(const char *const *action, double date)
{
  if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, date);
    xbt_free(name);
  }
}

class Replayer {
public:
  explicit Replayer(std::vector<std::string> args)
  {
    int argc;
    char* argv[2];
    argv[0] = &args.at(0)[0];
    if (args.size() == 1) {
      argc = 1;
    } else {
      argc    = 2;
      argv[1] = &args.at(1)[0];
    }
    xbt_replay_action_runner(argc, argv);
  }

  void operator()()
  {
    // Nothing to do here
  }

  /* My actions */
  static void compute(const char* const* action)
  {
    double amount = std::stod(action[2]);
    double clock  = simgrid::s4u::Engine::getClock();
    ACT_DEBUG("Entering %s", NAME);
    simgrid::s4u::this_actor::execute(amount);
    log_action(action, simgrid::s4u::Engine::getClock() - clock);
  }

  static void send(const char* const* action)
  {
    double size                 = std::stod(action[3]);
    char* payload               = xbt_strdup(action[3]);
    double clock                = simgrid::s4u::Engine::getClock();
    simgrid::s4u::MailboxPtr to = simgrid::s4u::Mailbox::byName(simgrid::s4u::this_actor::name() + "_" + action[2]);
    ACT_DEBUG("Entering Send: %s (size: %g) -- Actor %s on mailbox %s", NAME, size,
              simgrid::s4u::this_actor::name().c_str(), to->name());
    simgrid::s4u::this_actor::send(to, payload, size);
    xbt_free(payload);

    log_action(action, simgrid::s4u::Engine::getClock() - clock);
  }

  static void recv(const char* const* action)
  {
    double clock = simgrid::s4u::Engine::getClock();
    simgrid::s4u::MailboxPtr from =
        simgrid::s4u::Mailbox::byName(std::string(action[2]) + "_" + simgrid::s4u::this_actor::name());

    ACT_DEBUG("Receiving: %s -- Actor %s on mailbox %s", NAME, simgrid::s4u::this_actor::name().c_str(), from->name());
    simgrid::s4u::this_actor::recv(from);
    log_action(action, simgrid::s4u::Engine::getClock() - clock);
  }
};

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
  /* Explicit initialization of the action module is required now*/
  _xbt_replay_action_init();

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file [action_files]\n"
                       "\t# if all actions are in the same file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml actions\n"
                       "\t# if actions are in separate files, specified in deployment\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml ",
             argv[0], argv[0], argv[0]);

  e->loadPlatform(argv[1]);
  e->registerDefault(&xbt_replay_action_runner);
  e->registerFunction<Replayer>("p0");
  e->registerFunction<Replayer>("p1");
  e->loadDeployment(argv[2]);

  /*   Action registration */
  xbt_replay_action_register("compute", Replayer::compute);
  xbt_replay_action_register("send", Replayer::send);
  xbt_replay_action_register("recv", Replayer::recv);

  /* Actually do the simulation using MSG_action_trace_run */
  if (argv[3]) {
    simgrid::xbt::action_fs = new std::ifstream(argv[3], std::ifstream::in);
  }

  e->run();

  if (argv[3]) {
    delete simgrid::xbt::action_fs;
    simgrid::xbt::action_fs = nullptr;
  }

  XBT_INFO("Simulation time %g", e->getClock());

  _xbt_replay_action_exit(); /* Explicit finalization of the action module */

  return 0;
}
