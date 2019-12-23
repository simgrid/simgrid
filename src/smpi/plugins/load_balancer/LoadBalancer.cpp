/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <map>
#include <unordered_map>
#include <queue>

#include <boost/heap/fibonacci_heap.hpp>
#include <simgrid/plugins/load.h>
#include <src/smpi/plugins/load_balancer/load_balancer.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(plugin_load_balancer);

namespace simgrid {
namespace plugin {
namespace loadbalancer {

class XBT_PRIVATE compare_hosts {
public:
  bool operator()(simgrid::s4u::Host* const a, simgrid::s4u::Host* const b) const;
};

typedef boost::heap::fibonacci_heap<simgrid::s4u::Host*, boost::heap::compare<compare_hosts>>::handle_type heap_handle;

/**
 * Structure that imitates a std::pair, but it allows us
 * to use meaningful names instead of .first and .second
 */
struct XBT_PRIVATE pair_handle_load
{
  heap_handle update_handle;
  double load;
};

static std::map<simgrid::s4u::Host* const, pair_handle_load> additional_load;

bool compare_hosts::operator()(simgrid::s4u::Host* const a, simgrid::s4u::Host* const b) const {
  return additional_load[a].load > additional_load[b].load;
}


void LoadBalancer::run()
{
  simgrid::s4u::Engine* engine                     = simgrid::s4u::Engine::get_instance();
  std::vector<simgrid::s4u::Host*> available_hosts =
      engine->get_filtered_hosts([](simgrid::s4u::Host* host) { return host->is_on(); });
  xbt_assert(available_hosts.size() > 0, "No hosts available; are they all switched off?");

  // TODO: Account for daemon background load (-> use especially the availability file)

  std::vector<simgrid::s4u::ActorPtr> all_actors =
      engine->get_filtered_actors([](simgrid::s4u::ActorPtr actor) { return not actor->is_daemon(); });

  for (auto const& actor : all_actors) {
    new_mapping.assign(actor, actor->get_host());
  }
  // Sort the actors, from highest to lowest load; we then just iterate over these actors
  std::sort(all_actors.begin(), all_actors.end(), [this](simgrid::s4u::ActorPtr a, simgrid::s4u::ActorPtr b) {
    return actor_computation[a->get_pid()] > actor_computation[b->get_pid()];
  });

  // Sort the hosts. Use a heap datastructure, because we have to reorder
  // after a host got another actor assigned (or moved from).
  // We can't use std::priorityQueue here because we modify *two* elements: The top element, which
  // we can access and which has the lowest load, gets a new actor assigned. 
  // However, the host loosing that actor must be updated as well. 
  // std::priorityQueue is immutable and hence doesn't work for us.
  //
  // This heap contains the least loaded host at the top
  boost::heap::fibonacci_heap<simgrid::s4u::Host*, boost::heap::compare<compare_hosts>> usable_hosts;
  for (auto& host : available_hosts) {
    std::vector<simgrid::s4u::ActorPtr> actors = host->get_all_actors();
    heap_handle update_handle                  = usable_hosts.push(host); // Required to update elements in the heap
    additional_load[host]                      = {update_handle, 0};      // Save the handle for later
    const double total_flops_computed          = sg_host_get_computed_flops(host);
    for (auto const& actor : actors) {
      additional_load[host].load += actor_computation[actor->get_pid()] / total_flops_computed; // Normalize load - this allows comparison
                                                                                                // even between hosts with different frequencies
      XBT_DEBUG("Actor %li -> %f", actor->get_pid(), actor_computation[actor->get_pid()]);
    }
    usable_hosts.increase(update_handle);
    XBT_DEBUG("Host %s initialized to %f", host->get_cname(), additional_load[host].load);
  }

  // Implementation of the Greedy algorithm
  for (auto const& actor : all_actors) {
    simgrid::s4u::Host* target_host = usable_hosts.top(); // This is the host with the lowest load

    simgrid::s4u::Host* cur_mapped_host = new_mapping.get_host(actor);
    if (target_host != cur_mapped_host
        && additional_load[target_host].load + actor_computation[actor->get_pid()] < additional_load[cur_mapped_host].load
        && new_mapping.count_actors(cur_mapped_host) > 1) {
      usable_hosts.pop();
      XBT_DEBUG("Assigning %li from %s to %s -- actor_load: %f -- host_load: %f", actor->get_pid(), actor->get_host()->get_cname(), target_host->get_cname(), actor_computation[actor->get_pid()], additional_load[target_host].load);
      additional_load[cur_mapped_host].load = std::max<double>(0.0, additional_load[cur_mapped_host].load - actor_computation[actor->get_pid()]); // No negative loads, please!
      usable_hosts.update(additional_load[cur_mapped_host].update_handle, cur_mapped_host);
      additional_load[target_host].load         += actor_computation[actor->get_pid()];

      new_mapping.assign(actor, target_host);

      XBT_DEBUG("Assigning actor %li to host %s", actor->get_pid(), target_host->get_cname());

      XBT_DEBUG("host_load: %f after the assignment", additional_load[target_host].load);
      additional_load[target_host].update_handle = usable_hosts.push(target_host); // Save update handle for later
    }
  }

  while (!usable_hosts.empty()) {
    simgrid::s4u::Host* host = usable_hosts.top();
    usable_hosts.pop();

    sg_host_load_reset(host); // Reset host load for next iterations

    if (XBT_LOG_ISENABLED(plugin_load_balancer, e_xbt_log_priority_t::xbt_log_priority_debug)) {
      /* Debug messages that allow us to verify the load for each host */
      XBT_DEBUG("Host: %s, load total: %f", host->get_cname(), additional_load[host].load);
      double load_verif = 0.0;
      new_mapping.for_each_actor(host,
          [this, &load_verif](simgrid::s4u::ActorPtr actor) {
            load_verif += actor_computation[actor->get_pid()];
            XBT_DEBUG("        %li (load: %f)", actor->get_pid(), actor_computation[actor->get_pid()]);
      });
      XBT_DEBUG("Host load verification: %f", load_verif);
    }
  }
  for (auto& elem : actor_computation) { // Reset actor load
    elem.second = 0;
  }
}

simgrid::s4u::Host* LoadBalancer::get_mapping(simgrid::s4u::ActorPtr actor)
{
  return new_mapping.get_host(actor);
}

void LoadBalancer::record_actor_computation(simgrid::s4u::Actor const& actor, double load)
{
  actor_computation[actor.get_pid()] += load;
}
}
}
}
