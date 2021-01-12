/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "xbt/asserts.h"

#if SIMGRID_HAVE_JEDULE
namespace simgrid{
namespace jedule{

void Event::add_resources(const std::vector<sg_host_t>& host_selection)
{
  get_resource_selection_by_hosts(this->resource_subsets_, host_selection);
}

void Event::add_characteristic(const char* characteristic)
{
  xbt_assert( characteristic != nullptr );
  this->characteristics_list_.emplace_back(characteristic);
}

void Event::add_info(char* key, char* value)
{
  xbt_assert((key != nullptr) && value != nullptr);
  this->info_map_.insert({key, value});
}

void Event::print(FILE* jed_file) const
{
  fprintf(jed_file, "    <event>\n");
  fprintf(jed_file, "      <prop key=\"name\" value=\"%s\" />\n", this->name_.c_str());
  fprintf(jed_file, "      <prop key=\"start\" value=\"%g\" />\n", this->start_time_);
  fprintf(jed_file, "      <prop key=\"end\" value=\"%g\" />\n", this->end_time_);
  fprintf(jed_file, "      <prop key=\"type\" value=\"%s\" />\n", this->type_.c_str());

  xbt_assert(not this->resource_subsets_.empty());
  fprintf(jed_file, "      <res_util>\n");
  for (auto const& subset : this->resource_subsets_) {
    fprintf(jed_file, "        <select resources=\"");
    fprintf(jed_file, "%s", subset.parent->get_hierarchy_as_string().c_str());
    fprintf(jed_file, ".[%d-%d]", subset.start_idx, subset.start_idx + subset.nres - 1);
    fprintf(jed_file, "\" />\n");
  }
  fprintf(jed_file, "      </res_util>\n");

  if (not this->characteristics_list_.empty()) {
    fprintf(jed_file, "      <characteristics>\n");
    for (auto const& ch : this->characteristics_list_)
      fprintf(jed_file, "          <characteristic name=\"%s\" />\n", ch.c_str());
    fprintf(jed_file, "      </characteristics>\n");
  }

  if (not this->info_map_.empty()) {
    fprintf(jed_file, "      <info>\n");
    for (auto const& elm : this->info_map_)
      fprintf(jed_file, "        <prop key=\"%s\" value=\"%s\" />\n", elm.first.c_str(), elm.second.c_str());
    fprintf(jed_file, "      </info>\n");
  }

  fprintf(jed_file, "    </event>\n");
}

}
}
#endif
