/*
 * surf_routing_private.h
 *
 *  Created on: 14 avr. 2011
 *      Author: navarrop
 */

#ifndef _SURF_SURF_ROUTING_PRIVATE_H
#define _SURF_SURF_ROUTING_PRIVATE_H

#include <float.h>
#include "gras_config.h"

#include "surf_private.h"
#include "xbt/dynar.h"
#include "xbt/str.h"
#include "xbt/config.h"
#include "xbt/graph.h"
#include "xbt/set.h"
#include "surf/surfxml_parse.h"

/* ************************************************************************** */
/* ***************** GENERIC PARSE FUNCTIONS (declarations) ***************** */

void generic_set_processing_unit(routing_component_t rc,
                                        const char *name);
void generic_set_autonomous_system(routing_component_t rc,
                                          const char *name);
void generic_set_bypassroute(routing_component_t rc,
                                    const char *src, const char *dst,
                                    route_extended_t e_route);

int surf_link_resource_cmp(const void *a, const void *b);
int surf_pointer_resource_cmp(const void *a, const void *b);

/* ************************************************************************** */
/* *************** GENERIC BUSINESS METHODS (declarations) ****************** */

double generic_get_link_latency(routing_component_t rc, const char *src, const char *dst,
										route_extended_t e_route);
xbt_dynar_t generic_get_onelink_routes(routing_component_t rc);
route_extended_t generic_get_bypassroute(routing_component_t rc,
                                                const char *src,
                                                const char *dst);

/* ************************************************************************** */
/* ****************** GENERIC AUX FUNCTIONS (declarations) ****************** */

route_extended_t
generic_new_extended_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order);
route_t
generic_new_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order);
void generic_free_route(route_t route);
void generic_free_extended_route(route_extended_t e_route);
routing_component_t
generic_autonomous_system_exist(routing_component_t rc, char *element);
routing_component_t
generic_processing_units_exist(routing_component_t rc, char *element);
void generic_src_dst_check(routing_component_t rc, const char *src,
                                  const char *dst);


/* ************************************************************************** */
/* *************************** FLOYD ROUTING ******************************** */
void *model_floyd_create(void);  /* create structures for floyd routing model */
void model_floyd_load(void);     /* load parse functions for floyd routing model */
void model_floyd_unload(void);   /* unload parse functions for floyd routing model */
void model_floyd_end(void);      /* finalize the creation of floyd routing model */
void model_floyd_set_route(routing_component_t rc, const char *src,
        const char *dst, name_route_extended_t route);

/* ************************************************** */
/* ************** RULE-BASED ROUTING **************** */
void *model_rulebased_create(void);      /* create structures for rulebased routing model */
void model_rulebased_load(void);         /* load parse functions for rulebased routing model */
void model_rulebased_unload(void);       /* unload parse functions for rulebased routing model */
void model_rulebased_end(void);          /* finalize the creation of rulebased routing model */

/* ************************************************************************** */
/* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */
void *model_dijkstra_both_create(int cached);    /* create by calling dijkstra or dijkstracache */
void *model_dijkstra_create(void);       /* create structures for dijkstra routing model */
void *model_dijkstracache_create(void);  /* create structures for dijkstracache routing model */
void model_dijkstra_both_load(void);     /* load parse functions for dijkstra routing model */
void model_dijkstra_both_unload(void);   /* unload parse functions for dijkstra routing model */
void model_dijkstra_both_end(void);      /* finalize the creation of dijkstra routing model */
void model_dijkstra_both_set_route (routing_component_t rc, const char *src,
                     const char *dst, name_route_extended_t route);

/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */
void *model_full_create(void);   /* create structures for full routing model */
void model_full_load(void);      /* load parse functions for full routing model */
void model_full_unload(void);    /* unload parse functions for full routing model */
void model_full_end(void);       /* finalize the creation of full routing model */
void model_full_set_route(	/* Set the route and ASroute between src and dst */
		routing_component_t rc, const char *src, const char *dst, name_route_extended_t route);

/* ************************************************************************** */
/* ******************************* NO ROUTING ******************************* */
void *model_none_create(void);           /* none routing model */
void model_none_load(void);              /* none routing model */
void model_none_unload(void);            /* none routing model */
void model_none_end(void);               /* none routing model */

#endif                          /* _SURF_SURF_ROUTING_PRIVATE_H */
