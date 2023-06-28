/* Copyright (c) 2022-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// Chaos Monkey plugin: See the simgrid-monkey script for more information

#include <simgrid/kernel/Timer.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <xbt/config.hpp>

#include "src/simgrid/module.hpp" // SIMGRID_REGISTER_PLUGIN

namespace sg4 = simgrid::s4u;
static simgrid::config::Flag<bool> cfg_tell{"cmonkey/tell", "Request the Chaos Monkey to display all timestamps",
                                            false};
static simgrid::config::Flag<double> cfg_time{"cmonkey/time", "When should the chaos monkey kill a resource", -1.};
static simgrid::config::Flag<int> cfg_link{"cmonkey/link", "Which link should be killed (number)", -1};
static simgrid::config::Flag<int> cfg_host{"cmonkey/host", "Which host should be killed (number)", -1};

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cmonkey, kernel, "Chaos Monkey plugin");

static void sg_chaos_monkey_plugin_run()
{
  const auto* engine = sg4::Engine::get_instance();
  auto hosts  = engine->get_all_hosts();
  auto links  = engine->get_all_links();

  sg4::Engine::on_deadlock_cb([]() { exit(2); });

  if (not cfg_tell && cfg_time < 0 && cfg_host == -1 && cfg_link == -1) {
    XBT_CRITICAL(
        "You invoked the Chaos Monkey without anything to do.\n"
        "Bored, it kills your simulation after this message about how to use it manually (note that the\n"
        "simgrid-monkey script can tame the monkey for you if you prefer).\n\n"
        "First of all, you need information. Use --cfg=cmonkey/tell:1 to get all information about the existing\n"
        "resources and the timestamps of interest in your simulation.\n\n"
        "Then, use --cfg=cmonkey/host:0 --cfg=cmonkey/time:0.1 to turn off and on the host #0 at time 0.1s,\n"
        "or --cfg=cmonkey/link:22 --cfg=cmonkey/time:0.4 to turn the link #22 off and on again at time 0.4s.\n"
        "Only one resource can be rebooted in a given run.\n\n"
        "Please read the comments at the beginning of the simgrid-monkey script about how to exhaustively test a\n"
        "program resilience.\n");
    exit(1);
  }

  if (cfg_tell) {
    XBT_INFO("HOST_COUNT=%zu", hosts.size());
    XBT_INFO("LINK_COUNT=%zu", links.size());
    sg4::Engine::on_time_advance_cb([engine](double /* delta*/) { XBT_INFO("TIMESTAMP=%lf", engine->get_clock()); });
  }

  if (cfg_time >= 0) {
    int host = cfg_host;
    int link = cfg_link;
    xbt_assert(host >= 0 || link >= 0,
               "If a kill time is given, you must also specify a resource to kill (either a link or an host)");
    xbt_assert(host < 0 || link < 0, "Cannot specify both a link and an host to kill");
    if (host >= 0) {
      auto* h = hosts.at(host);
      simgrid::kernel::timer::Timer::set(cfg_time, [h]() {
        XBT_INFO("Kill host %s", h->get_cname());
        h->turn_off();
      });
      simgrid::kernel::timer::Timer::set(cfg_time + 30, [h]() {
        XBT_INFO("Restart host %s", h->get_cname());
        h->turn_on();
      });
    }
    if (link >= 0) {
      auto* l = links.at(link);
      simgrid::kernel::timer::Timer::set(cfg_time, [l]() {
        XBT_INFO("Kill link %s", l->get_cname());
        l->turn_off();
      });
      simgrid::kernel::timer::Timer::set(cfg_time + 30, [l]() {
        XBT_INFO("Restart host %s", l->get_cname());
        l->turn_on();
      });
    }
  }

  sg4::Engine::on_simulation_end_cb([]() { XBT_INFO("Chaos Monkey done!"); });
}

// Makes sure that this plugin can be activated from the command line with ``--cfg=plugin:cmonkey``
SIMGRID_REGISTER_PLUGIN(cmonkey, "Chaos monkey", []() {
  XBT_INFO("Initializing the chaos monkey.");

  // delay the initialization until after the parameter are parsed
  sg4::Engine::on_simulation_start_cb(sg_chaos_monkey_plugin_run);
})
