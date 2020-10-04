/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule.hpp"
#include "simgrid/host.h"
#include "simgrid/s4u/NetZone.hpp"
#include "xbt/asserts.h"
#include <algorithm>

#if SIMGRID_HAVE_JEDULE

namespace {
std::unordered_map<const char*, jed_container_t> host2_simgrid_parent_container;
std::unordered_map<std::string, jed_container_t> container_name2container;
}

namespace simgrid {
namespace jedule {
Subset::Subset(int start_idx, int end_idx, Container* parent)
    : start_idx(start_idx), nres(end_idx - start_idx + 1), parent(parent)
{
}

Container::Container(const std::string& name) : name(name)
{
  container_name2container.insert({this->name, this});
}

void Container::add_child(jed_container_t child)
{
  xbt_assert(child != nullptr);
  children_.emplace_back(child);
  child->set_parent(this);
}

void Container::add_resources(std::vector<sg_host_t> hosts)
{
  children_.clear();
  last_id_ = 0;

  for (auto const& host : hosts) {
    const char *host_name = sg_host_get_name(host);
    this->name2id.insert({host_name, this->last_id_});
    (this->last_id_)++;
    host2_simgrid_parent_container.insert({host_name, this});
    this->resource_list.push_back(host);
  }
}

void Container::create_hierarchy(const_sg_netzone_t from_as)
{
  if (from_as->get_children().empty()) {
    // I am no AS
    // add hosts to jedule platform
    std::vector<sg_host_t> table = from_as->get_all_hosts();
    this->add_resources(table);
  } else {
    for (auto const& nz : from_as->get_children()) {
      auto* child_container = new simgrid::jedule::Container(nz->get_name());
      this->add_child(child_container);
      child_container->create_hierarchy(nz);
    }
  }
}

int Container::get_child_position(const Container* child) const
{
  auto it = std::find_if(begin(children_), end(children_),
                         [&child](const std::unique_ptr<Container>& c) { return c.get() == child; });
  return it == end(children_) ? -1 : static_cast<int>(std::distance(begin(children_), it));
}

std::vector<int> Container::get_hierarchy()
{
  if (parent_ == nullptr) {
    int top_level = 0;
    std::vector<int> hier_list = {top_level};
    return hier_list;
  } else if (parent_->has_children()) {
    int child_nb = parent_->get_child_position(this);
    xbt_assert(child_nb > -1);
    std::vector<int> hier_list = parent_->get_hierarchy();
    hier_list.insert(hier_list.begin(), child_nb);
    return hier_list;
  } else {
    // we are in the last level
    return parent_->get_hierarchy();
  }
}

std::string Container::get_hierarchy_as_string()
{
  std::string output("");

  std::vector<int> hier_list = this->get_hierarchy();

  bool sep = false;
  for (auto const& id : hier_list) {
    if (sep)
      output += '.';
    else
      sep = true;
    output += std::to_string(id);
  }

  return output;
}

void Container::print_resources(FILE* jed_file)
{
  xbt_assert(not this->resource_list.empty());

  std::string resid = this->get_hierarchy_as_string();

  fprintf(jed_file, "      <rset id=\"%s\" nb=\"%zu\" names=\"", resid.c_str(), this->resource_list.size());
  bool sep = false;
  for (auto const& res : this->resource_list) {
    if (sep)
      putc('|', jed_file);
    else
      sep = true;
    const char * res_name = sg_host_get_name(res);
    fprintf(jed_file, "%s", res_name);
  }
  fprintf(jed_file, "\" />\n");
}

void Container::print(FILE* jed_file)
{
  fprintf(jed_file, "    <res name=\"%s\">\n", this->name.c_str());
  if (not children_.empty()) {
    for (auto const& child : children_) {
      child->print(jed_file);
    }
  } else {
    this->print_resources(jed_file);
  }
  fprintf(jed_file, "    </res>\n");
}

} // namespace jedule
} // namespace simgrid

static void add_subsets_to(std::vector<simgrid::jedule::Subset>& subset_list, std::vector<const char*> hostgroup,
                           jed_container_t parent)
{
  // get ids for each host
  // sort ids
  // compact ids
  // create subset for each id group

  xbt_assert( parent != nullptr );

  std::vector<unsigned int> id_list;

  for (auto const& host_name : hostgroup) {
    xbt_assert( host_name != nullptr );
    const simgrid::jedule::Container* parent_cont = host2_simgrid_parent_container.at(host_name);
    unsigned int id             = parent_cont->get_id_by_name(host_name);
    id_list.push_back(id);
  }
  std::sort(id_list.begin(), id_list.end());

  size_t nb_ids = id_list.size();
  size_t start  = 0;
  size_t pos    = start;
  for (size_t i = 0; i < nb_ids; i++) {
    if (id_list[i] - id_list[pos] > 1) {
      subset_list.emplace_back(id_list[start], id_list[pos], parent);
      start = i;

      if (i == nb_ids - 1) {
        subset_list.emplace_back(id_list[i], id_list[i], parent);
      }
    } else {
      if (i == nb_ids - 1) {
        subset_list.emplace_back(id_list[start], id_list[i], parent);
      }
    }
    pos = i;
  }
}

void get_resource_selection_by_hosts(std::vector<simgrid::jedule::Subset>& subset_list,
                                     const std::vector<sg_host_t>& host_list)
{
  // for each host name
  //  find parent container
  //  group by parent container
  std::unordered_map<const char*, std::vector<const char*>> parent2hostgroup;
  for (auto const& host : host_list) {
    const char *host_name = sg_host_get_name(host);
    const simgrid::jedule::Container* parent = host2_simgrid_parent_container.at(host_name);
    xbt_assert( parent != nullptr );

    auto host_group = parent2hostgroup.find(parent->get_cname());
    if (host_group == parent2hostgroup.end())
      parent2hostgroup.insert({parent->get_cname(), std::vector<const char*>(1, host_name)});
    else
      host_group->second.push_back(host_name);
  }

  for (auto const& elm : parent2hostgroup) {
    jed_container_t parent = container_name2container.at(elm.first);
    add_subsets_to(subset_list, elm.second, parent);
  }
}

#endif
