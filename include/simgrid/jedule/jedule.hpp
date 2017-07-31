/* Copyright (c) 2010-2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_HPP_
#define JEDULE_HPP_
#include "simgrid_config.h"
#include <cstdio>

#include "jedule_events.hpp"
#include "jedule_platform.hpp"

#if SIMGRID_HAVE_JEDULE

XBT_ATTRIB_UNUSED static std::unordered_map <const char *, jed_container_t> host2_simgrid_parent_container;
XBT_ATTRIB_UNUSED static std::unordered_map <std::string, jed_container_t> container_name2container;

namespace simgrid {
namespace jedule{


XBT_PUBLIC_CLASS Jedule {
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

SG_BEGIN_DECL()

typedef simgrid::jedule::Jedule *jedule_t;

SG_END_DECL()
#endif

#endif /* JEDULE_HPP_ */
