/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;
extern routing_component_t current_routing;
extern model_type_t current_routing_model;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

/* Routing model structure */
typedef struct {
  s_routing_component_t generic_routing;
} s_routing_component_none_t, *routing_component_none_t;

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

static void none_set_processing_unit(routing_component_t rc,
                                     const char *name)
{
}

static void none_set_autonomous_system(routing_component_t rc,
                                       const char *name)
{
}

/* Creation routing model functions */
routing_component_t model_none_create(void)
{
  routing_component_none_t new_component =
      xbt_new0(s_routing_component_none_t, 1);
  new_component->generic_routing.set_processing_unit =
      none_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      none_set_autonomous_system;
  new_component->generic_routing.set_route = NULL;
  new_component->generic_routing.set_ASroute = NULL;
  new_component->generic_routing.set_bypassroute = NULL;
  new_component->generic_routing.get_route = none_get_route;
  new_component->generic_routing.get_onelink_routes =
      none_get_onelink_routes;
  new_component->generic_routing.get_bypass_route = none_get_bypass_route;
  new_component->generic_routing.finalize = none_finalize;
  return (routing_component_t) new_component;
}

void model_none_load(void)
{
}

void model_none_unload(void)
{
}

void model_none_end(void)
{
}
