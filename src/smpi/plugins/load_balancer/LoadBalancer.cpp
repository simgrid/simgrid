/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <queue>
#include <simgrid/plugins/load.h>
#include <simgrid/s4u.hpp>
#include <simgrid/smpi/loadbalancer/load_balancer.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(plugin_load_balancer_impl, smpi, "Logging specific to the SMPI load balancing plugin");

namespace simgrid {
namespace plugin {
namespace loadbalancer {

static std::map<simgrid::s4u::Host*, double> additional_load;

static bool compare_by_avg_load(simgrid::s4u::Host* a, simgrid::s4u::Host* b)
{
  return sg_host_get_avg_load(a) + additional_load[a] > sg_host_get_avg_load(b) + additional_load[b];
}

LoadBalancer::LoadBalancer()
{
}

LoadBalancer::~LoadBalancer()
{
}

void LoadBalancer::run()
{
  simgrid::s4u::Engine* engine                     = simgrid::s4u::Engine::get_instance();
  std::vector<simgrid::s4u::Host*> available_hosts = engine->get_filtered_hosts([](simgrid::s4u::Host* host) {
    return not host->is_off();
  });
  xbt_assert(available_hosts.size() > 0, "No hosts available; are they all switched off?");
  for (auto& host : available_hosts) {
    additional_load[host] = 0;
  }

  // TODO: Account for daemon background load (-> use especially the availability file)

  std::vector<simgrid::s4u::ActorPtr> all_actors =
      engine->get_filtered_actors([](simgrid::s4u::ActorPtr actor) { return not actor->is_daemon(); });
  for (auto& actor : all_actors) {
    new_mapping[actor] = actor->get_host();
  }
  // Sort the actors, from highest to lowest load; we then just iterate over these actors
  std::sort(all_actors.begin(), all_actors.end(), [this](simgrid::s4u::ActorPtr a, simgrid::s4u::ActorPtr b) {
    return actor_computation[a->get_pid()] < actor_computation[b->get_pid()];
  });
  for (auto& actor : all_actors) {
    XBT_INFO("Order: %li", actor->get_pid());
  }
  // Sort the hosts. Use a heap datastructure, because we re-insert a host
  // that got an actor assigned in a possibly different position
  // This is equivalent to a std::greater<Host*> implementation that uses the load to compare the two objects.
  // --> The least loaded host will be in the first position
  std::priority_queue</*datatype*/ simgrid::s4u::Host*, /*container*/ std::vector<simgrid::s4u::Host*>,
                      /*comparison function*/ std::function<bool(simgrid::s4u::Host*, simgrid::s4u::Host*)>>
      usable_hosts(compare_by_avg_load, std::move(available_hosts));

  for (auto& actor : all_actors) {
    simgrid::s4u::Host* target_host = usable_hosts.top(); // This is the host with the lowest load

    if (target_host != actor->get_host() && actor->get_host()->get_actor_count() > 1) {
      usable_hosts.pop();

      assign(actor, target_host); // This updates also the load
      usable_hosts.push(target_host);
    }
  }
}

void LoadBalancer::assign(simgrid::s4u::ActorPtr actor, simgrid::s4u::Host* host)
{
  // If an actor gets re-assigned twice, we don't want to subtract
  // the load from the same host twice so we have to use the old value of new_mapping here
  additional_load[new_mapping[actor]] -= 1; // actor->load();
  additional_load[host] += 1;               // actor->load();
  new_mapping[actor] = host;

  XBT_INFO("Assigning actor %li to host %s", actor->get_pid(), host->get_cname());
}

simgrid::s4u::Host* LoadBalancer::get_mapping()
{
  return new_mapping[simgrid::s4u::Actor::self()];
}

void LoadBalancer::record_actor_computation(simgrid::s4u::ActorPtr actor, double load)
{
  actor_computation[actor->get_pid()] += load;
}
}
}
}
