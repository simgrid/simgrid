
/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/surf.h"
#ifndef SURF_RESOURCE_H
#define SURF_RESOURCE_H

static XBT_INLINE
    surf_resource_t surf_resource_new(size_t childsize,
                                      surf_model_t model, const char *name,
                                      xbt_dict_t props, void_f_pvoid_t free_f)
{
  surf_resource_t res = xbt_malloc0(childsize);
  res->model = model;
  res->name = xbt_strdup(name);
  res->properties = props;
  res->free_f=free_f;
  return res;
}

static XBT_INLINE void surf_resource_free(void *r)
{
  surf_resource_t resource = r;
  if(resource->free_f)
    resource->free_f(r);
  free(resource->name);
  xbt_dict_free(&resource->properties);
  free(resource);
}

static XBT_INLINE const char *surf_resource_name(const void *resource)
{
  return ((surf_resource_t) resource)->name;
}

static XBT_INLINE xbt_dict_t surf_resource_properties(const void *resource)
{
  return ((surf_resource_t) resource)->properties;
}

#endif                          /* SURF_RESOURCE_H */
