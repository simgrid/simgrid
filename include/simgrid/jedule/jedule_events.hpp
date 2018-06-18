/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_EVENTS_H_
#define JEDULE_EVENTS_H_

#include <simgrid/jedule/jedule_platform.hpp>

#include <simgrid/forward.h>

#include <vector>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace jedule{

class XBT_PUBLIC Event {
public:
  Event(std::string name, double start_time, double end_time, std::string type);
  ~Event();
  void add_characteristic(char* characteristic);
  void add_resources(std::vector<sg_host_t>* host_selection);
  void add_info(char* key, char* value);
  void print(FILE* file);

  // deprecated
  XBT_ATTRIB_DEPRECATED_v323("Please use Event::add_characteristic()") void addCharacteristic(char* characteristic)
  {
    add_characteristic(characteristic);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Event::add_resources()") void addResources(
      std::vector<sg_host_t>* host_selection)
  {
    add_resources(host_selection);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Event::add_info()") void addInfo(char* key, char* value)
  {
    add_info(key, value);
  }

private:
  std::string name_;
  double start_time_;
  double end_time_;
  std::string type_;
  std::vector<jed_subset_t>* resource_subsets_;
  std::vector<char*> characteristics_list_;   /* just a list of names */
  std::unordered_map<char*, char*> info_map_; /* key/value pairs */
};
}
}

typedef simgrid::jedule::Event* jed_event_t;

#endif /* JEDULE_EVENTS_H_ */
