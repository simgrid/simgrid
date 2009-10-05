/* Copyright (c) 2007-2009 Da SimGrid Team.  All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"

/* Creates a link and registers it in SD.
 */
SD_link_t __SD_link_create(void *surf_link, void *data)
{

  SD_link_t link;
  const char *name;

  SD_CHECK_INIT_DONE();
  xbt_assert0(surf_link != NULL, "surf_link is NULL !");

  link = xbt_new(s_SD_link_t, 1);
  link->surf_link = surf_link;
  link->data = data;            /* user data */
  if (surf_workstation_model->extension.workstation.link_shared(surf_link))
    link->sharing_policy = SD_LINK_SHARED;
  else
    link->sharing_policy = SD_LINK_FATPIPE;

  name = SD_link_get_name(link);
  xbt_dict_set(sd_global->links, name, link, __SD_link_destroy);        /* add the link to the dictionary */
  sd_global->link_count++;

  return link;
}

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

  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int i;

  SD_CHECK_INIT_DONE();
  xbt_assert0(SD_link_get_number() > 0, "There is no link!");

  if (sd_global->link_list == NULL) {   /* this is the first time the function is called */
    sd_global->link_list = xbt_new(SD_link_t, sd_global->link_count);

    i = 0;
    xbt_dict_foreach(sd_global->links, cursor, key, data) {
      sd_global->link_list[i++] = (SD_link_t) data;
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
  SD_CHECK_INIT_DONE();
  return sd_global->link_count;
}

/**
 * \brief Returns the user data of a link
 *
 * \param link a link
 * \return the user data associated with this link (can be \c NULL)
 * \see SD_link_set_data()
 */
void *SD_link_get_data(SD_link_t link)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return link->data;
}

/**
 * \brief Sets the user data of a link
 *
 * The new data can be \c NULL. The old data should have been freed first
 * if it was not \c NULL.
 *
 * \param link a link
 * \param data the new data you want to associate with this link
 * \see SD_link_get_data()
 */
void SD_link_set_data(SD_link_t link, void *data)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  link->data = data;
}

/**
 * \brief Returns the name of a link
 *
 * \param link a link
 * \return the name of this link (cannot be \c NULL)
 */
const char *SD_link_get_name(SD_link_t link)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return surf_resource_name(link->surf_link);
}

/**
 * \brief Returns the current bandwidth of a link
 *
 * \param link a link
 * \return the current bandwidth of this link, in bytes per second
 */
double SD_link_get_current_bandwidth(SD_link_t link)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return surf_workstation_model->extension.workstation.
    get_link_bandwidth(link->surf_link);
}

/**
 * \brief Returns the current latency of a link
 *
 * \param link a link
 * \return the current latency of this link, in seconds
 */
double SD_link_get_current_latency(SD_link_t link)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return surf_workstation_model->extension.workstation.
    get_link_latency(link->surf_link);
}

/**
 * \brief Returns the sharing policy of this workstation.
 *
 * \param link a link
 * \return the sharing policyfor the flows going through this link:
 * SD_LINK_SHARED or SD_LINK_FATPIPE
 *
 */
e_SD_link_sharing_policy_t SD_link_get_sharing_policy(SD_link_t link)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  return link->sharing_policy;
}


/* Destroys a link.
 */
void __SD_link_destroy(void *link)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(link != NULL, "Invalid parameter");
  /* link->surf_link is freed by surf_exit and link->data is freed by the user */
  xbt_free(link);
}
