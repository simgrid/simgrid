/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule.hpp"
#include "simgrid/jedule/jedule_platform.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "xbt/asserts.h"
#include "xbt/dynar.h"
#include <algorithm>

#if HAVE_JEDULE

namespace simgrid {
namespace jedule {
Subset::Subset(int start_idx, int end_idx, Container* parent)
: start_idx(start_idx), parent(parent)
{
  nres=end_idx-start_idx+1;
}


Container::Container(std::string name): name(name)
{
  container_name2container.insert({this->name, this});
}

Container::~Container()
{
  if(!this->children.empty())
    for (auto child: this->children)
      delete child;
}

void Container::addChild(jed_container_t child)
{
  xbt_assert(this != nullptr);
  xbt_assert(child != nullptr);
  this->children.push_back(child);
  child->parent = this;
}

void Container::addResources(std::vector<sg_host_t> hosts)
{
  this->is_lowest = 1;
  this->children.clear();
  this->last_id = 0;

  //FIXME do we need to sort?: xbt_dynar_sort_strings(host_names);

  for (auto host : hosts) {
    const char *host_name = sg_host_get_name(host);
    this->name2id.insert({host_name, this->last_id});
    (this->last_id)++;
    host2_simgrid_parent_container.insert({host_name, this});
    this->resource_list.push_back(host);
  }
}

void Container::createHierarchy(AS_t from_as)
{
  xbt_dict_cursor_t cursor = nullptr;
  char *key;
  AS_t elem;
  xbt_dict_t routing_sons = from_as->children();

  if (xbt_dict_is_empty(routing_sons)) {
    // I am no AS
    // add hosts to jedule platform
    xbt_dynar_t table = from_as->hosts();
    unsigned int dynar_cursor;
    sg_host_t host;

    std::vector<sg_host_t> hosts;

    xbt_dynar_foreach(table, dynar_cursor, host) {
      hosts.push_back(host);
    }
    this->addResources(hosts);
    xbt_dynar_free(&table);
  } else {
    xbt_dict_foreach(routing_sons, cursor, key, elem) {
      jed_container_t child_container = new simgrid::jedule::Container(std::string(elem->name()));
      this->addChild(child_container);
      child_container->createHierarchy(elem);
    }
  }
}

std::vector<int> Container::getHierarchy()
{
  xbt_assert( this!= nullptr );

  if(this->parent != nullptr ) {

    if(!this->parent->children.empty()) {
      // we are in the last level
      return this->parent->getHierarchy();
    } else {
      unsigned int i =0;
      int child_nb = -1;

      for (auto child : this->parent->children) {
        if( child == this) {
          child_nb = i;
          break;
        }
        i++;
      }

      xbt_assert( child_nb > - 1);
      std::vector<int> heir_list = this->parent->getHierarchy();
      heir_list.insert(heir_list.begin(), child_nb);
      return heir_list;
    }
  } else {
    int top_level = 0;
    std::vector<int> heir_list = {top_level};
    return heir_list;
  }
}

std::string Container::getHierarchyAsString()
{
  std::string output("");

  std::vector<int> heir_list = this->getHierarchy();

  unsigned int length = heir_list.size();
  unsigned int i = 0;
  for (auto id : heir_list) {
    output += std::to_string(id);
    if( i != length-1 ) {
      output += ".";
    }
  }

  return output;
}

void Container::printResources(FILE * jed_file)
{
  unsigned int i=0;
  xbt_assert(!this->resource_list.empty());

  unsigned int res_nb = this->resource_list.size();
  std::string resid = this->getHierarchyAsString();

  fprintf(jed_file, "      <rset id=\"%s\" nb=\"%u\" names=\"", resid.c_str(), res_nb);
  for (auto res: this->resource_list) {
    const char * res_name = sg_host_get_name(res);
    fprintf(jed_file, "%s", res_name);
    if( i != res_nb-1 ) {
      fprintf(jed_file, "|");
    }
    i++;
  }
  fprintf(jed_file, "\" />\n");
}

void Container::print(FILE* jed_file)
{
  xbt_assert( this != nullptr );
  fprintf(jed_file, "    <res name=\"%s\">\n", this->name.c_str());
  if( !this->children.empty()){
    for (auto child: this->children) {
      child->print(jed_file);
    }
  } else {
    this->printResources(jed_file);
  }
  fprintf(jed_file, "    </res>\n");
}

}
}

static void add_subsets_to(std::vector<jed_subset_t> *subset_list, std::vector<const char*> hostgroup, jed_container_t parent)
{
  // get ids for each host
  // sort ids
  // compact ids
  // create subset for each id group

  xbt_assert( parent != nullptr );

  std::vector<unsigned int> id_list;

  for (auto host_name : hostgroup) {
    xbt_assert( host_name != nullptr );
    jed_container_t parent = host2_simgrid_parent_container.at(host_name);
    unsigned int id = parent->name2id.at(host_name);
    id_list.push_back(id);
  }
  unsigned int nb_ids = id_list.size();
  std::sort(id_list.begin(), id_list.end());

  if( nb_ids > 0 ) {
    int start = 0;
    int pos = start;
    for(unsigned int i=0; i<nb_ids; i++) {
      if( id_list[i] - id_list[pos] > 1 ) {
        subset_list->push_back(new simgrid::jedule::Subset(id_list[start], id_list[pos], parent));
        start = i;

        if( i == nb_ids-1 ) {
         subset_list->push_back(new simgrid::jedule::Subset(id_list[i], id_list[i], parent));
        }
      } else {
        if( i == nb_ids-1 ) {
          subset_list->push_back(new simgrid::jedule::Subset(id_list[start], id_list[i], parent));
        }
      }
      pos = i;
    }
  }

}

void get_resource_selection_by_hosts(std::vector<jed_subset_t> *subset_list, std::vector<sg_host_t> *host_list)
{
  xbt_assert( host_list != nullptr );
  // for each host name
  //  find parent container
  //  group by parent container
  std::unordered_map<const char*, std::vector<const char*>> parent2hostgroup;
  for (auto host: *host_list) {
    const char *host_name = sg_host_get_name(host);
    jed_container_t parent = host2_simgrid_parent_container.at(host_name);
    xbt_assert( parent != nullptr );

    auto host_group = parent2hostgroup.find(parent->name.c_str());
    if (host_group == parent2hostgroup.end())
      parent2hostgroup.insert({parent->name.c_str(), std::vector<const char*>(1,host_name)});
    else
      host_group->second.push_back(host_name);
  }

  for (auto elm: parent2hostgroup) {
    jed_container_t parent = container_name2container.at(elm.first);
    add_subsets_to(subset_list, elm.second, parent);
  }
}

#endif
