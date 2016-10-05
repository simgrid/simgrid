/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <xbt/dict.h>
#include <xbt/lib.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/xbt_os_time.h>

#include <simgrid/s4u/host.hpp>

#include <simgrid/simdag.h>

#include "src/surf/network_interface.hpp"


XBT_LOG_NEW_DEFAULT_CATEGORY(flatifier, "Logging specific to this platform parsing tool");

static int name_compare_hosts(const void *n1, const void *n2)
{
  return std::strcmp(sg_host_get_name(*(sg_host_t *) n1), sg_host_get_name(*(sg_host_t *) n2));
}

static int name_compare_links(const void *n1, const void *n2)
{
  return std::strcmp(sg_link_name(*(SD_link_t *) n1),sg_link_name(*(SD_link_t *) n2));
}

static int parse_cmdline(int *timings, char **platformFile, int argc, char **argv)
{
  int wrong_option = 0;
  for (int i = 1; i < argc; i++) {
    if (std::strlen(argv[i]) > 1 && argv[i][0] == '-' && argv[i][1] == '-') {
      if (!std::strcmp(argv[i], "--timings")) {
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
  try {
    xbt_os_cputimer_start(parse_time);
    SD_create_environment(platformFile);
    xbt_os_cputimer_stop(parse_time);
  }
  catch (std::exception& e) {
    xbt_die("Error while loading %s: %s", platformFile, e.what());
  }
}

int main(int argc, char **argv)
{
  char *platformFile = nullptr;
  int timings=0;
  int version = 4;
  unsigned int i;
  xbt_dict_t props = nullptr;
  xbt_dict_cursor_t cursor = nullptr;
  xbt_lib_cursor_t cursor_src = nullptr;
  xbt_lib_cursor_t cursor_dst = nullptr;
  char *src,*dst,*key,*data;
  sg_netcard_t value1;
  sg_netcard_t value2;

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
    std::printf("<?xml version='1.0'?>\n");
    std::printf("<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n");
    std::printf("<platform version=\"%d\">\n", version);
    std::printf("<AS id=\"AS0\" routing=\"Full\">\n");

    // Hosts
    unsigned int totalHosts = sg_host_count();
    sg_host_t *hosts = sg_host_list();
    std::qsort((void *) hosts, totalHosts, sizeof(sg_host_t), name_compare_hosts);

    for (i = 0; i < totalHosts; i++) {
      std::printf("  <host id=\"%s\" speed=\"%.0f\"", sg_host_get_name(hosts[i]), sg_host_speed(hosts[i]));
      props = sg_host_get_properties(hosts[i]);
      if (hosts[i]->coresCount()>1) {
        std::printf(" core=\"%d\"", hosts[i]->coresCount());
      }
      if (props && !xbt_dict_is_empty(props)) {
        std::printf(">\n");
        xbt_dict_foreach(props, cursor, key, data) {
          std::printf("    <prop id=\"%s\" value=\"%s\"/>\n", key, data);
        }
        std::printf("  </host>\n");
      } else {
        std::printf("/>\n");
      }
    }

    // Routers
    xbt_lib_foreach(as_router_lib, cursor_src, key, value1) {
      value1 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib, key, ROUTING_ASR_LEVEL);
      if(value1->isRouter()) {
        std::printf("  <router id=\"%s\"/>\n",key);
      }
    }

    // Links
    unsigned int totalLinks = sg_link_count();
    SD_link_t *links = sg_link_list();

    std::qsort((void *) links, totalLinks, sizeof(SD_link_t), name_compare_links);

    for (i = 0; i < totalLinks; i++) {
      std::printf("  <link id=\"");

      std::printf("%s\" bandwidth=\"%.0f\" latency=\"%.9f\"", sg_link_name(links[i]),
             sg_link_bandwidth(links[i]), sg_link_latency(links[i]));
      if (sg_link_is_shared(links[i])) {
        std::printf("/>\n");
      } else {
        std::printf(" sharing_policy=\"FATPIPE\"/>\n");
      }
    }

    sg_host_t host1, host2;
    for (unsigned int it_src = 0; it_src < totalHosts; it_src++) { // Routes from host
      host1 = hosts[it_src];
      value1 = sg_host_by_name(host1->name().c_str())->pimpl_netcard;
      for (unsigned int it_dst = 0; it_dst < totalHosts; it_dst++) { // Routes to host
        host2 = hosts[it_dst];
        std::vector<Link*> *route = new std::vector<Link*>();
        value2 = host2->pimpl_netcard;
        routing_platf->getRouteAndLatency(value1, value2, route,nullptr);
        if (! route->empty()){
          std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", host1->name().c_str(), host2->name().c_str());
          for (auto link: *route)
            std::printf("<link_ctn id=\"%s\"/>",link->getName());
          std::printf("\n  </route>\n");
        }
        delete route;
      }
      xbt_lib_foreach(as_router_lib, cursor_dst, dst, value2){ //to router
        value2 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib,dst,ROUTING_ASR_LEVEL);
        if(value2->isRouter()){
          std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", host1->name().c_str(), dst);
          std::vector<Link*> *route = new std::vector<Link*>();
          routing_platf->getRouteAndLatency((sg_netcard_t)value1,(sg_netcard_t)value2,route,nullptr);
          for (auto link : *route)
            std::printf("<link_ctn id=\"%s\"/>",link->getName());
          delete route;
          std::printf("\n  </route>\n");
        }
      }
    }

    xbt_lib_foreach(as_router_lib, cursor_src, src, value1){ // Routes from router
      value1 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib,src,ROUTING_ASR_LEVEL);
      if (value1->isRouter()){
        xbt_lib_foreach(as_router_lib, cursor_dst, dst, value2){ //to router
          value2 = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib,dst,ROUTING_ASR_LEVEL);
          if(value2->isRouter()){
            std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", src, dst);
            std::vector<Link*> *route = new std::vector<Link*>();
            routing_platf->getRouteAndLatency((sg_netcard_t)value1,(sg_netcard_t)value2,route,nullptr);
            for(auto link :*route)
              std::printf("<link_ctn id=\"%s\"/>",link->getName());
            delete route;
            std::printf("\n  </route>\n");
          }
        }
        for (unsigned int it_dst = 0; it_dst < totalHosts; it_dst++) { // Routes to host
          host2 = hosts[it_dst];
          std::printf("  <route src=\"%s\" dst=\"%s\">\n  ",src, host2->name().c_str());
          std::vector<Link*> *route = new std::vector<Link*>();
          value2 = host2->pimpl_netcard;
          routing_platf->getRouteAndLatency((sg_netcard_t)value1,(sg_netcard_t)value2,route, nullptr);
          for(auto link : *route)
            std::printf("<link_ctn id=\"%s\"/>",link->getName());
          delete route;
          std::printf("\n  </route>\n");
        }
      }
    }

    std::printf("</AS>\n");
    std::printf("</platform>\n");
    std::free(hosts);
    std::free(links);
  }

  SD_exit();
  xbt_os_timer_free(parse_time);

  return 0;
}
