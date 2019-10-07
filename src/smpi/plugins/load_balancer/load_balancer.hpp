/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

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
  typedef std::unordered_multimap<simgrid::s4u::Host*, simgrid::s4u::ActorPtr> host_to_actors_map_t;
  host_to_actors_map_t host_to_actors;

  /** Each actor gets assigned to exactly one host -> map **/
  std::map<simgrid::s4u::ActorPtr, simgrid::s4u::Host*> actor_to_host;

  void assign(simgrid::s4u::ActorPtr actor, simgrid::s4u::Host* host)
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

  simgrid::s4u::Host* get_host(simgrid::s4u::ActorPtr actor) { return actor_to_host[actor]; }

  unsigned int count_actors(simgrid::s4u::Host* host)
  {
    return host_to_actors.count(host); // TODO This is linear in the size of the map. Maybe replace by constant lookup through another map?
  }

  void for_each_actor(simgrid::s4u::Host* host, const std::function<void(simgrid::s4u::ActorPtr)>& callback)
  {
    auto range = host_to_actors.equal_range(host);
    std::for_each(
        range.first,
        range.second,
        [&callback](host_to_actors_map_t::value_type& x) { callback(x.second); }
    );
  }
};

class XBT_PRIVATE LoadBalancer
{
  Mapping new_mapping;
  std::map</*proc id*/int, double> actor_computation;

public:
  void run();
  void assign(simgrid::s4u::ActorPtr actor, simgrid::s4u::Host* host);
  
  /**
   * FIXME These are functions used for testing and should be re-written or removed
   */
  simgrid::s4u::Host* get_mapping(simgrid::s4u::ActorPtr);
  void record_actor_computation(simgrid::s4u::Actor const& actor, double load);
};

}
}
}
#endif
