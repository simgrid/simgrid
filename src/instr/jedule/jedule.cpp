/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbt/asserts.h"
#include "simgrid/jedule/jedule.hpp"

#if HAVE_JEDULE

namespace simgrid{
namespace jedule {

Jedule::~Jedule() {
  delete this->root_container;
  for (auto evt: this->event_set)
    delete evt;
  this->event_set.clear();
}

void Jedule::addMetaInfo(char *key, char *value) {
  xbt_assert(key != nullptr);
  xbt_assert(value != nullptr);

  this->meta_info.insert({key, value});
}

void Jedule::cleanupOutput() {
  for (auto evt: this->event_set)
    delete evt;
  this->event_set.clear();
}
void Jedule::writeOutput(FILE *file) {
  if (!this->event_set.empty()){

    fprintf(file, "<jedule>\n");

    if (!this->meta_info.empty()){
      fprintf(file, "  <jedule_meta>\n");
      for (auto elm: this->meta_info)
        fprintf(file, "        <prop key=\"%s\" value=\"%s\" />\n",elm.first,elm.second);
      fprintf(file, "  </jedule_meta>\n");
    }

    fprintf(file, "  <platform>\n");
    this->root_container->print(file);
    fprintf(file, "  </platform>\n");

    fprintf(file, "  <events>\n");
    for (auto event :this->event_set)
        event->print(file);
    fprintf(file, "  </events>\n");

    fprintf(file, "</jedule>\n");
  }
}

}
}
#endif
