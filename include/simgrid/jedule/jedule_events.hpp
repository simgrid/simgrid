/* Copyright (c) 2010-2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_EVENTS_H_
#define JEDULE_EVENTS_H_

#include "simgrid_config.h"
#include "simgrid/forward.h"
#include <vector>
#include <string>
#include <unordered_map>

#include "jedule_platform.hpp"

#if SIMGRID_HAVE_JEDULE
namespace simgrid {
namespace jedule{

XBT_PUBLIC_CLASS Event{
  public:
    Event(std::string name, double start_time, double end_time, std::string type);
    ~Event();
    void addCharacteristic(char *characteristic);
    void addResources(std::vector<sg_host_t> *host_selection);
    void addInfo(char *key, char *value);
    void print(FILE *file);

  private:
    std::string name;
    double start_time;
    double end_time;
    std::string type;
    std::vector<jed_subset_t> *resource_subsets;
    std::vector<char*> characteristics_list; /* just a list of names (strings) */
    std::unordered_map<char*, char*> info_map;     /* key/value pairs */
};
}
}

extern "C" {
typedef simgrid::jedule::Event * jed_event_t;
}

#endif

#endif /* JEDULE_EVENTS_H_ */
