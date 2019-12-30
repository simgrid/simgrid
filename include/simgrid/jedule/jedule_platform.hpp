/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JED_SIMGRID_PLATFORM_H_
#define JED_SIMGRID_PLATFORM_H_

#include <simgrid/forward.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid {
namespace jedule{
class XBT_PUBLIC Container {
public:
  explicit Container(const std::string& name);
  Container(const Container&) = delete;
  Container& operator=(const Container&) = delete;

private:
  int last_id_   = 0;

public:
  std::string name;
  std::unordered_map<const char*, unsigned int> name2id;
  Container *parent = nullptr;
  std::vector<std::unique_ptr<Container>> children;
  std::vector<sg_host_t> resource_list;
  void add_child(Container* child);
  void add_resources(std::vector<sg_host_t> hosts);
  void create_hierarchy(const_sg_netzone_t from_as);
  std::vector<int> get_hierarchy();
  std::string get_hierarchy_as_string();
  void print(FILE *file);
  void print_resources(FILE* file);
};

class XBT_PUBLIC Subset {
public:
  Subset(int s, int n, Container* p);
  int start_idx; // start idx in resource_list of container
  int nres;      // number of resources spanning starting at start_idx
  Container *parent;
};

}
}
typedef simgrid::jedule::Container * jed_container_t;
void get_resource_selection_by_hosts(std::vector<simgrid::jedule::Subset>& subset_list,
                                     const std::vector<sg_host_t>& host_list);

#endif /* JED_SIMGRID_PLATFORM_H_ */
