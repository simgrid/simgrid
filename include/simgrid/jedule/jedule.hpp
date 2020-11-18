/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_HPP_
#define JEDULE_HPP_

#include <simgrid/jedule/jedule_events.hpp>
#include <simgrid/jedule/jedule_platform.hpp>
#include <simgrid/s4u/Engine.hpp>

#include <cstdio>

namespace simgrid {
namespace jedule{

class XBT_PUBLIC Jedule {
  std::unordered_map<char*, char*> meta_info_;
  std::vector<Event> event_set_;
  Container root_container_;

public:
  explicit Jedule(const std::string& name) : root_container_(name)
  {
    root_container_.create_hierarchy(s4u::Engine::get_instance()->get_netzone_root());
  }
  void add_meta_info(char* key, char* value);
  void add_event(const Event& event);
  void cleanup_output();
  void write_output(FILE* file);
};

} // namespace jedule
} // namespace simgrid

using jedule_t = simgrid::jedule::Jedule*;

#endif /* JEDULE_HPP_ */
