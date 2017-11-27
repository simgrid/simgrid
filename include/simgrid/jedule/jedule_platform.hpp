/* Copyright (c) 2010-2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JED_SIMGRID_PLATFORM_H_
#define JED_SIMGRID_PLATFORM_H_

#include "simgrid_config.h"
#include "simgrid/forward.h"
#include "xbt/dynar.h"
#include <unordered_map>
#include <vector>
#include <string>
#if SIMGRID_HAVE_JEDULE

namespace simgrid {
namespace jedule{
XBT_PUBLIC_CLASS Container {
public:
  Container(std::string name);
  virtual ~Container();
private:
  int last_id;
  int is_lowest = 0;
public:
  std::string name;
  std::unordered_map<const char*, unsigned int> name2id;
  Container *parent = nullptr;
  std::vector<Container*> children;
  std::vector<sg_host_t> resource_list;
  void addChild(Container* child);
  void addResources(std::vector<sg_host_t> hosts);
  void createHierarchy(sg_netzone_t from_as);
  std::vector<int> getHierarchy();
  std::string getHierarchyAsString();
  void print(FILE *file);
  void printResources(FILE *file);
};

XBT_PUBLIC_CLASS Subset {
public:
  Subset(int s, int n, Container* p);
  virtual ~Subset()=default;
  int start_idx; // start idx in resource_list of container
  int nres;      // number of resources spanning starting at start_idx
  Container *parent;
};

}
}
extern "C" {
typedef simgrid::jedule::Container * jed_container_t;
typedef simgrid::jedule::Subset * jed_subset_t;
void get_resource_selection_by_hosts(std::vector<jed_subset_t>* subset_list, std::vector<sg_host_t> *host_list);
}

#endif

#endif /* JED_SIMGRID_PLATFORM_H_ */
