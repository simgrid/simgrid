/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"
#include <pcre.h>               /* regular expression library */

/* Global vars */
extern routing_global_t global_routing;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_rulebased, surf, "Routing part of surf");

/* Routing model structure */

typedef struct {
  s_as_t generic_routing;
  xbt_dynar_t list_route;
  xbt_dynar_t list_ASroute;
} s_AS_rulebased_t, *AS_rulebased_t;

typedef struct s_rule_route s_rule_route_t, *rule_route_t;
typedef struct s_rule_route_extended s_rule_route_extended_t,
    *rule_route_extended_t;

struct s_rule_route {
  xbt_dynar_t re_str_link;      // dynar of char*
  pcre *re_src;
  pcre *re_dst;
};

struct s_rule_route_extended {
  s_rule_route_t generic_rule_route;
  char *re_src_gateway;
  char *re_dst_gateway;
};

static void rule_route_free(void *e)
{
  rule_route_t *elem = (rule_route_t *) (e);
  if (*elem) {
    xbt_dynar_free(&(*elem)->re_str_link);
    pcre_free((*elem)->re_src);
    pcre_free((*elem)->re_dst);
    xbt_free(*elem);
  }
  *elem = NULL;
}

static void rule_route_extended_free(void *e)
{
  rule_route_extended_t *elem = (rule_route_extended_t *) e;
  if (*elem) {
    xbt_dynar_free(&(*elem)->generic_rule_route.re_str_link);
    pcre_free((*elem)->generic_rule_route.re_src);
    pcre_free((*elem)->generic_rule_route.re_dst);
    xbt_free((*elem)->re_src_gateway);
    xbt_free((*elem)->re_dst_gateway);
    xbt_free(*elem);
  }
}

/* Parse routing model functions */

static int model_rulebased_parse_PU(AS_t rc, network_element_t elm)
{
  AS_rulebased_t routing = (AS_rulebased_t) rc;
  xbt_dynar_push(routing->generic_routing.index_network_elm,(void *)elm);
  return -1;
}

static int model_rulebased_parse_AS(AS_t rc, network_element_t elm)
{
  AS_rulebased_t routing = (AS_rulebased_t) rc;
  xbt_dynar_push(routing->generic_routing.index_network_elm,(void *)elm);
  return -1;
}

static void model_rulebased_parse_route(AS_t rc,
                                      const char *src, const char *dst,
                                      route_t route)
{
  AS_rulebased_t routing = (AS_rulebased_t) rc;
  rule_route_t ruleroute = xbt_new0(s_rule_route_t, 1);
  const char *error;
  int erroffset;

  if(!strcmp(rc->model_desc->name,"Vivaldi")){
	  if(!xbt_dynar_is_empty(route->link_list))
		  xbt_die("You can't have link_ctn with Model Vivaldi.");
  }

  ruleroute->re_src = pcre_compile(src, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute->re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, src, error);
  ruleroute->re_dst = pcre_compile(dst, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute->re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, dst, error);

  ruleroute->re_str_link = route->link_list;
  route->link_list = NULL; // Don't free it twice in each container

  xbt_dynar_push(routing->list_route, &ruleroute);
}

static void model_rulebased_parse_ASroute(AS_t rc,
                                        const char *src, const char *dst,
                                        route_t route)
{
  AS_rulebased_t routing = (AS_rulebased_t) rc;
  rule_route_extended_t ruleroute_e = xbt_new0(s_rule_route_extended_t, 1);
  const char *error;
  int erroffset;

  if(!strcmp(rc->model_desc->name,"Vivaldi")){
	  if(!xbt_dynar_is_empty(route->link_list))
		  xbt_die("You can't have link_ctn with Model Vivaldi.");
  }

  ruleroute_e->generic_rule_route.re_src =
      pcre_compile(src, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute_e->generic_rule_route.re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, src, error);
  ruleroute_e->generic_rule_route.re_dst =
      pcre_compile(dst, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute_e->generic_rule_route.re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, dst, error);
  ruleroute_e->generic_rule_route.re_str_link =
      route->link_list;
  ruleroute_e->re_src_gateway = xbt_strdup((char *)route->src_gateway); // DIRTY HACK possible only
  ruleroute_e->re_dst_gateway = xbt_strdup((char *)route->dst_gateway); // because of what is in routing_parse_E_ASroute
  xbt_dynar_push(routing->list_ASroute, &ruleroute_e);

  /* make sure that they don't get freed */
  route->link_list = NULL;
  route->src_gateway = route->dst_gateway = NULL;
}

