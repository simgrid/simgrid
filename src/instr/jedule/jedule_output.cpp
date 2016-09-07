/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule_output.hpp"
#include "simgrid/host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbt/dynar.h"
#include "xbt/asserts.h"

#if HAVE_JEDULE

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_out, jedule, "Logging specific to Jedule output");

xbt_dynar_t jedule_event_list;

void write_jedule_output(FILE *file, jedule_t jedule, xbt_dynar_t event_list) {
  if (!xbt_dynar_is_empty(jedule_event_list)){

    fprintf(file, "<jedule>\n");

    if (!jedule->jedule_meta_info.empty()){
      fprintf(file, "  <jedule_meta>\n");
      for (auto elm: jedule->jedule_meta_info)
        fprintf(file, "        <prop key=\"%s\" value=\"%s\" />\n",elm.first,elm.second);
      fprintf(file, "  </jedule_meta>\n");
    }

    fprintf(file, "  <platform>\n");
    jedule->root_container->print(file);
    fprintf(file, "  </platform>\n");

    fprintf(file, "  <events>\n");
    unsigned int i;
    jed_event_t event;
    xbt_dynar_foreach(event_list, i, event) {
        event->print(file);
    }
    fprintf(file, "  </events>\n");

    fprintf(file, "</jedule>\n");
  }
}

void jedule_init_output() {
  jedule_event_list = xbt_dynar_new(sizeof(jed_event_t), nullptr);
}

void jedule_cleanup_output() {
  while (!xbt_dynar_is_empty(jedule_event_list)) {
    jed_event_t evt = xbt_dynar_pop_as(jedule_event_list, jed_event_t) ;
    evt->deleteEvent();
  }

  xbt_dynar_free_container(&jedule_event_list);
}

void jedule_store_event(jed_event_t event) {
  xbt_assert(event != nullptr);
  xbt_dynar_push(jedule_event_list, &event);
}
#endif
