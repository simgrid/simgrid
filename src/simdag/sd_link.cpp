/* Copyright (c) 2006-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simdag/simdag_private.h"
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
  if (sd_global->link_list == NULL)    /* this is the first time the function is called */
    sd_global->link_list = sg_link_list();

  return sd_global->link_list;
}




