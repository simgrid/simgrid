/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING
#include "surf/surfxml_parse_private.h"

static void instr_routing_parse_start_AS (void);
static void instr_routing_parse_end_AS (void);
static void instr_routing_parse_start_link (void);
static void instr_routing_parse_end_link (void);
static void instr_routing_parse_start_host (void);
static void instr_routing_parse_end_host (void);
static void instr_routing_parse_start_router (void);
static void instr_routing_parse_end_router (void);

void instr_routing_define_callbacks ()
{
  surfxml_add_callback(STag_surfxml_AS_cb_list, &instr_routing_parse_start_AS);
  surfxml_add_callback(ETag_surfxml_AS_cb_list, &instr_routing_parse_end_AS);
  surfxml_add_callback(STag_surfxml_link_cb_list, &instr_routing_parse_start_link);
  surfxml_add_callback(ETag_surfxml_link_cb_list, &instr_routing_parse_end_link);
  surfxml_add_callback(STag_surfxml_host_cb_list, &instr_routing_parse_start_host);
  surfxml_add_callback(ETag_surfxml_host_cb_list, &instr_routing_parse_end_host);
  surfxml_add_callback(STag_surfxml_router_cb_list, &instr_routing_parse_start_router);
  surfxml_add_callback(ETag_surfxml_router_cb_list, &instr_routing_parse_end_router);
}

static void instr_routing_parse_start_AS ()
{
}

static void instr_routing_parse_end_AS ()
{
}

static void instr_routing_parse_start_link ()
{
}

static void instr_routing_parse_end_link ()
{
}

static void instr_routing_parse_start_host ()
{
}

static void instr_routing_parse_end_host ()
{
}

static void instr_routing_parse_start_router ()
{
}

static void instr_routing_parse_end_router ()
{
}

#endif

