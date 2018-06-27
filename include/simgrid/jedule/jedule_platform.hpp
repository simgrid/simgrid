/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JED_SIMGRID_PLATFORM_H_
#define JED_SIMGRID_PLATFORM_H_

#include <simgrid/forward.h>
#include <xbt/dynar.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace simgrid {
namespace jedule{
class XBT_PUBLIC Container {
public:
  explicit Container(std::string name);
  virtual ~Container();
private:
  int last_id_;
  int is_lowest_ = 0;

public:
  std::string name;
  std::unordered_map<const char*, unsigned int> name2id;
  Container *parent = nullptr;
  std::vector<Container*> children;
  std::vector<sg_host_t> resource_list;
  void add_child(Container* child);
  void add_resources(std::vector<sg_host_t> hosts);
  void create_hierarchy(sg_netzone_t from_as);
  std::vector<int> get_hierarchy();
  std::string get_hierarchy_as_string();
  void print(FILE *file);
  void print_resources(FILE* file);

  // deprecated
  XBT_ATTRIB_DEPRECATED_v323("Please use Container::add_child()") void addChild(Container* child) { add_child(child); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Container::add_resources()") void addResources(std::vector<sg_host_t> hosts)
  {
    add_resources(hosts);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Container::create_hierarchy()") void createHierarchy(sg_netzone_t from_as)
  {
    create_hierarchy(from_as);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Container::get_hierarchy()") std::vector<int> getHierarchy()
  {
    return get_hierarchy();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Container::get_hierarchy_as_string()") std::string getHierarchyAsString()
  {
    return get_hierarchy_as_string();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Container::print_resources()") void printResources(FILE* file)
  {
    print_resources(file);
  }
};

class XBT_PUBLIC Subset {
public:
  Subset(int s, int n, Container* p);
  virtual ~Subset()=default;
  int start_idx; // start idx in resource_list of container
  int nres;      // number of resources spanning starting at start_idx
  Container *parent;
};

}
}
typedef simgrid::jedule::Container * jed_container_t;
typedef simgrid::jedule::Subset * jed_subset_t;
void get_resource_selection_by_hosts(std::vector<jed_subset_t>* subset_list, std::vector<sg_host_t> *host_list);

#endif /* JED_SIMGRID_PLATFORM_H_ */
