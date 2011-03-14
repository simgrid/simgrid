/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>


#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "xbt/ex.h"
#include "xbt/graph.h"
#include "surf/surf.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(graphicator,
                             "Graphicator Logging System");

static int name_compare_links(const void *n1, const void *n2)
{
  char name1[80], name2[80];
  strcpy(name1, SD_link_get_name(*((SD_link_t *) n1)));
  strcpy(name2, SD_link_get_name(*((SD_link_t *) n2)));

  return strcmp(name1, name2);
}

static const char *node_name(xbt_node_t n)
{
  return xbt_graph_node_get_data(n);
}

static const char *edge_name(xbt_edge_t n)
{
  return xbt_graph_edge_get_data(n);
}

static xbt_node_t xbt_graph_search_node (xbt_graph_t graph, void *data,  int (*compare_function)(const char *, const char *))
{
  unsigned int cursor = 0;
  void *tmp = NULL;

  xbt_dynar_t dynar = xbt_graph_get_nodes (graph);
  xbt_dynar_foreach(dynar, cursor, tmp) {
    xbt_node_t node = (xbt_node_t)tmp;
    if (!compare_function (data, xbt_graph_node_get_data (node))) return node;
  }
  return NULL;
}

static xbt_edge_t xbt_graph_search_edge (xbt_graph_t graph, xbt_node_t n1, xbt_node_t n2)
{
  unsigned int cursor = 0;
  void *tmp = NULL;
  xbt_dynar_t dynar = xbt_graph_get_edges (graph);
  xbt_dynar_foreach(dynar, cursor, tmp) {
    xbt_edge_t edge = (xbt_edge_t)tmp;
    if (( xbt_graph_edge_get_source(edge) == n1 &&
          xbt_graph_edge_get_target(edge) == n2) ||
        ( xbt_graph_edge_get_source(edge) == n2 &&
          xbt_graph_edge_get_target(edge) == n1)){
      return edge;
    }
  }
  return NULL;
}

int main(int argc, char **argv)
{
  char *platformFile = NULL;
  char *graphvizFile = NULL;

  unsigned int i;
  xbt_dict_cursor_t cursor_src = NULL;
  xbt_dict_cursor_t cursor_dst = NULL;
  char *src;
  char *dst;
  char *data;
  xbt_ex_t e;

  SD_init(&argc, argv);

  if (argc < 3){
    XBT_INFO("Usage: %s <platform_file.xml> <graphviz_file.dot>", argv[0]);
    return 1;
  }
  platformFile = argv[1];
  graphvizFile = argv[2];

  TRY {
    SD_create_environment(platformFile);
  } CATCH(e) {
    xbt_die("Error while loading %s: %s",platformFile,e.msg);
  }

  //creating the graph structure
  xbt_graph_t graph = xbt_graph_new_graph (0, NULL);

  //adding hosts
  xbt_dict_foreach(global_routing->where_network_elements, cursor_src, src, data) {
    xbt_graph_new_node (graph, xbt_strdup(src));
  }

  //adding links
  int totalLinks = SD_link_get_number();
  const SD_link_t *links = SD_link_get_list();
  qsort((void *) links, totalLinks, sizeof(SD_link_t), name_compare_links);
  for (i = 0; i < totalLinks; i++) {
    xbt_graph_new_node (graph, xbt_strdup (SD_link_get_name(links[i])));
  }

  xbt_dict_foreach(global_routing->where_network_elements, cursor_src, src, data) {
    xbt_dict_foreach(global_routing->where_network_elements, cursor_dst, dst, data) {
      if (strcmp(src,"loopback")==0 || strcmp(dst,"loopback")==0) continue;

      xbt_node_t src_node = xbt_graph_search_node (graph, src, strcmp);
      xbt_node_t dst_node = xbt_graph_search_node (graph, dst, strcmp);
      if(get_network_element_type(src) != SURF_NETWORK_ELEMENT_AS &&
    		  get_network_element_type(dst) != SURF_NETWORK_ELEMENT_AS ){
        xbt_dynar_t route = global_routing->get_route(src,dst);
        xbt_node_t previous = src_node;
        for(i=0;i<xbt_dynar_length(route) ;i++)
        {
          void *link = xbt_dynar_get_as(route,i,void *);
          char *link_name = bprintf("%s",((surf_resource_t) link)->name);
          if (strcmp(link_name, "loopback")==0 || strcmp(link_name, "__loopback__")==0) continue;
          xbt_node_t link_node = xbt_graph_search_node (graph, link_name, strcmp);
          if (!link_node){
            link_node = xbt_graph_new_node (graph, strdup(link_name));
          }
          xbt_edge_t edge = xbt_graph_search_edge (graph, previous, link_node);
          if (!edge){
            XBT_DEBUG("\%s %s", (char*)xbt_graph_node_get_data(previous), (char*)xbt_graph_node_get_data(link_node));
            xbt_graph_new_edge (graph, previous, link_node, NULL);
          }
          previous = link_node;
          free(link_name);
        }
        xbt_edge_t edge = xbt_graph_search_edge (graph, previous, dst_node);
        if (!edge){
          XBT_DEBUG("\%s %s", (char*)xbt_graph_node_get_data(previous), (char*)xbt_graph_node_get_data(dst_node));
          xbt_graph_new_edge (graph, previous, dst_node, NULL);
        }
    }
   }
  }
  xbt_graph_export_graphviz (graph, graphvizFile, node_name, edge_name);
  xbt_graph_free_graph (graph, NULL, NULL, NULL);
  SD_exit();

  return 0;
}
