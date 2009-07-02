
/* Copyright (c) 2009 The SimGrid Team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "xbt/dict.h"

void surf_resource_free(void* r) {
  surf_resource_t resource = r;
  if (resource->name)
    free(resource->name);
  if (resource->properties)
    xbt_dict_free(&resource->properties);
  free(resource);
}

const char *surf_resource_name(const void *resource) {
  return ((surf_resource_t)resource)->name;
}

xbt_dict_t surf_resource_properties(const void *resource) {
  return ((surf_resource_t)resource)->properties;
}
