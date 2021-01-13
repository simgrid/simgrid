/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

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
  int last_id_ = 0;
  std::string name;
  std::unordered_map<const char*, unsigned int> name2id;
  Container* parent_ = nullptr;
  std::vector<std::unique_ptr<Container>> children_;
  std::vector<sg_host_t> resource_list;

public:
  explicit Container(const std::string& name);
  Container(const Container&) = delete;
  Container& operator=(const Container&) = delete;

  const char* get_cname() const { return name.c_str(); }
  void set_parent(Container* parent) { parent_ = parent; }
  bool has_children() const { return not children_.empty(); }
  int get_child_position(const Container* child) const;
  unsigned int get_id_by_name(const char* name) const { return name2id.at(name); }

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

} // namespace jedule
} // namespace simgrid
using jed_container_t = simgrid::jedule::Container*;
void get_resource_selection_by_hosts(std::vector<simgrid::jedule::Subset>& subset_list,
                                     const std::vector<sg_host_t>& host_list);

#endif /* JED_SIMGRID_PLATFORM_H_ */
