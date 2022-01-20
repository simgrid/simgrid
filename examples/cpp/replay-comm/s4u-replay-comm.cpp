/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/replay.hpp"
#include "xbt/str.h"
#include <boost/algorithm/string/join.hpp>
#include <cinttypes>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(replay_comm, "Messages specific for this example");

#define ACT_DEBUG(...)                                                                                                 \
  if (XBT_LOG_ISENABLED(replay_comm, xbt_log_priority_verbose)) {                                                   \
    std::string NAME = boost::algorithm::join(action, " ");                                                            \
    XBT_DEBUG(__VA_ARGS__);                                                                                            \
  } else                                                                                                               \
  ((void)0)

static void log_action(const simgrid::xbt::ReplayAction& action, double date)
{
  if (XBT_LOG_ISENABLED(replay_comm, xbt_log_priority_verbose)) {
    std::string s = boost::algorithm::join(action, " ");
    XBT_VERB("%s %f", s.c_str(), date);
  }
}

class Replayer {
public:
  explicit Replayer(std::vector<std::string> args)
  {
    const char* actor_name     = args.at(0).c_str();
    if (args.size() > 1) { // split mode, the trace file was provided in the deployment file
      const char* trace_filename = args[1].c_str();
      simgrid::xbt::replay_runner(actor_name, trace_filename);
    } else { // Merged mode
      simgrid::xbt::replay_runner(actor_name);
    }
  }

  void operator()() const
  {
    // Nothing to do here
  }

  /* My actions */
  static void compute(simgrid::xbt::ReplayAction& action)
  {
    double amount = std::stod(action[2]);
    double clock  = simgrid::s4u::Engine::get_clock();
    ACT_DEBUG("Entering %s", NAME.c_str());
    simgrid::s4u::this_actor::execute(amount);
    log_action(action, simgrid::s4u::Engine::get_clock() - clock);
  }

  static void send(simgrid::xbt::ReplayAction& action)
  {
    auto size                 = static_cast<uint64_t>(std::stod(action[3]));
    auto* payload             = new std::string(action[3]);
    double clock              = simgrid::s4u::Engine::get_clock();
    simgrid::s4u::Mailbox* to = simgrid::s4u::Mailbox::by_name(simgrid::s4u::this_actor::get_name() + "_" + action[2]);
    ACT_DEBUG("Entering Send: %s (size: %" PRIu64 ") -- Actor %s on mailbox %s", NAME.c_str(), size,
              simgrid::s4u::this_actor::get_cname(), to->get_cname());
    to->put(payload, size);
    log_action(action, simgrid::s4u::Engine::get_clock() - clock);
  }

  static void recv(simgrid::xbt::ReplayAction& action)
  {
    double clock = simgrid::s4u::Engine::get_clock();
    simgrid::s4u::Mailbox* from =
        simgrid::s4u::Mailbox::by_name(std::string(action[2]) + "_" + simgrid::s4u::this_actor::get_name());

    ACT_DEBUG("Receiving: %s -- Actor %s on mailbox %s", NAME.c_str(), simgrid::s4u::this_actor::get_cname(),
              from->get_cname());
    from->get_unique<std::string>();
    log_action(action, simgrid::s4u::Engine::get_clock() - clock);
  }
};

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file [action_files]\n"
             "\t# if all actions are in the same file\n"
             "\tExample: %s platform.xml deployment.xml actions\n"
             "\t# if actions are in separate files, specified in deployment\n"
             "\tExample: %s platform.xml deployment.xml ",
             argv[0], argv[0], argv[0]);

  e.load_platform(argv[1]);
  e.register_actor<Replayer>("p0");
  e.register_actor<Replayer>("p1");
  e.load_deployment(argv[2]);
  if (argv[3] != nullptr)
    xbt_replay_set_tracefile(argv[3]);

  /*   Action registration */
  xbt_replay_action_register("compute", Replayer::compute);
  xbt_replay_action_register("send", Replayer::send);
  xbt_replay_action_register("recv", Replayer::recv);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
