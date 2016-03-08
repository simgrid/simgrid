/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule_platform.h"

#include "xbt/asserts.h"
#include "xbt/dynar.h"
#include "xbt/str.h"

#include <stdlib.h>
#include <string.h>

#if HAVE_JEDULE

static xbt_dict_t host2_simgrid_parent_container;
static xbt_dict_t container_name2container;

static int compare_ids(const void *num1, const void *num2) {
  return *((int*) num1) - *((int*) num2);
}

static void jed_free_container(jed_simgrid_container_t container) {
  xbt_dict_free(&container->name2id);
  xbt_dynar_free(&container->resource_list);

  if( container->container_children != NULL ) {
    unsigned int iter;
    jed_simgrid_container_t child_container;
    xbt_dynar_foreach(container->container_children, iter, child_container) {
      jed_free_container(child_container);
    }
    xbt_dynar_free(&container->container_children);
  }
  xbt_free(container->name);
  xbt_free(container);
}

void jed_simgrid_create_container(jed_simgrid_container_t *container, const char *name)
{
  xbt_assert( name != NULL );

  *container = xbt_new0(s_jed_simgrid_container_t,1);
  (*container)->name = xbt_strdup(name);
  (*container)->is_lowest = 0;
  (*container)->container_children = xbt_dynar_new(sizeof(jed_simgrid_container_t), NULL);
  (*container)->parent = NULL;

  xbt_dict_set(container_name2container, (*container)->name, *container, NULL);
}

void jed_simgrid_add_container(jed_simgrid_container_t parent, jed_simgrid_container_t child) {
  xbt_assert(parent != NULL);
  xbt_assert(child != NULL);
  xbt_dynar_push(parent->container_children, &child);
  child->parent = parent;
}

void jed_simgrid_add_resources(jed_simgrid_container_t parent, xbt_dynar_t host_names) {
  unsigned int iter;
  char *host_name;
  char *buf;

  parent->is_lowest = 1;
  xbt_dynar_free(&parent->container_children);
  parent->container_children = NULL;
  parent->name2id = xbt_dict_new_homogeneous(xbt_free_f);
  parent->last_id = 0;
  parent->resource_list = xbt_dynar_new(sizeof(char *), NULL);

  xbt_dynar_sort_strings(host_names);

  xbt_dynar_foreach(host_names, iter, host_name) {
    buf = bprintf("%d", parent->last_id);
    (parent->last_id)++;
    xbt_dict_set(parent->name2id, host_name, buf, NULL);
    xbt_dict_set(host2_simgrid_parent_container, host_name, parent, NULL);
    xbt_dynar_push(parent->resource_list, &host_name);
  }
}

static void add_subset_to(xbt_dynar_t subset_list, int start, int end, jed_simgrid_container_t parent) {
  jed_res_subset_t subset;

  xbt_assert( subset_list != NULL );
  xbt_assert( parent != NULL );

  subset = xbt_new0(s_jed_res_subset_t,1);
  subset->start_idx = start;
  subset->nres      = end-start+1;
  subset->parent    = parent;

  xbt_dynar_push(subset_list, &subset);
}

