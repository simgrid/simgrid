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
#include "surf/surf.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(validator,
                             "Logging specific to this SimDag example");

static int name_compare_hosts(const void *n1, const void *n2)
{
  char name1[80], name2[80];
  strcpy(name1, SD_workstation_get_name(*((SD_workstation_t *) n1)));
  strcpy(name2, SD_workstation_get_name(*((SD_workstation_t *) n2)));

  return strcmp(name1, name2);
}

static int name_compare_links(const void *n1, const void *n2)
{
  char name1[80], name2[80];
  strcpy(name1, SD_link_get_name(*((SD_link_t *) n1)));
  strcpy(name2, SD_link_get_name(*((SD_link_t *) n2)));

  return strcmp(name1, name2);
}

int main(int argc, char **argv)
{
  char *platformFile = NULL;
  int totalHosts, totalLinks;
  unsigned int i;
  xbt_dict_t props = NULL;
  xbt_dict_cursor_t cursor = NULL;
  xbt_lib_cursor_t cursor_src = NULL;
  xbt_lib_cursor_t cursor_dst = NULL;
  char *src,*dst,*key,*data;
  char **value;
  xbt_ex_t e;

  const SD_workstation_t *hosts;
  const SD_link_t *links;

  setvbuf(stdout, NULL, _IOLBF, 0);

  SD_init(&argc, argv);

  platformFile = argv[1];
  XBT_DEBUG("%s", platformFile);
  TRY {
    SD_create_environment(platformFile);
  }
  CATCH(e) {
    xbt_die("Error while loading %s: %s",platformFile,e.msg);
  }

  printf("<?xml version='1.0'?>\n");
  printf("<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n");
  printf("<platform version=\"3\">\n");
  printf("<AS id=\"AS0\" routing=\"Full\">\n");

  // Hosts
  totalHosts = SD_workstation_get_number();
  hosts = SD_workstation_get_list();
  qsort((void *) hosts, totalHosts, sizeof(SD_workstation_t),
        name_compare_hosts);

  for (i = 0; i < totalHosts; i++) {
    printf("  <host id=\"%s\" power=\"%.0f\"",
           SD_workstation_get_name(hosts[i]),
           SD_workstation_get_power(hosts[i]));
    props = SD_workstation_get_properties(hosts[i]);
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
  xbt_lib_foreach(as_router_lib, cursor_src, key, value) {
	  if(((network_element_info_t)xbt_lib_get_or_null(as_router_lib, key,
			  ROUTING_ASR_LEVEL))->rc_type == SURF_NETWORK_ELEMENT_ROUTER)
	  {
		  printf("  <router id=\"%s\"/>\n",key);
	  }
  }

  // Links
  totalLinks = SD_link_get_number();
  links = SD_link_get_list();

  qsort((void *) links, totalLinks, sizeof(SD_link_t), name_compare_links);

  for (i = 0; i < totalLinks; i++) {
    printf("  <link id=\"");

    printf("%s\" bandwidth=\"%.0f\" latency=\"%.9f\"",
           SD_link_get_name(links[i]),
           SD_link_get_current_bandwidth(links[i]),
           SD_link_get_current_latency(links[i]));
    if (SD_link_get_sharing_policy(links[i]) == SD_LINK_SHARED) {
      printf("/>\n");
    } else {
      printf(" sharing_policy=\"FATPIPE\"/>\n");
    }
  }


  xbt_lib_foreach(host_lib, cursor_src, src, value) // Routes from host
  {
		  xbt_lib_foreach(host_lib, cursor_dst, dst, value) //to host
		  {
				printf("  <route src=\"%s\" dst=\"%s\">\n  "
					  ,src
					  ,dst);
				xbt_dynar_t route=NULL;
				routing_get_route_and_latency(src,dst,&route,NULL);
				for(i=0;i<xbt_dynar_length(route) ;i++)
				{
					void *link = xbt_dynar_get_as(route,i,void *);

					char *link_name = xbt_strdup(((surf_resource_t)link)->name);
					printf("<link_ctn id=\"%s\"/>",link_name);
					free(link_name);
				}
				printf("\n  </route>\n");
		  }
		  xbt_lib_foreach(as_router_lib, cursor_dst, dst, value) //to router
		  {
			    if(routing_get_network_element_type(dst) == SURF_NETWORK_ELEMENT_ROUTER){
				printf("  <route src=\"%s\" dst=\"%s\">\n  "
					  ,src
					  ,dst);
				xbt_dynar_t route=NULL;
				routing_get_route_and_latency(src,dst,&route,NULL);
				for(i=0;i<xbt_dynar_length(route) ;i++)
				{
					void *link = xbt_dynar_get_as(route,i,void *);

					char *link_name = xbt_strdup(((surf_resource_t)link)->name);
					printf("<link_ctn id=\"%s\"/>",link_name);
					free(link_name);
				}
				printf("\n  </route>\n");
			    }
		  }
  }

  xbt_lib_foreach(as_router_lib, cursor_src, src, value) // Routes from router
  {
	  if(routing_get_network_element_type(src) == SURF_NETWORK_ELEMENT_ROUTER){
		  xbt_lib_foreach(as_router_lib, cursor_dst, dst, value) //to router
		  {
				if(routing_get_network_element_type(dst) == SURF_NETWORK_ELEMENT_ROUTER){
				printf("  <route src=\"%s\" dst=\"%s\">\n  "
					  ,src
					  ,dst);
				xbt_dynar_t route=NULL;
				routing_get_route_and_latency(src,dst,&route,NULL);
				for(i=0;i<xbt_dynar_length(route) ;i++)
				{
					void *link = xbt_dynar_get_as(route,i,void *);

					char *link_name = xbt_strdup(((surf_resource_t)link)->name);
					printf("<link_ctn id=\"%s\"/>",link_name);
					free(link_name);
				}
				printf("\n  </route>\n");
				}
		  }
		  xbt_lib_foreach(host_lib, cursor_dst, dst, value) //to host
		  {
				printf("  <route src=\"%s\" dst=\"%s\">\n  "
					  ,src, dst);
				xbt_dynar_t route=NULL;
				routing_get_route_and_latency(src,dst,&route, NULL);
				for(i=0;i<xbt_dynar_length(route) ;i++)
				{
					void *link = xbt_dynar_get_as(route,i,void *);

					char *link_name = xbt_strdup(((surf_resource_t)link)->name);
					printf("<link_ctn id=\"%s\"/>",link_name);
					free(link_name);
				}
				printf("\n  </route>\n");
		  }
	  }
  }

  printf("</AS>\n");
  printf("</platform>\n");
  SD_exit();

  return 0;
}
