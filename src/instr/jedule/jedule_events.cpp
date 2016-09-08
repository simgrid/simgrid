/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule_events.hpp"
#include "simgrid/jedule/jedule.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "xbt/asserts.h"

#if HAVE_JEDULE
namespace simgrid{
namespace jedule{

Event::~Event(){
  while (!this->resource_subsets.empty()){
    xbt_free(this->resource_subsets.back());
    this->resource_subsets.pop_back();
  }
}

void Event::addResources(std::vector<sg_host_t> *host_selection) {
  xbt_dynar_t resource_subset_list;
  jed_res_subset_t res_set;
  unsigned int i;

  resource_subset_list = xbt_dynar_new(sizeof(jed_res_subset_t), nullptr);

  jed_simgrid_get_resource_selection_by_hosts(resource_subset_list, host_selection);
  xbt_dynar_foreach(resource_subset_list, i, res_set)  {
    this->resource_subsets.push_back(res_set);
  }

  xbt_dynar_free_container(&resource_subset_list);
}

void Event::addCharacteristic(char *characteristic) {
  xbt_assert( characteristic != nullptr );
  this->characteristics_list.push_back(characteristic);
}

void Event::addInfo(char* key, char *value) {
  xbt_assert((key != nullptr) && value != nullptr);
  this->info_map.insert({key, value});
}

void Event::print(FILE *jed_file){
  fprintf(jed_file, "    <event>\n");
  fprintf(jed_file, "      <prop key=\"name\" value=\"%s\" />\n", this->name.c_str());
  fprintf(jed_file, "      <prop key=\"start\" value=\"%g\" />\n", this->start_time);
  fprintf(jed_file, "      <prop key=\"end\" value=\"%g\" />\n", this->end_time);
  fprintf(jed_file, "      <prop key=\"type\" value=\"%s\" />\n", this->type.c_str());

  xbt_assert(!this->resource_subsets.empty());
  fprintf(jed_file, "      <res_util>\n");
  for (auto subset: this->resource_subsets) {
    fprintf(jed_file, "        <select resources=\"");
    fprintf(jed_file, "%s", subset->parent->getHierarchyAsString().c_str());
    fprintf(jed_file, ".[%d-%d]", subset->start_idx, subset->start_idx + subset->nres - 1);
    fprintf(jed_file, "\" />\n");
  }
  fprintf(jed_file, "      </res_util>\n");

  if (!this->characteristics_list.empty()){
    fprintf(jed_file, "      <characteristics>\n");
    for (auto ch: this->characteristics_list)
      fprintf(jed_file, "          <characteristic name=\"%s\" />\n", ch);
    fprintf(jed_file, "      </characteristics>\n");
  }

  if (!this->info_map.empty()){
    fprintf(jed_file, "      <info>\n");
    for (auto elm: this->info_map)
      fprintf(jed_file, "        <prop key=\"%s\" value=\"%s\" />\n",elm.first,elm.second);
    fprintf(jed_file, "      </info>\n");
  }

  fprintf(jed_file, "    </event>\n");
}

}
}
#endif