static void add_subsets_to(xbt_dynar_t subset_list, xbt_dynar_t hostgroup, jed_simgrid_container_t parent) {
  unsigned int iter;
  char *host_name;
  xbt_dynar_t id_list;
  int *id_ar;
  int nb_ids;
  char *id_str;

  // get ids for each host
  // sort ids
  // compact ids
  // create subset for each id group

  xbt_assert( host2_simgrid_parent_container != NULL );
  xbt_assert( subset_list != NULL );
  xbt_assert( hostgroup != NULL );
  xbt_assert( parent != NULL );

  id_list = xbt_dynar_new(sizeof(char *), NULL);

  xbt_dynar_foreach(hostgroup, iter, host_name) {
    jed_simgrid_container_t parent;
    xbt_assert( host_name != NULL );
    parent = (jed_simgrid_container_t)xbt_dict_get(host2_simgrid_parent_container, host_name);
    id_str = (char*)xbt_dict_get(parent->name2id, host_name);
    xbt_dynar_push(id_list, &id_str);
  }

  nb_ids = xbt_dynar_length(id_list);
  id_ar = xbt_new0(int,nb_ids);
  xbt_dynar_foreach(id_list, iter, id_str) {
    id_ar[iter] = xbt_str_parse_int(id_str, "Parse error: not a number: %s");
  }

  qsort (id_ar, nb_ids, sizeof(int), &compare_ids);

  if( nb_ids > 0 ) {
    int start = 0;
    int pos;
    int i;

    pos = start;
    for(i=0; i<nb_ids; i++) {
      if( id_ar[i] - id_ar[pos] > 1 ) {
        add_subset_to( subset_list, id_ar[start], id_ar[pos], parent );
        start = i;

        if( i == nb_ids-1 ) {
          add_subset_to( subset_list, id_ar[i], id_ar[i], parent );
        }
      } else {
        if( i == nb_ids-1 ) {
          add_subset_to( subset_list, id_ar[start], id_ar[i], parent );
        }
      }

      pos = i;
    }
  }

  free(id_ar);
  xbt_dynar_free(&id_list);
}

void jed_simgrid_get_resource_selection_by_hosts(xbt_dynar_t subset_list, xbt_dynar_t host_names) {
  char *host_name;
  unsigned int iter;
  xbt_dict_t parent2hostgroup;  // group hosts by parent

  parent2hostgroup = xbt_dict_new_homogeneous(NULL);

  xbt_assert( host_names != NULL );

  // for each host name
  //  find parent container
  //  group by parent container

  xbt_dynar_foreach(host_names, iter, host_name) {
    jed_simgrid_container_t parent = (jed_simgrid_container_t)xbt_dict_get(host2_simgrid_parent_container, host_name);
    xbt_assert( parent != NULL );

    xbt_dynar_t hostgroup = (xbt_dynar_t)xbt_dict_get_or_null (parent2hostgroup, parent->name);
    if( hostgroup == NULL ) {
      hostgroup = xbt_dynar_new(sizeof(char*), NULL);
      xbt_dict_set(parent2hostgroup, parent->name, hostgroup, NULL);
    }

    xbt_dynar_push(hostgroup, &host_name);
  }

  xbt_dict_cursor_t cursor=NULL;
  char *parent_name;
  xbt_dynar_t hostgroup;
  jed_simgrid_container_t parent;

  xbt_dict_foreach(parent2hostgroup,cursor,parent_name,hostgroup) {
    parent = (jed_simgrid_container_t)xbt_dict_get(container_name2container, parent_name);
    add_subsets_to(subset_list, hostgroup, parent);
  }
  xbt_dynar_free(&hostgroup);

  xbt_dict_free(&parent2hostgroup);
}

void jedule_add_meta_info(jedule_t jedule, char *key, char *value) {
  char *val_cp;

  xbt_assert(key != NULL);
  xbt_assert(value != NULL);

  val_cp = xbt_strdup(value);
  xbt_dict_set(jedule->jedule_meta_info, key, val_cp, NULL);
}

void jed_create_jedule(jedule_t *jedule) {
  *jedule = xbt_new0(s_jedule_t,1);
  host2_simgrid_parent_container = xbt_dict_new_homogeneous(NULL);
  container_name2container       = xbt_dict_new_homogeneous(NULL);
  (*jedule)->jedule_meta_info    = xbt_dict_new_homogeneous(NULL);
}

void jed_free_jedule(jedule_t jedule) {
  jed_free_container(jedule->root_container);

  xbt_dict_free(&jedule->jedule_meta_info);
  xbt_free(jedule);

  xbt_dict_free(&host2_simgrid_parent_container);
  xbt_dict_free(&container_name2container);
}
#endif
