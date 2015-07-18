/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simgrid/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"
#include "simgrid/link.h"

/**
 * \brief Returns the link list
 *
 * Use SD_link_get_number() to know the array size.
 *
 * \return an array of \ref SD_link_t containing all links
 * \see SD_link_get_number()
 */
const SD_link_t *SD_link_get_list(void)
{

  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  int i;

  if (sd_global->link_list == NULL) {   /* this is the first time the function is called */
    sd_global->link_list = xbt_new(SD_link_t, xbt_lib_length(link_lib));

    i = 0;
    xbt_lib_foreach(link_lib, cursor, key, data) {
        sd_global->link_list[i++] = (SD_link_t) data[SD_LINK_LEVEL];
    }
  }
  return sd_global->link_list;
}

/**
 * \brief Returns the number of links
 *
 * \return the number of existing links
 * \see SD_link_get_list()
 */
int SD_link_get_number(void)
{
  return xbt_lib_length(link_lib);
}




