/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_HPP_
#define JEDULE_HPP_

#include <simgrid/jedule/jedule_events.hpp>
#include <simgrid/jedule/jedule_platform.hpp>

#include <cstdio>

namespace simgrid {
namespace jedule{

class XBT_PUBLIC Jedule {
public:
  explicit Jedule(const std::string& name) : root_container_(name) {}
  std::vector<Event> event_set_;
  Container root_container_;
  void add_meta_info(char* key, char* value);
  void cleanup_output();
  void write_output(FILE* file);

private:
  std::unordered_map<char*, char*> meta_info_;
};

}
}

typedef simgrid::jedule::Jedule *jedule_t;

#endif /* JEDULE_HPP_ */
