/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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
/* ******************************* NO ROUTING ******************************* */
/* Only save the AS tree, and forward calls to child ASes */
AS_t model_none_create(void);
AS_t model_none_create_sized(size_t childsize);
void model_none_finalize(AS_t as);
/* ************************************************************************** */
/* ***************** GENERIC PARSE FUNCTIONS (declarations) ***************** */
AS_t model_generic_create_sized(size_t childsize);
void model_generic_finalize(AS_t as);

int generic_parse_PU(AS_t rc, network_element_t elm);
int generic_parse_AS(AS_t rc, network_element_t elm);
void generic_parse_bypassroute(AS_t rc, const char *src, const char *dst,
                               route_t e_route);

/* ************************************************************************** */
/* *************** GENERIC BUSINESS METHODS (declarations) ****************** */

xbt_dynar_t generic_get_onelink_routes(AS_t rc);
route_t generic_get_bypassroute(AS_t rc,
    network_element_t src,
    network_element_t dst,
    double *lat);

/* ************************************************************************** */
/* ****************** GENERIC AUX FUNCTIONS (declarations) ****************** */

route_t
generic_new_extended_route(e_surf_routing_hierarchy_t hierarchy,
                           route_t data, int order);
AS_t
generic_autonomous_system_exist(AS_t rc, char *element);
AS_t
generic_processing_units_exist(AS_t rc, char *element);
void generic_src_dst_check(AS_t rc, network_element_t src,
    network_element_t dst);


/* ************************************************************************** */
/* *************************** FLOYD ROUTING ******************************** */
AS_t model_floyd_create(void);  /* create structures for floyd routing model */
void model_floyd_end(AS_t as);      /* finalize the creation of floyd routing model */
void model_floyd_parse_route(AS_t rc, const char *src,
        const char *dst, route_t route);

/* ************************************************** */
/* ************** RULE-BASED ROUTING **************** */
AS_t model_rulebased_create(void);      /* create structures for rulebased routing model */

/* ************************************************** */
/* **************  Cluster ROUTING   **************** */
AS_t model_cluster_create(void);      /* create structures for cluster routing model */

/* Pass info from the cluster parser to the cluster routing */
void surf_routing_cluster_add_backbone(AS_t as, void* bb);

/* ************************************************** */
/* **************  Vivaldi ROUTING   **************** */
AS_t model_vivaldi_create(void);      /* create structures for vivaldi routing model */
#define HOST_PEER(peername) bprintf("peer_%s", peername)
#define ROUTER_PEER(peername) bprintf("router_%s", peername)
#define LINK_UP_PEER(peername) bprintf("link_%s_up", peername)
#define LINK_DOWN_PEER(peername) bprintf("link_%s_down", peername)

/* ************************************************************************** */
/* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */
AS_t model_dijkstra_both_create(int cached);    /* create by calling dijkstra or dijkstracache */
AS_t model_dijkstra_create(void);       /* create structures for dijkstra routing model */
AS_t model_dijkstracache_create(void);  /* create structures for dijkstracache routing model */
void model_dijkstra_both_end(AS_t as);      /* finalize the creation of dijkstra routing model */
void model_dijkstra_both_parse_route (AS_t rc, const char *src,
                     const char *dst, route_t route);

/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */
AS_t model_full_create(void);   /* create structures for full routing model */
void model_full_end(AS_t as);       /* finalize the creation of full routing model */
void model_full_set_route(	/* Set the route and ASroute between src and dst */
		AS_t rc, const char *src, const char *dst, route_t route);


#endif                          /* _SURF_SURF_ROUTING_PRIVATE_H */
