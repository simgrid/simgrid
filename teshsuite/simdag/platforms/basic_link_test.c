/* Copyright (c) 2008-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simgrid/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic_link_test, sd,
                                "SimDag test basic_link_test");

static int cmp_link(const void*a, const void*b) {
	const char *nameA = SD_link_get_name(*(SD_link_t*)a);
	const char *nameB = SD_link_get_name(*(SD_link_t*)b);
  return strcmp( nameA, nameB );
}

int main(int argc, char **argv)
{
  int i;
  const char *user_data = "some user_data";
  const SD_link_t *links;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  links = SD_link_get_list();
  int count = SD_link_get_number();
  XBT_INFO("Link count: %d", count);
  qsort((void *)links, count, sizeof(SD_link_t), cmp_link);
   
  for(i=0; i<SD_link_get_number();i++){
    XBT_INFO("%s: latency = %.5f, bandwidth = %f",
             SD_link_get_name(links[i]),
             SD_link_get_current_latency(links[i]),
             SD_link_get_current_bandwidth(links[i]));
    SD_link_set_data(links[i], (void*) user_data);
    if(strcmp(user_data, (const char*)SD_link_get_data(links[i]))){
      XBT_ERROR("User data was corrupted.");
    }
  }

  SD_exit();
  return 0;
}
