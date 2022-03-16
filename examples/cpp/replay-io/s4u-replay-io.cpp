/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
#include <xbt/replay.hpp>
#include <xbt/str.h>

#include <boost/algorithm/string/join.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(replay_io, "Messages specific for this example");
namespace sg4 = simgrid::s4u;

#define ACT_DEBUG(...)                                                                                                 \
  if (XBT_LOG_ISENABLED(replay_io, xbt_log_priority_verbose)) {                                                        \
    std::string NAME = boost::algorithm::join(action, " ");                                                            \
    XBT_DEBUG(__VA_ARGS__);                                                                                            \
  } else                                                                                                               \
    ((void)0)

class Replayer {
  static std::unordered_map<std::string, sg4::File*> opened_files;

  static void log_action(const simgrid::xbt::ReplayAction& action, double date)
  {
    if (XBT_LOG_ISENABLED(replay_io, xbt_log_priority_verbose)) {
      std::string s = boost::algorithm::join(action, " ");
      XBT_VERB("%s %f", s.c_str(), date);
    }
  }

  static sg4::File* get_file_descriptor(const std::string& file_name)
  {
    std::string full_name = sg4::this_actor::get_name() + ":" + file_name;
    return opened_files.at(full_name);
  }

public:
  explicit Replayer(std::vector<std::string> args)
  {
    const char* actor_name = args[0].c_str();
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
  static void open(simgrid::xbt::ReplayAction& action)
  {
    std::string file_name = action[2];
    double clock          = sg4::Engine::get_clock();
    std::string full_name = sg4::this_actor::get_name() + ":" + file_name;

    ACT_DEBUG("Entering Open: %s (filename: %s)", NAME.c_str(), file_name.c_str());
    auto* file = sg4::File::open(file_name, nullptr);
    opened_files.emplace(full_name, file);

    log_action(action, sg4::Engine::get_clock() - clock);
  }

  static void read(simgrid::xbt::ReplayAction& action)
  {
    std::string file_name = action[2];
    sg_size_t size        = std::stoul(action[3]);
    double clock          = sg4::Engine::get_clock();

    sg4::File* file = get_file_descriptor(file_name);

    ACT_DEBUG("Entering Read: %s (size: %llu)", NAME.c_str(), size);
    file->read(size);

    log_action(action, sg4::Engine::get_clock() - clock);
  }

  static void close(simgrid::xbt::ReplayAction& action)
  {
    std::string file_name = action[2];
    std::string full_name = sg4::this_actor::get_name() + ":" + file_name;
    double clock          = sg4::Engine::get_clock();

    ACT_DEBUG("Entering Close: %s (filename: %s)", NAME.c_str(), file_name.c_str());
    auto entry = opened_files.find(full_name);
    xbt_assert(entry != opened_files.end(), "File not found in opened files: %s", full_name.c_str());
    entry->second->close();
    opened_files.erase(entry);
    log_action(action, sg4::Engine::get_clock() - clock);
  }
};

std::unordered_map<std::string, sg4::File*> Replayer::opened_files;

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
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

  if (argv[3] != nullptr)
    xbt_replay_set_tracefile(argv[3]);

  /*   Action registration */
  xbt_replay_action_register("open", Replayer::open);
  xbt_replay_action_register("read", Replayer::read);
  xbt_replay_action_register("close", Replayer::close);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
