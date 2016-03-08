/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbt/dynar.h"
#include "xbt/asserts.h"

#include "simgrid/jedule/jedule_output.h"

#if HAVE_JEDULE

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_out, jedule, "Logging specific to Jedule output");

xbt_dynar_t jedule_event_list;

static FILE *jed_file;

static void get_hierarchy_list(xbt_dynar_t hier_list, jed_simgrid_container_t container) {
  xbt_assert( container != NULL );

  if( container->parent != NULL ) {

    if( container->parent->container_children == NULL ) {
      // we are in the last level
      get_hierarchy_list(hier_list, container->parent);
    } else {
      unsigned int i;
      int child_nb = -1;
      jed_simgrid_container_t child_container;

      xbt_dynar_foreach(container->parent->container_children, i, child_container) {
        if( child_container == container ) {
          child_nb = i;
          break;
        }
      }

      xbt_assert( child_nb > - 1);
      xbt_dynar_insert_at(hier_list, 0, &child_nb);

      get_hierarchy_list(hier_list, container->parent);
    }
  } else {
    int top_level = 0;
    xbt_dynar_insert_at(hier_list, 0, &top_level);
  }
}

static void get_hierarchy_string(jed_simgrid_container_t container, char *outbuf) {
    char buf[1024];
    xbt_dynar_t hier_list;
    unsigned int iter;
    int number;
    unsigned int length;

    outbuf[0] = '\0';
    hier_list = xbt_dynar_new(sizeof(int), NULL);
    get_hierarchy_list(hier_list, container);

    length = xbt_dynar_length(hier_list);

    xbt_dynar_foreach(hier_list, iter, number) {
        if( iter != length-1 ) {
            sprintf(buf, "%d.", number);
        } else {
            sprintf(buf, "%d", number);
        }
        strcat(outbuf, buf);
    }

    xbt_dynar_free(&hier_list);
}

static void print_key_value_dict(xbt_dict_t key_value_dict) {
  xbt_dict_cursor_t cursor=NULL;
  char *key,*data;

  if( key_value_dict != NULL ) {
    xbt_dict_foreach(key_value_dict,cursor,key,data) {
      fprintf(jed_file, "        <prop key=\"%s\" value=\"%s\" />\n",key,data);
    }
  }
}

static void print_resources(jed_simgrid_container_t resource_parent) {
  unsigned int res_nb;
  unsigned int i;
  char *res_name;
    char resid[1024];
  xbt_assert( resource_parent->resource_list != NULL );

  res_nb = xbt_dynar_length(resource_parent->resource_list);

  get_hierarchy_string(resource_parent, resid);

  fprintf(jed_file, "      <rset id=\"%s\" nb=\"%d\" names=\"", resid, res_nb);
  xbt_dynar_foreach(resource_parent->resource_list, i, res_name) {
    fprintf(jed_file, "%s", res_name);
    if( i != res_nb-1 ) {
      fprintf(jed_file, "|");
    }
  }
  fprintf(jed_file, "\" />\n");
}

static void print_container(jed_simgrid_container_t container) {
  unsigned int i;
  jed_simgrid_container_t child_container;

  xbt_assert( container != NULL );

  fprintf(jed_file, "    <res name=\"%s\">\n", container->name);
  if( container->container_children != NULL ) {
    xbt_dynar_foreach(container->container_children, i, child_container) {
      print_container(child_container);
    }
  } else {
    print_resources(container);
  }
  fprintf(jed_file, "    </res>\n");
}

static void print_platform(jed_simgrid_container_t root_container) {
  fprintf(jed_file, "  <platform>\n");
  print_container(root_container);
  fprintf(jed_file, "  </platform>\n");
}

static void print_event(jed_event_t event) {
  unsigned int i;
  jed_res_subset_t subset;

  xbt_assert( event != NULL );
  xbt_assert( event->resource_subsets != NULL );

  fprintf(jed_file, "    <event>\n");

  fprintf(jed_file, "      <prop key=\"name\" value=\"%s\" />\n", event->name);
  fprintf(jed_file, "      <prop key=\"start\" value=\"%g\" />\n", event->start_time);
  fprintf(jed_file, "      <prop key=\"end\" value=\"%g\" />\n", event->end_time);
  fprintf(jed_file, "      <prop key=\"type\" value=\"%s\" />\n", event->type);

  fprintf(jed_file, "      <res_util>\n");

  xbt_dynar_foreach(event->resource_subsets, i, subset) {
    int start = subset->start_idx;
    int end   = subset->start_idx + subset->nres - 1;
    char resid[1024];

    get_hierarchy_string(subset->parent, resid);
    fprintf(jed_file, "        <select resources=\"");
    fprintf(jed_file, "%s", resid);
    fprintf(jed_file, ".[%d-%d]", start, end);
    fprintf(jed_file, "\" />\n");
  }

  fprintf(jed_file, "      </res_util>\n");
  if (!xbt_dynar_is_empty(event->characteristics_list)){
    fprintf(jed_file, "      <characteristics>\n");
    char *ch;
    unsigned int iter;
    xbt_dynar_foreach(event->characteristics_list, iter, ch) {
      fprintf(jed_file, "          <characteristic name=\"%s\" />\n", ch);
    }
    fprintf(jed_file, "      </characteristics>\n");
  }

  if (!xbt_dict_is_empty(event->info_hash)){
    fprintf(jed_file, "      <info>\n");
    print_key_value_dict(event->info_hash);
    fprintf(jed_file, "      </info>\n");
  }

  fprintf(jed_file, "    </event>\n");
}

static void print_events(xbt_dynar_t event_list)  {
  unsigned int i;
  jed_event_t event;

  fprintf(jed_file, "  <events>\n");
  xbt_dynar_foreach(event_list, i, event) {
    print_event(event);
  }
  fprintf(jed_file, "  </events>\n");
}

void write_jedule_output(FILE *file, jedule_t jedule, xbt_dynar_t event_list, xbt_dict_t meta_info_dict) {

  jed_file = file;
  if (!xbt_dynar_is_empty(jedule_event_list)){

    fprintf(jed_file, "<jedule>\n");

    if (!xbt_dict_is_empty(jedule->jedule_meta_info)){
      fprintf(jed_file, "  <jedule_meta>\n");
      print_key_value_dict(jedule->jedule_meta_info);
      fprintf(jed_file, "  </jedule_meta>\n");
    }

    print_platform(jedule->root_container);

    print_events(event_list);

    fprintf(jed_file, "</jedule>\n");
  }
}

static void jed_event_free_ref(void *evp)
{
  jed_event_t ev = *(jed_event_t *)evp;
  jed_event_free(ev);
}

void jedule_init_output() {
  jedule_event_list = xbt_dynar_new(sizeof(jed_event_t), jed_event_free_ref);
}

void jedule_cleanup_output() {
  xbt_dynar_free(&jedule_event_list);
}

void jedule_store_event(jed_event_t event) {
  xbt_assert(event != NULL);
  xbt_dynar_push(jedule_event_list, &event);
}
#endif
