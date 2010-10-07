/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "surf/surf_private.h"

extern routing_global_t global_routing;

int main(int argc, char **argv)
{
  /* initialisation of SD */
  int size;
  xbt_dict_t eltms = xbt_dict_new();
  SD_init(&argc, argv);
  xbt_dict_cursor_t cursor=NULL;
   char *key,*data;

  /* creation of the environment */
  SD_create_environment(argv[1]);

  eltms = global_routing->where_network_elements;
  size = xbt_dict_size(eltms);

  printf("Workstation number: %d, link number: %d, elmts number: %d\n",SD_workstation_get_number(), SD_link_get_number(),size);

  xbt_dict_foreach(eltms,cursor,key,data) {
     printf("   - Seen: \"%s\" is type : %d\n",key,(int) global_routing->get_network_element_type(key));
  }
  //printf("%s is router : %d\n",name1,global_routing->is_router(name1));
  xbt_dict_free(&eltms);
  SD_exit();
  return 0;
}
