/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
#include <xbt/replay.hpp>
#include <xbt/str.h>

#include <boost/algorithm/string/join.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(replay_io, "Messages specific for this example");

#define ACT_DEBUG(...)                                                                                                 \
  if (XBT_LOG_ISENABLED(replay_io, xbt_log_priority_verbose)) {                                                        \
    std::string NAME = boost::algorithm::join(action, " ");                                                            \
    XBT_DEBUG(__VA_ARGS__);                                                                                            \
  } else                                                                                                               \
    ((void)0)

class Replayer {
  static std::unordered_map<std::string, simgrid::s4u::File> opened_files;

  static void log_action(const simgrid::xbt::ReplayAction& action, double date)
  {
    if (XBT_LOG_ISENABLED(replay_io, xbt_log_priority_verbose)) {
      std::string s = boost::algorithm::join(action, " ");
      XBT_VERB("%s %f", s.c_str(), date);
    }
  }

  static simgrid::s4u::File* get_file_descriptor(const std::string& file_name)
  {
    std::string full_name = simgrid::s4u::this_actor::get_name() + ":" + file_name;
    return &opened_files.at(full_name);
  }

public:
  explicit Replayer(std::vector<std::string> args)
  {
    const char* actor_name = args[0].c_str();
    simgrid::xbt::replay_runner(actor_name, nullptr);
  }

  void operator()() const
  {
    // Nothing to do here
  }

  /* My actions */
  static void open(simgrid::xbt::ReplayAction& action)
  {
    std::string file_name = action[2];
    double clock          = simgrid::s4u::Engine::get_clock();
    std::string full_name = simgrid::s4u::this_actor::get_name() + ":" + file_name;

    ACT_DEBUG("Entering Open: %s (filename: %s)", NAME.c_str(), file_name.c_str());
    opened_files.emplace(std::piecewise_construct, std::forward_as_tuple(full_name),
                         std::forward_as_tuple(file_name, nullptr));

    log_action(action, simgrid::s4u::Engine::get_clock() - clock);
  }

  static void read(simgrid::xbt::ReplayAction& action)
  {
    std::string file_name = action[2];
    sg_size_t size        = std::stoul(action[3]);
    double clock          = simgrid::s4u::Engine::get_clock();

    simgrid::s4u::File* file = get_file_descriptor(file_name);

    ACT_DEBUG("Entering Read: %s (size: %llu)", NAME.c_str(), size);
    file->read(size);

    log_action(action, simgrid::s4u::Engine::get_clock() - clock);
  }

  static void close(simgrid::xbt::ReplayAction& action)
  {
    std::string file_name = action[2];
    std::string full_name = simgrid::s4u::this_actor::get_name() + ":" + file_name;
    double clock          = simgrid::s4u::Engine::get_clock();

    ACT_DEBUG("Entering Close: %s (filename: %s)", NAME.c_str(), file_name.c_str());
    XBT_ATTRIB_UNUSED auto count = opened_files.erase(full_name);
    xbt_assert(count == 1, "File not found in opened files: %s", full_name.c_str());

    log_action(action, simgrid::s4u::Engine::get_clock() - clock);
  }
};

std::unordered_map<std::string, simgrid::s4u::File> Replayer::opened_files;

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();

  xbt_assert(argc > 3,
             "Usage: %s platform_file deployment_file [action_files]\n"
             "\texample: %s platform.xml deployment.xml actions # if all actions are in the same file\n"
             "\t# if actions are in separate files, specified in deployment\n"
             "\texample: %s platform.xml deployment.xml",
             argv[0], argv[0], argv[0]);

  e.load_platform(argv[1]);
  e.register_actor<Replayer>("p0");
  e.load_deployment(argv[2]);

  /*   Action registration */
  xbt_replay_action_register("open", Replayer::open);
  xbt_replay_action_register("read", Replayer::read);
  xbt_replay_action_register("close", Replayer::close);

  std::ifstream ifs;
  if (argv[3]) {
    ifs.open(argv[3], std::ifstream::in);
    simgrid::xbt::action_fs = &ifs;
  }

  e.run();

  simgrid::xbt::action_fs = nullptr;

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