static void model_rulebased_parse_bypassroute(AS_t rc,
                                            const char *src,
                                            const char *dst,
                                            route_t e_route)
{
  xbt_die("bypass routing not supported for Route-Based model");
}

#define BUFFER_SIZE 4096        /* result buffer size */
#define OVECCOUNT 30            /* should be a multiple of 3 */

static char *remplace(char *value, const char **src_list, int src_size,
                      const char **dst_list, int dst_size)
{
  char result[BUFFER_SIZE];
  int i_res = 0;
  int i = 0;

  while (value[i]) {
    if (value[i] == '$') {
      i++;                      /* skip the '$' */
      if (value[i] < '0' || value[i] > '9')
        xbt_die("bad string parameter, no number indication, at offset: "
                "%d (\"%s\")", i, value);

      /* solve the number */
      int number = value[i++] - '0';
      while (value[i] >= '0' && value[i] <= '9')
        number = 10 * number + (value[i++] - '0');

      /* solve the indication */
      const char **param_list;
      _XBT_GNUC_UNUSED int param_size;
      if (value[i] == 's' && value[i + 1] == 'r' && value[i + 2] == 'c') {
        param_list = src_list;
        param_size = src_size;
      } else if (value[i] == 'd' && value[i + 1] == 's'
                 && value[i + 2] == 't') {
        param_list = dst_list;
        param_size = dst_size;
      } else {
        xbt_die("bad string parameter, support only \"src\" and \"dst\", "
                "at offset: %d (\"%s\")", i, value);
      }
      i += 3;

      xbt_assert(number < param_size,
                 "bad string parameter, not enough length param_size, "
                 "at offset: %d (\"%s\") %d %d", i, value, param_size, number);

      const char *param = param_list[number];
      int j = 0;
      while (param[j] && i_res < BUFFER_SIZE)
        result[i_res++] = param[j++];
    } else {
      result[i_res++] = value[i++]; /* next char */
    }
    if (i_res >= BUFFER_SIZE)
      xbt_die("solving string \"%s\", small buffer size (%d)",
              value, BUFFER_SIZE);
  }
  result[i_res++] = '\0';
  char *res = xbt_malloc(i_res);
  return memcpy(res, result, i_res);
}

static void rulebased_get_route_and_latency(AS_t rc,
    network_element_t src, network_element_t dst,
                                route_t res,double*lat);
static xbt_dynar_t rulebased_get_onelink_routes(AS_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new (sizeof(onelink_t), xbt_free);
  //We have already bypass cluster routes with network NS3
  if(!strcmp(surf_network_model->name,"network NS3"))
	return ret;

  char *k1;

  //find router
  network_element_t router = NULL;
  xbt_lib_cursor_t cursor;
  xbt_lib_foreach(as_router_lib, cursor, k1, router)
  {
    if (router->rc_type == SURF_NETWORK_ELEMENT_ROUTER)
      break;
  }

  if (!router)
    xbt_die ("rulebased_get_onelink_routes works only if the AS is a cluster, sorry.");

  network_element_t host = NULL;
  xbt_lib_foreach(as_router_lib, cursor, k1, host){
    route_t route = xbt_new0(s_route_t,1);
    route->link_list = xbt_dynar_new(global_routing->size_of_link,NULL);
    rulebased_get_route_and_latency (rc, router, host, route,NULL);

    int number_of_links = xbt_dynar_length(route->link_list);

    if(number_of_links == 1) {
		//loopback
    }
    else{
		if (number_of_links != 2) {
		  xbt_die ("rulebased_get_onelink_routes works only if the AS is a cluster, sorry.");
		}

		void *link_ptr;
		xbt_dynar_get_cpy (route->link_list, 1, &link_ptr);
		onelink_t onelink = xbt_new0 (s_onelink_t, 1);
		onelink->src = host;
		onelink->dst = router;
		onelink->link_ptr = link_ptr;
		xbt_dynar_push (ret, &onelink);
    }
  }
  return ret;
}

