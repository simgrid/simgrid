/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_engine_filtering, "Messages specific for this s4u example");

namespace filter {
class FrequencyChanged {
  std::map<simgrid::s4u::Host*, int> host_list;

public:
  explicit FrequencyChanged(simgrid::s4u::Engine& e)
  {
    std::vector<simgrid::s4u::Host*> list = e.get_all_hosts();
    for (auto& host : list) {
      host_list.insert({host, host->get_pstate()});
    }
  }

  bool operator()(simgrid::s4u::Host* host) { return host->get_pstate() != host_list.at(host); }

  double get_old_speed(simgrid::s4u::Host* host) { return host_list.at(host); }
};
class SingleCore {
public:
  bool operator()(simgrid::s4u::Host* host) { return host->get_core_count() == 1; }
};

bool filter_speed_more_than_50Mf(simgrid::s4u::Host* host);
bool filter_speed_more_than_50Mf(simgrid::s4u::Host* host)
{
  return host->getSpeed() > 50E6;
}
}
int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  /**
   * Use a lambda function to filter hosts: We only want multicore hosts
   */
  XBT_INFO("Hosts currently registered with this engine: %zu", e.get_host_count());
  std::vector<simgrid::s4u::Host*> list =
      e.get_filtered_hosts([](simgrid::s4u::Host* host) { return host->get_core_count() > 1; });

  for (auto& host : list)
    XBT_INFO("The following hosts have more than one core: %s", host->get_cname());

  xbt_assert(list.size() == 1);

  /*
   * Use a function object (functor) without memory
   */
  list = e.get_filtered_hosts(filter::SingleCore());

  for (auto& host : list)
    XBT_INFO("The following hosts are SingleCore: %s", host->get_cname());

  /**
   * This just shows how to use a function object that uses
   * memory to filter.
   */
  XBT_INFO("A simple example: Let's retrieve all hosts that changed their frequency");
  filter::FrequencyChanged filter(e);
  e.host_by_name("MyHost2")->set_pstate(2);
  list = e.get_filtered_hosts(filter);

  for (auto& host : list)
    XBT_INFO("The following hosts changed their frequency: %s (from %.1ff to %.1ff)", host->get_cname(), host->getPstateSpeed(filter.get_old_speed(host)), host->getSpeed());

  /*
   * You can also just use any normal function (namespaced as well, if you want) to filter
   */
  list = e.get_filtered_hosts(filter::filter_speed_more_than_50Mf);

  for (auto& host : list)
    XBT_INFO("The following hosts have a frequency > 50Mf: %s", host->get_cname());

  return 0;
}
