/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_HPP_
#define JEDULE_HPP_

#include <simgrid/jedule/jedule_events.hpp>
#include <simgrid/jedule/jedule_platform.hpp>
#include <simgrid_config.h>

#include <cstdio>

#if SIMGRID_HAVE_JEDULE

namespace simgrid {
namespace jedule{

class XBT_PUBLIC Jedule {
public:
  Jedule()=default;
  ~Jedule();
  std::vector<Event *> event_set;
  Container* root_container = nullptr;
  std::unordered_map<char*, char*> meta_info;
  void addMetaInfo(char* key, char* value);
  void cleanupOutput();
  void writeOutput(FILE *file);
};

}
}

extern "C" {
typedef simgrid::jedule::Jedule *jedule_t;
}
#endif

#endif /* JEDULE_HPP_ */
