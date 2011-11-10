/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

/* Routing model structure */
/* Business methods */
static xbt_dynar_t none_get_onelink_routes(AS_t rc)
{
  return NULL;
}

static route_extended_t none_get_route(AS_t rc,
                                       const char *src, const char *dst)
{
  return NULL;
}

static route_extended_t none_get_bypass_route(AS_t rc,
                                              const char *src,
                                              const char *dst)
{
  return NULL;
}

void model_none_finalize(AS_t as) {
  xbt_free(as);
}

static void none_parse_PU(AS_t rc,
                                     const char *name)
{
}

static void none_parse_AS(AS_t rc,
                                       const char *name)
{
}

/* Creation routing model functions */
AS_t model_none_create() {
  return model_none_create_sized(sizeof(s_as_t));
}
AS_t model_none_create_sized(size_t childsize) {
  AS_t new_component = xbt_malloc0(childsize);
  new_component->parse_PU = none_parse_PU;
  new_component->parse_AS = none_parse_AS;
  new_component->parse_route = NULL;
  new_component->parse_ASroute = NULL;
  new_component->parse_bypassroute = NULL;
  new_component->get_route = none_get_route;
  new_component->get_onelink_routes = none_get_onelink_routes;
  new_component->get_bypass_route = none_get_bypass_route;
  new_component->finalize = model_none_finalize;
  return new_component;
}

