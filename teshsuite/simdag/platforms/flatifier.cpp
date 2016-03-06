/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/network_interface.hpp"
#include "simgrid/simdag.h"
#include "xbt/xbt_os_time.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(flatifier, "Logging specific to this platform parsing tool");

static int name_compare_hosts(const void *n1, const void *n2)
{
  return strcmp(sg_host_get_name(*(sg_host_t *) n1), sg_host_get_name(*(sg_host_t *) n2));
}

static int name_compare_links(const void *n1, const void *n2)
{
  return strcmp(sg_link_name(*(SD_link_t *) n1),sg_link_name(*(SD_link_t *) n2));
}

static int parse_cmdline(int *timings, char **platformFile, int argc, char **argv)
{
  int wrong_option = 0;
  for (int i = 1; i < argc; i++) {
    if (strlen(argv[i]) > 1 && argv[i][0] == '-' && argv[i][1] == '-') {
      if (!strcmp(argv[i], "--timings")) {
        *timings = 1;
      } else {
          wrong_option = 1;
          break;
      }
    } else {
      *platformFile = argv[i];
    }
  }
  return wrong_option;
}

static void create_environment(xbt_os_timer_t parse_time, const char *platformFile)
{
  xbt_ex_t e;
  TRY {
    xbt_os_cputimer_start(parse_time);
    SD_create_environment(platformFile);
    xbt_os_cputimer_stop(parse_time);
  }
  CATCH(e) {
    xbt_die("Error while loading %s: %s", platformFile, e.msg);
  }
}

