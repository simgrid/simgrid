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

#define STR_BUF_SIZE 1024

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_out, jedule, "Logging specific to Jedule output");

xbt_dynar_t jedule_event_list;


void print_key_value_dict(FILE *jed_file, std::unordered_map<char*, char*> key_value_dict) {
  if(!key_value_dict.empty()) {
    for (auto elm: key_value_dict) {
      fprintf(jed_file, "        <prop key=\"%s\" value=\"%s\" />\n",elm.first,elm.second);
    }
  }
}

static void print_platform(FILE *jed_file, jed_container_t root_container) {
  fprintf(jed_file, "  <platform>\n");
  root_container->print(jed_file);
  fprintf(jed_file, "  </platform>\n");
}

static void print_events(FILE *jed_file, xbt_dynar_t event_list)  {
  unsigned int i;
  jed_event_t event;

  fprintf(jed_file, "  <events>\n");
  xbt_dynar_foreach(event_list, i, event) {
      event->print(jed_file);
  }
  fprintf(jed_file, "  </events>\n");
}

void write_jedule_output(FILE *file, jedule_t jedule, xbt_dynar_t event_list, xbt_dict_t meta_info_dict) {
  if (!xbt_dynar_is_empty(jedule_event_list)){

    fprintf(file, "<jedule>\n");

    if (!jedule->jedule_meta_info.empty()){
      fprintf(file, "  <jedule_meta>\n");
      print_key_value_dict(file, jedule->jedule_meta_info);
      fprintf(file, "  </jedule_meta>\n");
    }

    print_platform(file, jedule->root_container);

    print_events(file, event_list);

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
