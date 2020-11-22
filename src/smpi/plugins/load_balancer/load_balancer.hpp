/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef HAVE_SG_PLUGIN_LB
#define HAVE_SG_PLUGIN_LB

#include <simgrid/s4u.hpp>

namespace simgrid {
namespace plugin {
namespace loadbalancer {

class XBT_PRIVATE Mapping {
public:
  /** Each host can have an arbitrary number of actors -> multimap **/
  using host_to_actors_map_t = std::unordered_multimap<s4u::Host*, s4u::ActorPtr>;
  host_to_actors_map_t host_to_actors;

  /** Each actor gets assigned to exactly one host -> map **/
  std::map<s4u::ActorPtr, s4u::Host*> actor_to_host;

  void assign(s4u::ActorPtr actor, s4u::Host* host)
  {
    /* Remove "actor" from its old host -> get all elements that have the current host as key **/
    auto range = host_to_actors.equal_range(/* current host */actor_to_host[actor]);
    for (auto it = range.first; it != range.second; it++) {
      if (it->second == actor) {
        host_to_actors.erase(it); // unassign this actor
        break;
      }
    }

    actor_to_host[actor] = host;
    host_to_actors.insert({host, actor});
  }

  s4u::Host* get_host(s4u::ActorPtr actor) const { return actor_to_host.at(actor); }

  unsigned int count_actors(s4u::Host* host) const
  {
    return host_to_actors.count(host); // TODO This is linear in the size of the map. Maybe replace by constant lookup through another map?
  }

  void for_each_actor(s4u::Host* host, const std::function<void(s4u::ActorPtr)>& callback)
  {
    auto range = host_to_actors.equal_range(host);
    std::for_each(range.first, range.second,
                  [&callback](host_to_actors_map_t::value_type const& x) { callback(x.second); });
  }
};

class XBT_PRIVATE LoadBalancer
{
  Mapping new_mapping;
  std::map</*proc id*/int, double> actor_computation;

public:
  void run();
  void assign(s4u::ActorPtr actor, s4u::Host* host);

  /** FIXME These are functions used for testing and should be re-written or removed */
  s4u::Host* get_mapping(s4u::ActorPtr);
  void record_actor_computation(s4u::Actor const& actor, double load);
};

} // namespace loadbalancer
} // namespace plugin
} // namespace simgrid
#endif
