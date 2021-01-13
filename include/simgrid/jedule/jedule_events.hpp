/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

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
  Event(const std::string& name, double start_time, double end_time, const std::string& type)
      : name_(name), start_time_(start_time), end_time_(end_time), type_(type)
  {
  }
  void add_characteristic(const char* characteristic);
  void add_resources(const std::vector<sg_host_t>& host_selection);
  void add_info(char* key, char* value);
  void print(FILE* file) const;

private:
  std::string name_;
  double start_time_;
  double end_time_;
  std::string type_;
  std::vector<Subset> resource_subsets_;
  std::vector<std::string> characteristics_list_;         /* just a list of names */
  std::unordered_map<std::string, std::string> info_map_; /* key/value pairs */
};
}
}

using jed_event_t = simgrid::jedule::Event*;

#endif /* JEDULE_EVENTS_H_ */