int main(int argc, char **argv)
{
  char *platformFile = NULL;
  unsigned int totalHosts, totalLinks;
  int timings=0;
  int version = 4;
  const char *link_ctn = "link_ctn";
  unsigned int i;
  xbt_dict_t props = NULL;
  xbt_dict_cursor_t cursor = NULL;
  xbt_lib_cursor_t cursor_src = NULL;
  xbt_lib_cursor_t cursor_dst = NULL;
  char *src,*dst,*key,*data;
  sg_netcard_t value1;
  sg_netcard_t value2;

  const sg_host_t *hosts;
  const SD_link_t *links;
  xbt_os_timer_t parse_time = xbt_os_timer_new();

  SD_init(&argc, argv);

  if (parse_cmdline(&timings, &platformFile, argc, argv) || !platformFile) {
    xbt_die("Invalid command line arguments: expected [--timings] platformFile");
  }

  XBT_DEBUG("%d,%s", timings, platformFile);

  create_environment(parse_time, platformFile);

  if (timings) {
    XBT_INFO("Parsing time: %fs (%zu hosts, %d links)", xbt_os_timer_elapsed(parse_time),
             sg_host_count(), sg_link_count());
  } else {
    printf("<?xml version='1.0'?>\n");
    printf("<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n");
    printf("<platform version=\"%d\">\n", version);
    printf("<AS id=\"AS0\" routing=\"Full\">\n");

    // Hosts
    totalHosts = sg_host_count();
    hosts = sg_host_list();
    qsort((void *) hosts, totalHosts, sizeof(sg_host_t), name_compare_hosts);

    for (i = 0; i < totalHosts; i++) {
      printf("  <host id=\"%s\" speed=\"%.0f\"", sg_host_get_name(hosts[i]), sg_host_speed(hosts[i]));
      props = sg_host_get_properties(hosts[i]);
      if (sg_host_core_count(hosts[i])>1) {
        printf(" core=\"%d\"", sg_host_core_count(hosts[i]));
      }
      if (props && !xbt_dict_is_empty(props)) {
        printf(">\n");
        xbt_dict_foreach(props, cursor, key, data) {
          printf("    <prop id=\"%s\" value=\"%s\"/>\n", key, data);
        }
        printf("  </host>\n");
      } else {
        printf("/>\n");
      }
    }

    // Routers
    xbt_lib_foreach(as_router_lib, cursor_src, key, value1) {
      value1 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib, key, ROUTING_ASR_LEVEL);
      if(value1->isRouter()) {
        printf("  <router id=\"%s\"/>\n",key);
      }
    }

    // Links
    totalLinks = sg_link_count();
    links = sg_link_list();

    qsort((void *) links, totalLinks, sizeof(SD_link_t), name_compare_links);

    for (i = 0; i < totalLinks; i++) {
      printf("  <link id=\"");

      printf("%s\" bandwidth=\"%.0f\" latency=\"%.9f\"", sg_link_name(links[i]),
             sg_link_bandwidth(links[i]), sg_link_latency(links[i]));
      if (sg_link_is_shared(links[i])) {
        printf("/>\n");
      } else {
        printf(" sharing_policy=\"FATPIPE\"/>\n");
      }
    }

    sg_host_t host1, host2;
    xbt_dict_foreach(host_list, cursor_src, src, host1){ // Routes from host
      value1 = sg_host_by_name(src)->pimpl_netcard;
      xbt_dict_foreach(host_list, cursor_dst, dst, host2){ //to host
        std::vector<Link*> *route = new std::vector<Link*>();
        value2 = sg_host_by_name(dst)->pimpl_netcard;
        routing_platf->getRouteAndLatency(value1, value2, route,NULL);
        if (! route->empty()){
          printf("  <route src=\"%s\" dst=\"%s\">\n  ", src, dst);
          for (auto link: *route)
            printf("<%s id=\"%s\"/>",link_ctn,link->getName());
          printf("\n  </route>\n");
        }
        delete route;
      }
      xbt_lib_foreach(as_router_lib, cursor_dst, dst, value2){ //to router
        value2 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib,dst,ROUTING_ASR_LEVEL);
        if(value2->isRouter()){
          printf("  <route src=\"%s\" dst=\"%s\">\n  ", src, dst);
          std::vector<Link*> *route = new std::vector<Link*>();
          routing_platf->getRouteAndLatency((sg_netcard_t)value1,(sg_netcard_t)value2,route,NULL);
          for (auto link : *route)
            printf("<%s id=\"%s\"/>",link_ctn,link->getName());
          delete route;
          printf("\n  </route>\n");
        }
      }
    }

    xbt_lib_foreach(as_router_lib, cursor_src, src, value1){ // Routes from router
      value1 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib,src,ROUTING_ASR_LEVEL);
      if (value1->isRouter()){
        xbt_lib_foreach(as_router_lib, cursor_dst, dst, value2){ //to router
          value2 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib,dst,ROUTING_ASR_LEVEL);
          if(value2->isRouter()){
            printf("  <route src=\"%s\" dst=\"%s\">\n  ", src, dst);
            std::vector<Link*> *route = new std::vector<Link*>();
            routing_platf->getRouteAndLatency((sg_netcard_t)value1,(sg_netcard_t)value2,route,NULL);
            for(auto link :*route)
              printf("<%s id=\"%s\"/>",link_ctn,link->getName());
            delete route;
            printf("\n  </route>\n");
          }
        }
        xbt_dict_foreach(host_list, cursor_dst, dst, value2){ //to host
          printf("  <route src=\"%s\" dst=\"%s\">\n  ",src, dst);
          std::vector<Link*> *route = new std::vector<Link*>();
          value2 = sg_host_by_name(dst)->pimpl_netcard;
          routing_platf->getRouteAndLatency((sg_netcard_t)value1,(sg_netcard_t)value2,route, NULL);
          for(auto link : *route)
            printf("<%s id=\"%s\"/>",link_ctn,link->getName());
          delete route;
          printf("\n  </route>\n");
        }
      }
    }

    printf("</AS>\n");
    printf("</platform>\n");
  }
  SD_exit();
  xbt_os_timer_free(parse_time);

  return 0;
}
