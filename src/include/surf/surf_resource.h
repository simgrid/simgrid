
/* Copyright (c) 2009 The SimGrid Team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/surf.h"
#ifndef SURF_RESOURCE_H
#define SURF_RESOURCE_H

static XBT_INLINE void surf_resource_free(void* r) {
  surf_resource_t resource = r;
  if (resource->name)
    free(resource->name);
  if (resource->properties)
    xbt_dict_free(&resource->properties);
  free(resource);
}

static XBT_INLINE const char *surf_resource_name(const void *resource) {
  return ((surf_resource_t)resource)->name;
}

static XBT_INLINE xbt_dict_t surf_resource_properties(const void *resource) {
  return ((surf_resource_t)resource)->properties;
}

#endif /* SURF_RESOURCE_H */
