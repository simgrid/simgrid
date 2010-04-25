/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef WIN32
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
  int totalHosts, totalLinks, tmp_length;
  int i, j, k;
  xbt_dict_t props = NULL;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;

  const SD_workstation_t *hosts;
  const SD_link_t *links, *tmp;

  SD_init(&argc, argv);

  platformFile = argv[1];
  DEBUG1("%s", platformFile);
  SD_create_environment(platformFile);

  printf("<?xml version='1.0'?>\n");
  printf("<!DOCTYPE platform SYSTEM \"simgrid.dtd\">\n");
  printf("<platform version=\"2\">\n");

  totalHosts = SD_workstation_get_number();
  hosts = SD_workstation_get_list();
  qsort((void *) hosts, totalHosts, sizeof(SD_workstation_t),
        name_compare_hosts);

  for (i = 0; i < totalHosts; i++) {
    printf("  <host id=\"%s\" power=\"%.0f\"",
           SD_workstation_get_name(hosts[i]),
           SD_workstation_get_power(hosts[i]));
    props = SD_workstation_get_properties(hosts[i]);
    if (xbt_dict_length(props) > 0) {
      printf(">\n");
      xbt_dict_foreach(props, cursor, key, data) {
        printf("    <prop id=\"%s\" value=\"%s\"/>\n", key, data);
      }
      printf("  </host>\n");
    } else {
      printf("/>\n");
    }
  }

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

  for (i = 0; i < totalHosts; i++) {
    for (j = 0; j < totalHosts; j++) {
      tmp = SD_route_get_list(hosts[i], hosts[j]);
      if (tmp) {
        printf("  <route src=\"%s\" dst=\"%s\">\n    ",
               SD_workstation_get_name(hosts[i]),
               SD_workstation_get_name(hosts[j]));

        tmp_length = SD_route_get_size(hosts[i], hosts[j]);
        for (k = 0; k < tmp_length; k++) {
          printf("<link:ctn id=\"%s\"/>", SD_link_get_name(tmp[k]));
        }
        printf("\n  </route>\n");
      }
    }
  }
  printf("</platform>\n");
  SD_exit();

  return 0;
}
