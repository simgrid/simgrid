/* Copyright (c) 2008-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic_link_test, sd, "SimDag test basic_link_test");

static int cmp_link(const void*a, const void*b) {
  return strcmp(sg_link_name(*(SD_link_t*)a)  , sg_link_name(*(SD_link_t*)b));
}

int main(int argc, char **argv)
{
  const char *user_data = "some user_data";

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  const SD_link_t *links = sg_link_list();
  int count = sg_link_count();
  XBT_INFO("Link count: %d", count);
  qsort((void *)links, count, sizeof(SD_link_t), cmp_link);
   
  for (int i=0; i < count; i++){
    XBT_INFO("%s: latency = %.5f, bandwidth = %f", sg_link_name(links[i]),
             sg_link_latency(links[i]), sg_link_bandwidth(links[i]));
    sg_link_data_set(links[i], (void*) user_data);
    xbt_assert(!strcmp(user_data, (const char*)sg_link_data(links[i])),"User data was corrupted.");
  }

  SD_exit();
  return 0;
}
