/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

/* This example shows how to use simgrid::s4u::Engine::get_filtered_hosts() to retrieve
 * all hosts that match a given criteria. This criteria can be specified either with:
 *   - an inlined callback
 *   - a boolean function, such as filter_speed_more_than_50Mf() below
 *   - a functor (= function object), that can either be stateless such as filter::SingleCore below, or that can save
 *     state such as filter::FrequencyChanged below
 *
 * This file provides examples for each of these categories. You should implement your own filters in your code.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_engine_filtering, "Messages specific for this s4u example");

namespace filter {
/* First example of thing that we can use as a filtering criteria: a simple boolean function */
static bool filter_speed_more_than_50Mf(simgrid::s4u::Host* host)
{
  return host->get_speed() > 50E6;
}

/* Second kind of thing that we can use as a filtering criteria: a functor (=function object).
 * This one is a bit stupid: it's a lot of boilerplate over a dummy boolean function.
 */
class SingleCore {
public:
  bool operator()(simgrid::s4u::Host* host) { return host->get_core_count() == 1; }
};

/* This functor is a bit more complex, as it saves the current state when created.
 * Then, it allows one to easily retrieve the hosts which frequency changed since the functor creation.
 */
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
}
int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  /* Use a lambda function to filter hosts: We only want multicore hosts */
  XBT_INFO("Hosts currently registered with this engine: %zu", e.get_host_count());
  std::vector<simgrid::s4u::Host*> list =
      e.get_filtered_hosts([](simgrid::s4u::Host* host) { return host->get_core_count() > 1; });

  for (auto& host : list)
    XBT_INFO("The following hosts have more than one core: %s", host->get_cname());

  xbt_assert(list.size() == 1);

  /* Use a function object (functor) without memory */
  list = e.get_filtered_hosts(filter::SingleCore());

  for (auto& host : list)
    XBT_INFO("The following hosts are SingleCore: %s", host->get_cname());

  /* Use a function object that uses memory to filter */
  XBT_INFO("A simple example: Let's retrieve all hosts that changed their frequency");
  filter::FrequencyChanged filter(e);
  e.host_by_name("MyHost2")->set_pstate(2);
  list = e.get_filtered_hosts(filter);

  for (auto& host : list)
    XBT_INFO("The following hosts changed their frequency: %s (from %.1ff to %.1ff)", host->get_cname(),
             host->get_pstate_speed(filter.get_old_speed(host)), host->get_speed());

  /* You can also just use any regular function (namespaced on need) to filter  */
  list = e.get_filtered_hosts(filter::filter_speed_more_than_50Mf);

  for (auto& host : list)
    XBT_INFO("The following hosts have a frequency > 50Mf: %s", host->get_cname());

  return 0;
}
