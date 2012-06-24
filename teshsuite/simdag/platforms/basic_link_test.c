/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "surf/surf_private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic_link_test, sd,
                                "SimDag test basic_link_test");

int main(int argc, char **argv)
{
  int i;
  const char *user_data = "some user_data";
  const SD_link_t *links;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  XBT_INFO("Link number: %d", SD_link_get_number());
  links = SD_link_get_list();
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
