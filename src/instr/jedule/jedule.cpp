/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule.hpp"
#include "simgrid/config.h"
#include "xbt/asserts.h"

#if SIMGRID_HAVE_JEDULE

namespace simgrid{
namespace jedule {

void Jedule::add_meta_info(char* key, char* value)
{
  xbt_assert(key != nullptr);
  xbt_assert(value != nullptr);

  this->meta_info_.insert({key, value});
}

void Jedule::add_event(const Event& event)
{
  event_set_.emplace_back(event);
}

void Jedule::write_output(FILE* file)
{
  if (not this->event_set_.empty()) {
    fprintf(file, "<jedule>\n");

    if (not this->meta_info_.empty()) {
      fprintf(file, "  <jedule_meta>\n");
      for (auto const& elm : this->meta_info_)
        fprintf(file, "        <prop key=\"%s\" value=\"%s\" />\n",elm.first,elm.second);
      fprintf(file, "  </jedule_meta>\n");
    }

    fprintf(file, "  <platform>\n");
    this->root_container_.print(file);
    fprintf(file, "  </platform>\n");

    fprintf(file, "  <events>\n");
    for (auto const& event : this->event_set_)
      event.print(file);
    fprintf(file, "  </events>\n");

    fprintf(file, "</jedule>\n");
  }
}

} // namespace jedule
} // namespace simgrid
#endif
