/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "xbt/asserts.h"

#if SIMGRID_HAVE_JEDULE
namespace simgrid{
namespace jedule{

Event::Event(std::string name, double start_time, double end_time, std::string type)
  : name(name), start_time(start_time), end_time(end_time), type(type)
{
  this->resource_subsets = new std::vector<jed_subset_t>();
}

Event::~Event()
{
  if (not this->resource_subsets->empty()) {
    for (auto const& subset : *this->resource_subsets)
      delete subset;
    delete this->resource_subsets;
  }
}

void Event::addResources(std::vector<sg_host_t> *host_selection)
{
  get_resource_selection_by_hosts(this->resource_subsets, host_selection);
}

void Event::addCharacteristic(char *characteristic)
{
  xbt_assert( characteristic != nullptr );
  this->characteristics_list.push_back(characteristic);
}

void Event::addInfo(char* key, char *value) {
  xbt_assert((key != nullptr) && value != nullptr);
  this->info_map.insert({key, value});
}

void Event::print(FILE *jed_file)
{
  fprintf(jed_file, "    <event>\n");
  fprintf(jed_file, "      <prop key=\"name\" value=\"%s\" />\n", this->name.c_str());
  fprintf(jed_file, "      <prop key=\"start\" value=\"%g\" />\n", this->start_time);
  fprintf(jed_file, "      <prop key=\"end\" value=\"%g\" />\n", this->end_time);
  fprintf(jed_file, "      <prop key=\"type\" value=\"%s\" />\n", this->type.c_str());

  xbt_assert(not this->resource_subsets->empty());
  fprintf(jed_file, "      <res_util>\n");
  for (auto const& subset : *this->resource_subsets) {
    fprintf(jed_file, "        <select resources=\"");
    fprintf(jed_file, "%s", subset->parent->getHierarchyAsString().c_str());
    fprintf(jed_file, ".[%d-%d]", subset->start_idx, subset->start_idx + subset->nres-1);
    fprintf(jed_file, "\" />\n");
  }
  fprintf(jed_file, "      </res_util>\n");

  if (not this->characteristics_list.empty()) {
    fprintf(jed_file, "      <characteristics>\n");
    for (auto const& ch : this->characteristics_list)
      fprintf(jed_file, "          <characteristic name=\"%s\" />\n", ch);
    fprintf(jed_file, "      </characteristics>\n");
  }

  if (not this->info_map.empty()) {
    fprintf(jed_file, "      <info>\n");
    for (auto const& elm : this->info_map)
      fprintf(jed_file, "        <prop key=\"%s\" value=\"%s\" />\n",elm.first,elm.second);
    fprintf(jed_file, "      </info>\n");
  }

  fprintf(jed_file, "    </event>\n");
}

}
}
#endif
