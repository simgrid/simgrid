/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;
extern routing_component_t current_routing;
extern routing_model_description_t current_routing_model;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

/* Routing model structure */
/* Business methods */
static xbt_dynar_t none_get_onelink_routes(routing_component_t rc)
{
  return NULL;
}

static route_extended_t none_get_route(routing_component_t rc,
                                       const char *src, const char *dst)
{
  return NULL;
}

static route_extended_t none_get_bypass_route(routing_component_t rc,
                                              const char *src,
                                              const char *dst)
{
  return NULL;
}

static void none_finalize(routing_component_t rc)
{
  xbt_free(rc);
}

static void none_parse_PU(routing_component_t rc,
                                     const char *name)
{
}

static void none_parse_AS(routing_component_t rc,
                                       const char *name)
{
}

/* Creation routing model functions */
routing_component_t model_none_create(void)
{
  routing_component_t new_component = xbt_new(s_routing_component_t, 1);
  new_component->parse_PU = none_parse_PU;
  new_component->parse_AS = none_parse_AS;
  new_component->parse_route = NULL;
  new_component->parse_ASroute = NULL;
  new_component->parse_bypassroute = NULL;
  new_component->get_route = none_get_route;
  new_component->get_onelink_routes = none_get_onelink_routes;
  new_component->get_bypass_route = none_get_bypass_route;
  new_component->finalize = none_finalize;
  return new_component;
}