/* Business methods */
static void rulebased_get_route_and_latency(AS_t rc,
    network_element_t src, network_element_t dst,
                                route_t route, double *lat)
{
  XBT_DEBUG("rulebased_get_route_and_latency from '%s' to '%s'",src->name,dst->name);
  xbt_assert(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  AS_rulebased_t routing = (AS_rulebased_t) rc;

  char* src_name = src->name;
  char* dst_name = dst->name;

  int are_processing_units=0;
  xbt_dynar_t rule_list;
  if ((src->rc_type == SURF_NETWORK_ELEMENT_HOST || src->rc_type == SURF_NETWORK_ELEMENT_ROUTER)&&
      (dst->rc_type == SURF_NETWORK_ELEMENT_HOST || dst->rc_type == SURF_NETWORK_ELEMENT_ROUTER)){
    are_processing_units = 1;
    rule_list = routing->list_route;
  } else if (src->rc_type == SURF_NETWORK_ELEMENT_AS && dst->rc_type == SURF_NETWORK_ELEMENT_AS) {
    are_processing_units = 0;
    rule_list = routing->list_ASroute;
  } else
    THROWF(arg_error,0,"No route from '%s' to '%s'",src_name,dst_name);

  int rc_src = -1;
  int rc_dst = -1;
  int src_length = (int) strlen(src_name);
  int dst_length = (int) strlen(dst_name);

  rule_route_t ruleroute;
  unsigned int cpt;
  int ovector_src[OVECCOUNT];
  int ovector_dst[OVECCOUNT];
  const char **list_src = NULL;
  const char **list_dst = NULL;
  _XBT_GNUC_UNUSED int res;
  xbt_dynar_foreach(rule_list, cpt, ruleroute) {
    rc_src =
        pcre_exec(ruleroute->re_src, NULL, src_name, src_length, 0, 0,
                  ovector_src, OVECCOUNT);
    if (rc_src >= 0) {
      rc_dst =
          pcre_exec(ruleroute->re_dst, NULL, dst_name, dst_length, 0, 0,
                    ovector_dst, OVECCOUNT);
      if (rc_dst >= 0) {
        res = pcre_get_substring_list(src_name, ovector_src, rc_src, &list_src);
        xbt_assert(!res, "error solving substring list for src \"%s\"", src_name);
        res = pcre_get_substring_list(dst_name, ovector_dst, rc_dst, &list_dst);
        xbt_assert(!res, "error solving substring list for dst \"%s\"", dst_name);
        char *link_name;
        xbt_dynar_foreach(ruleroute->re_str_link, cpt, link_name) {
          char *new_link_name =
              remplace(link_name, list_src, rc_src, list_dst, rc_dst);
          void *link =
        		  xbt_lib_get_or_null(link_lib, new_link_name, SURF_LINK_LEVEL);
          if (link) {
            xbt_dynar_push(route->link_list, &link);
            if (lat)
              *lat += surf_network_model->extension.network.get_link_latency(link);
          } else
            THROWF(mismatch_error, 0, "Link %s not found", new_link_name);
          xbt_free(new_link_name);
        }
      }
    }
    if (rc_src >= 0 && rc_dst >= 0)
      break;
  }

  if (rc_src >= 0 && rc_dst >= 0) {
    /* matched src and dest, nothing more to do (?) */
  } else if (!strcmp(src_name, dst_name) && are_processing_units) {
    xbt_dynar_push(route->link_list, &(global_routing->loopback));
    if (lat)
      *lat += surf_network_model->extension.network.get_link_latency(global_routing->loopback);
  } else {
    THROWF(arg_error,0,"No route from '%s' to '%s'??",src_name,dst_name);
    //xbt_dynar_reset(route->link_list);
  }

  if (!are_processing_units && !xbt_dynar_is_empty(route->link_list)) {
    rule_route_extended_t ruleroute_extended =
        (rule_route_extended_t) ruleroute;
    char *gw_src_name = remplace(ruleroute_extended->re_src_gateway, list_src, rc_src,
        list_dst, rc_dst);
    route->src_gateway = (network_element_t)xbt_lib_get_or_null(host_lib, gw_src_name, ROUTING_HOST_LEVEL);
    route->src_gateway = (network_element_t)xbt_lib_get_or_null(host_lib, gw_src_name, ROUTING_HOST_LEVEL);
    if(!route->src_gateway)
      route->src_gateway = (network_element_t)xbt_lib_get_or_null(as_router_lib, gw_src_name, ROUTING_ASR_LEVEL);
    if(!route->src_gateway)
      route->src_gateway = (network_element_t)xbt_lib_get_or_null(as_router_lib, gw_src_name, ROUTING_ASR_LEVEL);
    xbt_free(gw_src_name);

    char *gw_dst_name = remplace(ruleroute_extended->re_dst_gateway, list_src, rc_src,
        list_dst, rc_dst);
    route->dst_gateway = (network_element_t)xbt_lib_get_or_null(host_lib, gw_dst_name, ROUTING_HOST_LEVEL);
    route->dst_gateway = (network_element_t)xbt_lib_get_or_null(host_lib, gw_dst_name, ROUTING_HOST_LEVEL);
    if(!route->dst_gateway)
      route->dst_gateway = (network_element_t)xbt_lib_get_or_null(as_router_lib, gw_dst_name, ROUTING_ASR_LEVEL);
    if(!route->dst_gateway)
      route->dst_gateway = (network_element_t)xbt_lib_get_or_null(as_router_lib, gw_dst_name, ROUTING_ASR_LEVEL);
    xbt_free(gw_dst_name);
  }

  if (list_src)
    pcre_free_substring_list(list_src);
  if (list_dst)
    pcre_free_substring_list(list_dst);
}

static route_t rulebased_get_bypass_route(AS_t rc, network_element_t src, network_element_t dst) {
  return NULL;
}

static void rulebased_finalize(AS_t rc)
{
  AS_rulebased_t routing =
      (AS_rulebased_t) rc;
  if (routing) {
    xbt_dynar_free(&routing->list_route);
    xbt_dynar_free(&routing->list_ASroute);
    model_generic_finalize(rc);
  }
}

/* Creation routing model functions */
AS_t model_rulebased_create(void) {

  AS_rulebased_t new_component = (AS_rulebased_t)
      model_generic_create_sized(sizeof(s_AS_rulebased_t));

  new_component->generic_routing.parse_PU = model_rulebased_parse_PU;
  new_component->generic_routing.parse_AS = model_rulebased_parse_AS;
  new_component->generic_routing.parse_route = model_rulebased_parse_route;
  new_component->generic_routing.parse_ASroute = model_rulebased_parse_ASroute;
  new_component->generic_routing.parse_bypassroute = model_rulebased_parse_bypassroute;
  new_component->generic_routing.get_onelink_routes = rulebased_get_onelink_routes;
  new_component->generic_routing.get_route_and_latency = rulebased_get_route_and_latency;
  new_component->generic_routing.get_bypass_route = rulebased_get_bypass_route;
  new_component->generic_routing.finalize = rulebased_finalize;
  /* initialization of internal structures */
  new_component->list_route = xbt_dynar_new(sizeof(rule_route_t), &rule_route_free);
  new_component->list_ASroute =
      xbt_dynar_new(sizeof(rule_route_extended_t),
                    &rule_route_extended_free);

  return (AS_t) new_component;
}
